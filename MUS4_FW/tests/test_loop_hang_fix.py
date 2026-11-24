"""v1.10.17 loop-hang fix 源码结构锁定测试。

病灶（6 路排查结论）：同步 WebServer 的 handleClient() 跑在主 loopTask 内，
core 3.3.10 的 NetworkClient::write() = select(1s) × 重试 10 次，客户端 TCP 停滞
时单次 HTTP 应答最长阻塞 ~20s，主 loop 的 RC 滤波/控制输出/Serial1 遥测全部
冻结（实测 web_http_dt_max=18997）。本文件锁定四层修复：

1. handleClient 移至独立 FreeRTOS 任务（栈/优先级/核绑定常量集中在 FirmwareConfig.h）；
2. loopTask 订阅 esp_task_wdt 并每帧喂狗；网络任务不订阅 TWDT（OTA 上传合法长跑，
   任何小于它的超时都会把正常 OTA 误判为死机），改用软看护告警；
3. STATUS 文本与 /api/status（同一 printWirelessStatus 实现）增加真实
   min_free_heap 水位、reset_reason 可读文本、loop_dt_max_ms 帧间隔峰值；
4. 跨任务共享数据去 String 化（hostWifi*/hostReportedIp 改 char 数组）+ wifiWebData
   环形缓冲 head/count 发布与读取走 portMUX 最小临界区（历史撕堆前科防线）。
"""

import pathlib
import re


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
SKETCH = PROJECT_ROOT / "MUS4_FW.ino"
CONFIG = PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "FirmwareConfig.h"
SERVER = PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp"
SERVER_H = PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.h"
LOOP_DIAG_H = PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "LoopDiagnostics.h"
LOOP_DIAG_CPP = PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "LoopDiagnostics.cpp"


def body_of(source, signature_pattern):
    m = re.search(signature_pattern, source, re.DOTALL)
    assert m is not None, f"未找到函数体: {signature_pattern}"
    return m.group(1)


WEB_TASK_SIGNATURE = r"static void wifiWebConsoleTask\(void\s*\*\s*arg\)\s*\{(.*?)\n\}"


def test_web_task_constants_are_centralized_in_firmware_config():
    """网络任务的栈/优先级/核绑定/轮询与看门狗超时常量必须集中在 FirmwareConfig.h。"""
    config = CONFIG.read_text(encoding="utf-8")

    assert re.search(r"^#define\s+WIFI_WEB_TASK_ENABLED\s+1\b", config, re.MULTILINE)
    assert re.search(r"^#define\s+WIFI_WEB_TASK_STACK_SIZE\s+8192\b", config, re.MULTILINE)
    assert re.search(r"^#define\s+WIFI_WEB_TASK_PRIORITY\s+2\b", config, re.MULTILINE)
    assert re.search(r"^#define\s+WIFI_WEB_TASK_CORE\s+0\b", config, re.MULTILINE)
    assert re.search(r"^#define\s+WIFI_WEB_TASK_POLL_INTERVAL_MS\s+2\b", config, re.MULTILINE)
    assert re.search(r"^#define\s+WIFI_WEB_TASK_STALL_WARN_MS\s+2000\b", config, re.MULTILINE)
    assert re.search(r"^#define\s+LOOP_TASK_WDT_ENABLED\s+1\b", config, re.MULTILINE)
    assert re.search(r"^#define\s+LOOP_TASK_WDT_TIMEOUT_S\s+5\b", config, re.MULTILINE)


def test_main_loop_no_longer_drives_http_handle_client():
    """handleClient 必须由独立任务驱动，主 loop 路径只保留遥测采样。

    - wifiWebConsoleTask 任务函数内调用 wifiWebServer.handleClient()；
    - xTaskCreatePinnedToCore 使用 FirmwareConfig.h 的栈/优先级/核常量；
    - updateWebConsoleServer()（主 loop 每帧调用）不再调用 handleClient，
      但保留 sampleWifiWebData() 遥测采样与 OTA 期间跳过采样的原语义；
    - 任务驱动门槛与原实现一致（consoleStarted/staConnected 任一就绪）。
    """
    server = SERVER.read_text(encoding="utf-8")
    sketch = SKETCH.read_text(encoding="utf-8")

    task_body = body_of(server, WEB_TASK_SIGNATURE)
    assert "wifiWebServer.handleClient()" in task_body
    # 驱动门槛：与原 updateWebConsoleServer 的门槛同款
    assert "wifiRuntime.consoleStarted" in task_body
    assert "wifiRuntime.staConnected" in task_body

    assert "xTaskCreatePinnedToCore(wifiWebConsoleTask, WIFI_WEB_TASK_NAME, WIFI_WEB_TASK_STACK_SIZE" in server
    assert "WIFI_WEB_TASK_PRIORITY" in server
    assert "WIFI_WEB_TASK_CORE" in server

    update_body = body_of(server, r"void updateWebConsoleServer\(\)\s*\{(.*?)\n\}")
    # 主 loop 侧不得再直驱 HTTP（除「任务创建失败」的显式兜底分支外）
    assert "wifiWebServer.handleClient()" not in update_body or (
        "s_wifiWebTaskRunning" in update_body
        and update_body.index("s_wifiWebTaskRunning") < update_body.index("wifiWebServer.handleClient()")
    ), "updateWebConsoleServer 不得直驱 handleClient（仅允许任务创建失败的显式兜底）"
    # 遥测采样留在主 loop，OTA 上传期间跳过采样的原语义不变
    assert "sampleWifiWebData()" in update_body
    assert "otaRuntime.inProgress" in update_body
    assert update_body.index("otaRuntime.inProgress") < update_body.index("sampleWifiWebData()")

    # sketch loop() 里不得出现任何 handleClient 调用
    assert "handleClient" not in sketch


