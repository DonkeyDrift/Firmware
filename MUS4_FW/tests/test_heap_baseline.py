"""堆基线削减 + WS 遥测阈值自适应 + 云端 TLS 心跳安全(v1.10.17 loop-hang 修复).

v1.10.16 实测(.68 车):稳态 free_heap 仅 43–46KB,低于旧的固定 60KB WS 推送
门限(ws_heap_skip 以 ~100/s 增长,WS 曲线与日志 socket 推送被永久饿死);
CloudReporter 在任务生命周期常驻 WiFiClientSecure,mbedTLS 会话(约 40–50KB
堆)是基线堆被吃掉的最大单项。本文件用源码结构断言锁定四件事:

1. WS 推送与日志 socket 推送共用「绝对地板 + 最大可分配块校验 + 滞回恢复」
   的自适应堆门限 webTelemetryHeapOk(),不再以固定 60KB 一刀切禁推;
   43–46KB 实测基线高于恢复线,WS 正常可用,真正的低内存仍有保护。
2. 发送失败(binaryAll DISCARDED)按指数退避、成功复位,防止 16ms 广播节奏
   持续填满 AsyncWebSocket 每客户端队列(WS_MAX_QUEUED_MESSAGES=32)。
3. 云端 TLS 会话改「纪元」常驻(v1.10.17 回归修复):会话对象任务级构造、
   心跳经 keep-alive 复用,任一跳失败立即 client.stop() 整体释放,冷态重建
   过 DRAM(8BIT) 双门限;失败退避 60s→120s→300s 封顶,成功后恢复 5 分钟。
   上报仍在独立 core0 任务,主 loop 不做任何同步网络操作(完整生命周期细节
   另见 test_cloud_reporter_regression.py)。
4. cloud-report 任务每次上报实测栈高水位与堆水位并回传主 loop 打日志,为
   后续栈大小调优提供数据;任务栈保持 16384 字节(TLS 握手调用深度需要余量)。
"""

import pathlib
import re


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
WEB_TELEMETRY_PATH = PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebTelemetry.cpp"
WIFI_TYPES_PATH = PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h"
REPORTER_PATH = PROJECT_ROOT / "libraries" / "mus4_cloud" / "src" / "CloudReporter.cpp"


def _source(path):
    return path.read_text(encoding="utf-8")


def _function_block(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


def _call_block(source, signature):
    start = source.index(signature)
    opening = source.index("(", start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "(":
            depth += 1
        elif source[index] == ")":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"unterminated call: {signature}")


# --- 1. WS 遥测:自适应堆门限,不再 60KB 一刀切 ---


def test_ws_telemetry_no_longer_references_fixed_60kb_gate():
    """WebTelemetry 里不得再出现任何对固定 60KB 门限的比较或引用。

    旧实现 pushWifiWebSocketData / sendWebLogToSocket 直接
    `ESP.getFreeHeap() < WIFI_WEB_TELEMETRY_MIN_FREE_HEAP` 即跳过;.68 稳态
    43–46KB 使 WS 曲线与日志推送永久全死。新策略应完全建立在自适应常量上。
    """
    source = _source(WEB_TELEMETRY_PATH)
    assert "ESP.getFreeHeap() < WIFI_WEB_TELEMETRY_MIN_FREE_HEAP" not in source
    assert "WIFI_WEB_TELEMETRY_MIN_FREE_HEAP" not in source


def test_ws_heap_gate_is_shared_adaptive_with_hysteresis():
    """推送与日志 socket 推送必须走同一个 webTelemetryHeapOk() 门限。

    门限逻辑三要素:free_heap 地板、最大可分配块(防碎片)地板、滞回恢复线。
    进入压力态靠地板击穿,退出靠恢复线——两处不对称才是滞回。低堆跳过计数
    wifiWebSocketHeapSkips 保留(STATUS 字段 ws_heap_skip 兼容)。
    """
    source = _source(WEB_TELEMETRY_PATH)
    gate = _function_block(source, "static bool webTelemetryHeapOk(")
    push = _function_block(source, "static void pushWifiWebSocketData(")
    log_sink = _function_block(source, "static void sendWebLogToSocket(")

    assert "ESP.getFreeHeap()" in gate
    assert "ESP.getMaxAllocHeap()" in gate
    assert "WIFI_WEB_TELEMETRY_PUSH_FLOOR_HEAP" in gate
    assert "WIFI_WEB_TELEMETRY_PUSH_MIN_MAX_ALLOC_HEAP" in gate
    assert "WIFI_WEB_TELEMETRY_RECOVERY_HEAP" in gate
    # 滞回:进入条件(地板)先于退出条件(恢复线)
    assert gate.index("PUSH_FLOOR_HEAP") < gate.index("RECOVERY_HEAP")

    assert "webTelemetryHeapOk()" in push
    assert "webTelemetryHeapOk()" in log_sink
    # 堆门限必须先于实际广播执行
    assert push.index("webTelemetryHeapOk()") < push.index("binaryAll")
    assert "wifiWebSocketHeapSkips++" in push


def test_ws_heap_policy_constants_keep_baseline_working_with_real_protection():
    """自适应常量的数值边界:基线可用 + 真低内存保护 + 滞回带有效。

    实测 .68 稳态 free_heap 43–46KB:地板必须显著低于 43KB(WS 正常可用),
    恢复线必须低于 43KB(压力态不会在基线附近永久卡死),又必须高于地板
    (滞回带存在)。最大可分配块校验在 KB 量级即可覆盖 WS 每帧分配需求。
    旧常量 60000 仍保留定义(STATUS 兼容、feature-flags 锁定行)。
    """
    source = _source(WIFI_TYPES_PATH)
    floor = re.search(r"const uint32_t WIFI_WEB_TELEMETRY_PUSH_FLOOR_HEAP = (\d+);", source)
    recovery = re.search(r"const uint32_t WIFI_WEB_TELEMETRY_RECOVERY_HEAP = (\d+);", source)
    max_alloc = re.search(
        r"const uint32_t WIFI_WEB_TELEMETRY_PUSH_MIN_MAX_ALLOC_HEAP = (\d+);", source
    )

    assert floor is not None, "缺少 WIFI_WEB_TELEMETRY_PUSH_FLOOR_HEAP"
    assert recovery is not None, "缺少 WIFI_WEB_TELEMETRY_RECOVERY_HEAP"
    assert max_alloc is not None, "缺少 WIFI_WEB_TELEMETRY_PUSH_MIN_MAX_ALLOC_HEAP"
    floor_v = int(floor.group(1))
    recovery_v = int(recovery.group(1))
    max_alloc_v = int(max_alloc.group(1))

    assert 16384 <= floor_v <= 32768, "地板应远低于 43KB 实测基线,又不能低到失去保护意义"
    assert floor_v < recovery_v <= 42000, "恢复线须高于地板(滞回)且低于 43KB 基线,否则压力态卡死"
    assert 4096 <= max_alloc_v <= 16384, "maxAlloc 校验应覆盖 WS 每帧分配,防碎片误判"

    assert "const uint32_t WIFI_WEB_TELEMETRY_MIN_FREE_HEAP = 60000;" in source


# --- 2. WS 发送失败指数退避 ---


def test_ws_send_failure_backs_off_and_resets_on_success():
    """binaryAll DISCARDED 必须指数退避,成功一帧即复位。

    慢客户端会填满 AsyncWebSocket 每客户端队列(WS_MAX_QUEUED_MESSAGES=32,
    每条约 400B → 单客户端最坏 ~13KB 堆)。继续 16ms 节奏硬推只会把堆耗在
    注定丢弃的拷贝上;退避 + 复位让健康客户端立即恢复满速。
    """
    types = _source(WIFI_TYPES_PATH)
    source = _source(WEB_TELEMETRY_PATH)
    push = _function_block(source, "static void pushWifiWebSocketData(")

    start = re.search(r"const unsigned long WIFI_WEB_SOCKET_SEND_BACKOFF_START_MS = (\d+);", types)
    cap = re.search(r"const unsigned long WIFI_WEB_SOCKET_SEND_BACKOFF_MAX_MS = (\d+);", types)
    assert start is not None, "缺少退避起始间隔常量"
    assert cap is not None, "缺少退避上限常量"
    start_v = int(start.group(1))
    cap_v = int(cap.group(1))
    assert 200 <= start_v <= 2000
    assert start_v < cap_v <= 30000

    assert "wifiWebSocketSendBackoffUntilMs" in push
    # 毫秒回绕安全比较
    assert "(long)(now - wifiWebSocketSendBackoffUntilMs) < 0" in push
    # 失败分支:计数 + 指数翻倍
    assert "wifiWebSocketQueueFullSkips++" in push
    assert "wifiWebSocketSendBackoffMs *= 2" in push
    # 成功分支:复位到起始间隔
    assert "wifiWebSocketSendBackoffMs = WIFI_WEB_SOCKET_SEND_BACKOFF_START_MS;" in push


# --- 3. 云端 TLS 心跳:会话按需建立、用完释放;节奏与重试语义不变 ---


def test_cloud_tls_session_resident_epoch_with_release_on_failure():
    """TLS 会话锁定「纪元」常驻模型(v1.10.17 回归修复后)。

    上一轮的按次建连已被实测证伪:DRAM(8BIT) 池稳态仅 ~50.7KB,而一次 TLS
    尝试峰值 ~48.8KB,按次建连只有 ~1.9KB 余量,60s 快速重试曾把池打到 108
    字节并两次 PANIC。模型改为:会话对象在 worker 循环前任务级构造并跨心跳
    常驻(keep-alive 复用,稳态每跳 ~0KB 新分配),源码中恰好一处;任一跳
    失败立即 client.stop() 整体释放进入冷态,冷态重建须过 DRAM 双门限。
    """
    source = _source(REPORTER_PATH)
    worker = _function_block(source, "void cloudReportTask(")
    loop_start = worker.index("for (;;")

    # 会话对象任务级构造(循环前),源码中恰好一处——不再按次建连
    assert "WiFiClientSecure client;" in worker[:loop_start]
    assert "HTTPClient http;" in worker[:loop_start]
    assert worker.count("WiFiClientSecure client;") == 1
    assert worker.count("HTTPClient http;") == 1
    # 失败分支显式整体释放(纪元终结),stop 位于循环内而非构造处
    assert "client.stop();" in worker
    assert worker.index("client.stop();") > loop_start


def test_cloud_report_cadence_and_retry_semantics_unchanged():
    """纪元常驻 + 早期窗口不得破坏既有节奏约束:稳态 5 分钟、早期窗口结束后
    按 60s→120s→300s 退避阶梯重试(封顶 = 稳态间隔,低堆跳过计入同一阶梯)、
    成功后失败计数清零、换 IP 补报后无条件记住 IP(防死循环)。开机早期窗口
    的积极重试语义由 test_cloud_early_session.py 锁定。"""
    source = _source(REPORTER_PATH)
    worker = _function_block(source, "void cloudReportTask(")

    assert "CLOUD_REPORT_INTERVAL_MS = 300000UL" in source
    assert "CLOUD_REPORT_RETRY_MS = 60000UL" in source
    # 间隔统一由纯函数推导:成功过 → 5 分钟稳态;早期窗口 → 积极重试;否则退避阶梯
    assert "retryIntervalMs(now, everSucceeded, consecutiveFailures)" in worker
    ladder = _function_block(source, "static unsigned long retryBackoffMs(")
    assert "CLOUD_REPORT_RETRY_MS" in ladder
    assert "CLOUD_REPORT_RETRY_MS * 2" in ladder
    assert "CLOUD_REPORT_INTERVAL_MS" in ladder
    # 成功复位:连续失败清零,退避回阶梯基数
    assert "consecutiveFailures = 0;" in worker
    assert "consecutiveFailures += 1;" in worker
    assert "lastReportedIp = ip;" in worker
    assert "everSucceeded = true;" in worker


def test_cloud_report_task_stays_independent_on_core0():
    """主 loop 依旧不做同步网络操作;worker 仍惰性创建并钉在 core0。"""
    source = _source(REPORTER_PATH)
    update = _function_block(source, "void update()")

    assert "reportNow(" not in update
    assert ".POST(" not in update
    create = _call_block(update, "xTaskCreatePinnedToCore")
    arguments = create[create.index("(") + 1 : -1].split(",")
    assert arguments[0].strip() == "cloudReportTask"
    assert arguments[2].strip() == "CLOUD_REPORT_TASK_STACK_SIZE"
    assert arguments[-1].strip() == "0"  # core0


# --- 4. 栈/堆实测:基线调优数据,不再盲调 ---


def test_cloud_worker_measures_stack_and_heap_and_main_loop_logs_it():
    """worker 每次上报后实测栈高水位与堆水位,只回传 POD;主 loop 消费打日志。

    v1.10.16 的诊断盲区:无从判断 16KB 任务栈是否浪费、堆基线何时恶化。
    测量点必须在 worker 上下文(高水位只在本任务读才有意义),日志写入留在
    主 loop(与既有约束一致:worker/attempt 不调用 mus4Log*)。任务栈大小
    保持 16384 字节——TLS 握手调用深度需要余量,待实测数据再收敛。
    """
    source = _source(REPORTER_PATH)
    worker = _function_block(source, "void cloudReportTask(")
    emit = _function_block(source, "static void emitPendingReportLog(")

    assert "uxTaskGetStackHighWaterMark(nullptr)" in worker
    assert "ESP.getFreeHeap()" in worker
    assert "ESP.getMinFreeHeap()" in worker
    # POD 回传通道
    assert "freeHeapAfter" in source
    assert "minFreeHeap" in source
    assert "stackHwmBytes" in source
    # 主 loop 日志带实测字段
    assert "stack_hwm" in emit
    assert "min_heap" in emit
    # 栈大小保持 TLS 安全余量
    stack = re.search(r"CLOUD_REPORT_TASK_STACK_SIZE\s*=\s*(\d+)", source)
    assert stack is not None
    assert int(stack.group(1)) == 16384