def test_web_task_policy_no_twdt_with_soft_stall_watch():
    """网络任务的看门狗策略：不订阅 TWDT（OTA 上传会在 handleClient 内合法运行
    数分钟，任何小于它的超时都会误触发复位），改用软看护——单次 handleClient
    超过 WIFI_WEB_TASK_STALL_WARN_MS 且非 OTA 时打告警日志；控制安全由 loopTask
    TWDT 兜底。loopTask 则必须订阅并每帧喂狗。"""
    server = SERVER.read_text(encoding="utf-8")
    sketch = SKETCH.read_text(encoding="utf-8")
    loop_diag_cpp = LOOP_DIAG_CPP.read_text(encoding="utf-8")
    loop_diag_h = LOOP_DIAG_H.read_text(encoding="utf-8")

    task_body = body_of(server, WEB_TASK_SIGNATURE)
    assert "esp_task_wdt_add" not in task_body, "网络任务不得订阅 TWDT（OTA 上传合法长跑，会误判）"
    assert "esp_task_wdt" not in task_body
    assert "WIFI_WEB_TASK_STALL_WARN_MS" in task_body
    assert "otaRuntime.inProgress" in task_body, "软看护告警必须在 OTA 上传期间豁免"

    # loopTask 看门狗：订阅(NULL) + 每帧喂狗
    assert "esp_task_wdt_add(NULL)" in loop_diag_cpp
    assert "esp_task_wdt_reset()" in loop_diag_cpp
    assert "esp_task_wdt_init(" in loop_diag_cpp
    assert "LOOP_TASK_WDT_TIMEOUT_S" in loop_diag_cpp
    assert "void loopDiagnosticsBegin()" in loop_diag_h
    assert "void loopDiagnosticsFeed()" in loop_diag_h

    # 订阅发生在 setup() 末尾（Wi-Fi/STA 首连可能阻塞 >5s，不能在 setup 前段订阅），
    # 喂狗在 loop() 末尾每帧执行。
    sketch_setup = body_of(sketch, r"void setup\(\)\s*\{(.*?)\n\}")
    sketch_loop = body_of(sketch, r"void loop\(\)\s*\{(.*?)\n\}")
    assert "loopDiagnosticsBegin();" in sketch_setup
    assert "loopDiagnosticsFeed();" in sketch_loop
    assert sketch_setup.index("loopDiagnosticsBegin();") > sketch_setup.index("setupWifiConsole();"), (
        "loopTask 看门狗必须在 setupWifiConsole()（可能阻塞 15s 的首连）之后订阅，避免开机误复位"
    )


def test_status_reports_real_min_free_heap_watermark():
    """min_free_heap 不得再用编译期常量 WIFI_WEB_TELEMETRY_MIN_FREE_HEAP 冒充，
    必须来自 esp_get_minimum_free_heap_size() 实测水位；字段名保留旧名兼容。"""
    server = SERVER.read_text(encoding="utf-8")

    assert "esp_get_minimum_free_heap_size()" in server
    # 旧字段名保留
    assert "min_free_heap=%lu" in server
    status_printf = body_of(server, r'void printWirelessStatus\(Print& out\)\s*\{(.*?)\n\}')
    assert "WIFI_WEB_TELEMETRY_MIN_FREE_HEAP" not in status_printf, (
        "STATUS 的 min_free_heap 值不得来自编译期常量"
    )


def test_status_reports_reset_reason_and_loop_dt_max():
    """STATUS 文本与 /api/status（同一实现）输出 reset_reason 可读文本与
    loop_dt_max_ms 主 loop 帧间隔峰值；字段命名风格与现有一致（小写下划线）。"""
    server = SERVER.read_text(encoding="utf-8")
    loop_diag_cpp = LOOP_DIAG_CPP.read_text(encoding="utf-8")

    assert "reset_reason=%s" in server
    assert "loop_dt_max_ms=%lu" in server
    assert "esp_reset_reason()" in loop_diag_cpp
    assert "espResetReasonText(" in server
    assert "loopDiagnosticsSnapshot(" in server
    # 可读文本映射：看门狗/panic 等关键复位原因必须有名字
    for reason in ("TASK_WDT", "PANIC", "BROWNOUT", "POWERON", "SW_RESET"):
        assert f'"{reason}"' in loop_diag_cpp
    # 未知枚举不能输出空串
    assert "OTHER_" in loop_diag_cpp


def test_status_slot_kind_and_reset_reason_copy_out_of_shared_string():
    """printWirelessStatus 现在可能同时从主 loop（STATUS 命令）与 Web 任务
    （/api/status）并发执行。槽位身份缓存 String 必须互斥重建、锁内拷贝返回，
    不得把共享 String 的 .c_str() 指针泄漏出锁（悬垂/并发 realloc 撕堆）。"""
    server = SERVER.read_text(encoding="utf-8")

    assert "static void otherSlotKindText(char* buf, size_t len)" in server
    assert "xSemaphoreTake(s_otherSlotKindMutex" in server
    assert "strlcpy(buf, s_otherSlotKind.c_str(), len)" in server
    # 旧的裸指针返回不得回归
    assert "return s_otherSlotKind.c_str()" not in server
    assert "otherSlotKindText(otherSlotBuf, sizeof(otherSlotBuf));" in server


def test_shared_host_wifi_strings_are_pod_buffers():
    """hostWifi*/hostReportedIp 由主 loop（handleSerial2）写、Web 任务读，禁止
    跨任务共享可变 static String（v1.7.16/17 撕堆前科）；必须是 char 数组。"""
    server = SERVER.read_text(encoding="utf-8")
    server_h = SERVER_H.read_text(encoding="utf-8")
    sketch = SKETCH.read_text(encoding="utf-8")

    assert "char hostWifiStatus[" in server_h
    assert "char hostReportedIp[" in server_h
    assert "extern char hostWifiStatus[" in server_h
    assert "extern char hostReportedIp[" in server_h

    for name in ("hostWifiStatus", "hostWifiSsid", "hostWifiIp", "hostWifiError", "hostReportedIp"):
        assert f"String {name}" not in server, f"{name} 必须是 char 数组而非 String"

    # main 侧 FirmwareApp（taskSerialRead）跨编译单元调用 handleSerial2，定义为
    # 非 static；Tony 侧为 static。两种签名都接受，只锁定函数体结构。
    sketch_handle_serial2 = body_of(
        sketch, r"(?:static )?void handleSerial2\(\)\s*\{(.*?)\n\}")
    assert "String hostWifiStatus" not in sketch_handle_serial2
    assert "hostWifiStatus = String(" not in sketch
    assert "hostReportedIp = String(" not in sketch
    assert "strlcpy(hostWifiStatus, " in sketch_handle_serial2
    assert "strlcpy(hostReportedIp, " in sketch_handle_serial2


def test_webdata_ring_publisher_and_reader_use_critical_section_snapshot():
    """wifiWebData 环形缓冲：主 loop 写、Web 任务（/api/data）读。head/count 的
    发布（写侧）与快照（读侧）必须走同一 portMUX 临界区；读者用快照遍历，
    槽位内容先于指针发布写完，读者范围不含写入中的槽位。"""
    sketch = SKETCH.read_text(encoding="utf-8")
    server = SERVER.read_text(encoding="utf-8")

    assert "portMUX_TYPE wifiWebDataMux = portMUX_INITIALIZER_UNLOCKED;" in sketch
    assert "extern portMUX_TYPE wifiWebDataMux;" in server

    sample_body = body_of(sketch, r"void sampleWifiWebData\(\)\s*\{(.*?)\n\}")
    assert sample_body.count("portENTER_CRITICAL(&wifiWebDataMux)") >= 2, (
        "写侧必须在临界区内发布 seq 与 head/count"
    )
    assert sample_body.count("portEXIT_CRITICAL(&wifiWebDataMux)") >= 2

    data_body = body_of(server, r"static void handleWifiWebData\(\)\s*\{(.*?)\n\}")
    assert "portENTER_CRITICAL(&wifiWebDataMux)" in data_body
    assert "headSnap" in data_body
    assert "countSnap" in data_body


def test_ota_upload_flow_and_503_middleware_unchanged():
    """改造不得动 OTA 上传流程与 503 中间件语义（行为回归防线）。"""
    server = SERVER.read_text(encoding="utf-8")

    assert 'wifiWebServer.on("/update", HTTP_POST, handleWifiWebUpdatePost, handleWifiWebUpdateUpload);' in server
    setup_body = body_of(server, r"void setupWebConsoleServer\(\)\s*\{(.*?)\n\}")
    assert "addMiddleware" in setup_body
    assert 'server.uri() != "/update"' in setup_body
    assert "503" in setup_body
    # OTA 上传开始时的 park 强制与状态置位保持原样
    upload_body = body_of(server, r"static void handleWifiWebUpdateUpload\(\)\s*\{(.*?)\n\}")
    assert "os.parkGuardActive = true;" in upload_body
    assert "forceWifiOtaParkLocked();" in upload_body
    assert "os.inProgress = true;" in upload_body
