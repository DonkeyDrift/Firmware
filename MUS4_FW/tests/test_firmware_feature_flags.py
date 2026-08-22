import pathlib
import re


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
MUS4_SKETCH = PROJECT_ROOT / "MUS4_FW.ino"
FIRMWARE_SOURCE_PATHS = [
    MUS4_SKETCH,
    PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "FirmwareConfig.h",
    PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h",
    PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "StringPrint.h",
    PROJECT_ROOT / "libraries" / "mus4_log" / "src" / "JsonUtil.h",
    PROJECT_ROOT / "libraries" / "mus4_log" / "src" / "JsonUtil.cpp",
    PROJECT_ROOT / "libraries" / "mus4_i2c" / "src" / "I2CBusTools.h",
    PROJECT_ROOT / "libraries" / "mus4_i2c" / "src" / "I2CBusTools.cpp",
    PROJECT_ROOT / "libraries" / "mus4_ui" / "src" / "LedStatus.h",
    PROJECT_ROOT / "libraries" / "mus4_ui" / "src" / "LedStatus.cpp",
    PROJECT_ROOT / "libraries" / "mus4_log" / "src" / "Mus4Log.h",
    PROJECT_ROOT / "libraries" / "mus4_log" / "src" / "Mus4Log.cpp",
    PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "JoystickCalibration.h",
    PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "JoystickCalibration.cpp",
    PROJECT_ROOT / "libraries" / "mus4_i2c" / "src" / "Sensors.h",
    PROJECT_ROOT / "libraries" / "mus4_i2c" / "src" / "Sensors.cpp",
    PROJECT_ROOT / "libraries" / "mus4_diag" / "src" / "GamepadMode.h",
    PROJECT_ROOT / "libraries" / "mus4_diag" / "src" / "GamepadMode.cpp",
    PROJECT_ROOT / "libraries" / "mus4_rc" / "src" / "RcFilter.h",
    PROJECT_ROOT / "libraries" / "mus4_rc" / "src" / "RcFilter.cpp",
    PROJECT_ROOT / "libraries" / "mus4_rc" / "src" / "RcPwmCapture.h",
    PROJECT_ROOT / "libraries" / "mus4_rc" / "src" / "RcPwmCapture.cpp",
    PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "ControlMixer.h",
    PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "ControlMixer.cpp",
    PROJECT_ROOT / "libraries" / "mus4_safety" / "src" / "SafetyState.h",
    PROJECT_ROOT / "libraries" / "mus4_safety" / "src" / "SafetyState.cpp",
    PROJECT_ROOT / "libraries" / "mus4_safety" / "src" / "ActuatorOutput.h",
    PROJECT_ROOT / "libraries" / "mus4_safety" / "src" / "ActuatorOutput.cpp",
    PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "CommandParser.h",
    PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "CommandParser.cpp",
    PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "CommandDispatcher.h",
    PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "CommandDispatcher.cpp",
    PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "LocalCommands.h",
    PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "LocalCommands.cpp",
    PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "SerialLineReader.h",
    PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "SerialLineReader.cpp",
    PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h",
    PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "WirelessConsole.h",
    PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "WirelessConsole.cpp",
    PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiStaConfig.h",
    PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiStaConfig.cpp",
    PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiStaHistory.h",
    PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiStaHistory.cpp",
    PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiIdentity.h",
    PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiIdentity.cpp",
    PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "DriftAssist.h",
    PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "DriftAssist.cpp",
    PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "SteeringControl.h",
    PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "SteeringControl.cpp",
    PROJECT_ROOT / "libraries" / "mus4_diag" / "src" / "Diagnostics.h",
    PROJECT_ROOT / "libraries" / "mus4_diag" / "src" / "Diagnostics.cpp",
    PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "SerialBufferTypes.h",
    PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebLogBuffer.h",
    PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebLogBuffer.cpp",
    PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp",
    PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "WirelessConsole.cpp",
    PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiOta.h",
    PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiOta.cpp",
    PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebTelemetry.h",
    PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebTelemetry.cpp",
    PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.h",
    PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.cpp",
    PROJECT_ROOT / "libraries" / "mus4_auth" / "src" / "AuthService.h",
    PROJECT_ROOT / "libraries" / "mus4_auth" / "src" / "AuthService.cpp",
    PROJECT_ROOT / "libraries" / "mus4_auth" / "library.properties",
]
ARDUINO_WSL_SCRIPT = PROJECT_ROOT / "arduino-cli-wsl.ps1"
CONFIG_YAML = PROJECT_ROOT / "config.yaml"
WSLBUILD_YAML = PROJECT_ROOT / "wslbuild.yaml"
BUILD_INFO = PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "BuildInfo.h"
CHANGELOG = PROJECT_ROOT / "CHANGELOG.md"
SMART_PROVISIONING_SKETCH = PROJECT_ROOT / "examples" / "smart_provisioning" / "smart_provisioning.ino"
SMART_PROVISIONING_WEB_UI = PROJECT_ROOT / "examples" / "smart_provisioning" / "web_ui.h"


def firmware_source_text():
    return "\n".join(
        path.read_text(encoding="utf-8")
        for path in FIRMWARE_SOURCE_PATHS
        if path.exists()
    )


def test_local_libraries_path_is_configured_for_build_tools():
    wsl_script = ARDUINO_WSL_SCRIPT.read_text(encoding="utf-8")
    config = CONFIG_YAML.read_text(encoding="utf-8")
    wsl_config = WSLBUILD_YAML.read_text(encoding="utf-8")

    assert 'libraries_path: "libraries"' in config
    assert "libraries_path: libraries" in wsl_config
    assert "libraries_path" in wsl_script
    assert "--libraries" in wsl_script
    assert "$WSLWorkDir/$script:LibrariesPath" in wsl_script
    assert "$WSLProjectRoot/$script:LibrariesPath" in wsl_script


def test_wsl_upload_prefers_app_firmware_bin_and_excludes_auxiliary_bins():
    wsl_script = ARDUINO_WSL_SCRIPT.read_text(encoding="utf-8")

    assert "function Test-IsAppFirmwareBin" in wsl_script
    assert "function Get-AppFirmwareBin" in wsl_script
    for excluded in [
        "*_flashed.bin",
        "*.partitions.bin",
        "*.bootloader.bin",
        "*.merged.bin",
        "boot_app0*.bin",
    ]:
        assert excluded in wsl_script
    assert "Sort-Object @{Expression = { if ($_.Name -eq $PreferredName) { 0 } else { 1 } }}" in wsl_script
    assert "Get-AppFirmwareBin -SearchDirs $candidateBuildDirs -PreferredName $PreferredBinName" in wsl_script


def test_smart_provisioning_example_returns_ip_before_closing_ap():
    source = SMART_PROVISIONING_SKETCH.read_text(encoding="utf-8")

    assert "WiFi.mode(WIFI_AP_STA)" in source
    assert "server.on(\"/config\", HTTP_POST, handleConfig)" in source
    assert "server.send(statusCode, \"application/json\", body)" in source
    assert "WiFi.localIP().toString()" in source
    assert "scheduleApShutdown()" in source
    assert "WiFi.softAPdisconnect(true)" in source
    assert "dnsServer.processNextRequest()" in source
    assert "MDNS.begin(MDNS_HOSTNAME)" in source
    assert "Password: <redacted>" in source
    assert "WiFi.mode(WIFI_STA)" in source
    assert "delay(AP_SHUTDOWN_DELAY_MS)" not in source


def test_smart_provisioning_web_ui_polls_new_ip_and_falls_back_to_mdns():
    source = SMART_PROVISIONING_WEB_UI.read_text(encoding="utf-8")

    assert "fetch('/config'" in source
    assert "'Content-Type':'application/json'" in source
    assert "JSON.stringify({ssid,password})" in source
    assert "http://${ip}/" in source
    assert "mode:'no-cors'" in source
    assert "await sleep(2000)" in source
    assert "http://esp32.local/" in source
    assert "location.href=url" in source
    assert "手动打开" in source


def test_websocket_curve_data_feature_is_enabled_and_streams_logs():
    source = firmware_source_text()
    web_telemetry = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebTelemetry.cpp").read_text(encoding="utf-8")

    assert re.search(r"^#define\s+ENABLE_WIFI_WEBSOCKET_TELEMETRY\b", source, re.MULTILINE)
    assert "sendWebLogToSocket" in web_telemetry
    assert "webLogBufferSetSocketSink(sendWebLogToSocket)" in web_telemetry
    assert r'\"type\":\"log\"' in web_telemetry


def test_websocket_event_callback_does_not_invoke_log_sink_in_async_task():
    """v1.7.16：AsyncTCP task 上的 onEvent 回调不得直接调用 mus4LogLine / mus4Logf /
    appendWebLog —— 否则会触发 sendWebLogToSocket 在 AsyncTCP task 上写共享 String
    `wifiWebSocketPayload`，与 main loop 上的同一 String 操作并发 realloc 撕裂堆，
    最终表现为 `bad magic` 与设备 reboot。改为只在 main loop 消费连接事件标志。

    v1.7.17 进一步：onEvent 回调也不得调 sendWifiWebSocketHello —— 它内部也写
    共享 String。Hello 帧改由 main loop 在消费 pendingWsConnectEvent 时发出。"""

    web_telemetry = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebTelemetry.cpp").read_text(encoding="utf-8")

    handler_match = re.search(
        r"static void handleWifiWebSocketEvent\([^)]*\)\s*\{(.*?)^\}",
        web_telemetry,
        re.DOTALL | re.MULTILINE,
    )
    assert handler_match is not None, "未找到 handleWifiWebSocketEvent 函数体"
    body = handler_match.group(1)
    assert "mus4LogLine(" not in body, "handleWifiWebSocketEvent 不得调 mus4LogLine（会在 AsyncTCP task 触发 sink）"
    assert "mus4Logf(" not in body, "handleWifiWebSocketEvent 不得调 mus4Logf（同上）"
    assert "appendWebLog(" not in body, "handleWifiWebSocketEvent 不得调 appendWebLog（同上）"
    # v1.7.17：hello 帧也不能在 AsyncTCP task 发，否则继续与 main loop 撕共享 String。
    assert "sendWifiWebSocketHello(" not in body, "handleWifiWebSocketEvent 不得调 sendWifiWebSocketHello（共享 String race）"

    # 改用主循环消费的 volatile 标志。
    assert "volatile bool pendingWsConnectEvent" in web_telemetry
    assert "volatile bool pendingWsDisconnectEvent" in web_telemetry
    # updateWifiWebSocket 主循环里读标志后打日志 + 发 hello（sink/hello 唯一安全的触发点）。
    update_match = re.search(
        r"void updateWifiWebSocket\(\)\s*\{(.*?)^\}",
        web_telemetry,
        re.DOTALL | re.MULTILINE,
    )
    assert update_match is not None
    update_body = update_match.group(1)
    assert "pendingWsConnectEvent" in update_body
    assert "pendingWsDisconnectEvent" in update_body
    assert 'mus4LogLine("web", "ws connected")' in update_body
    assert 'mus4LogLine("web", "ws disconnected")' in update_body
    assert "sendWifiWebSocketHello(" in update_body, "main loop 必须在消费 pendingWsConnectEvent 时发 hello"


def test_websocket_text_payloads_never_share_a_static_string():
    """v1.7.17：杀掉 race 的根本一刀 —— 不再保留任何跨函数/跨上下文共享的 text 缓冲
    `static String wifiWebSocketPayload`。hello 和 log JSON 各自用栈上局部 String，
    Arduino `String::operator+=` 的 realloc 只触碰本函数私有堆块，永远不会被另一
    上下文 realloc 同一块。"""

    web_telemetry = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebTelemetry.cpp").read_text(encoding="utf-8")

    # 共享 String 必须消失（任何形式的命名 / 任何作用域）。
    assert "static String wifiWebSocketPayload" not in web_telemetry, (
        "static String wifiWebSocketPayload 必须删除：跨上下文共享会撕堆"
    )
    assert "wifiWebSocketPayload.reserve" not in web_telemetry, (
        "v1.7.17 后不再有共享 String，setup 也不应再 reserve 它"
    )

    # 两个发送函数体内必须各自声明 `String payload`（局部）。
    hello_match = re.search(
        r"static void sendWifiWebSocketHello\([^)]*\)\s*\{(.*?)^\}",
        web_telemetry,
        re.DOTALL | re.MULTILINE,
    )
    assert hello_match, "找不到 sendWifiWebSocketHello 函数体"
    assert "String payload" in hello_match.group(1), (
        "sendWifiWebSocketHello 必须用栈上局部 String payload"
    )

    sink_match = re.search(
        r"static void sendWebLogToSocket\([^)]*\)\s*\{(.*?)^\}",
        web_telemetry,
        re.DOTALL | re.MULTILINE,
    )
    assert sink_match, "找不到 sendWebLogToSocket 函数体"
    assert "String payload" in sink_match.group(1), (
        "sendWebLogToSocket 必须用栈上局部 String payload"
    )


def test_websocket_uses_broadcast_not_raw_client_pointer():
    """v1.7.16：消除裸 `wifiWebSocketClient` 指针 deref —— `WS_EVT_DISCONNECT` 在
    AsyncTCP task 上把指针置 nullptr 与 main loop 在 pushWifiWebSocketData /
    sendWebLogToSocket 中的检查 + 调用之间存在 TOCTOU。
    v1.7.26：支持多客户端并发观看曲线，日志与数据改为广播
    `wifiWebSocket.textAll(...)` / `wifiWebSocket.binaryAll(...)`，只序列化一次并
    通过共享 buffer 分发给所有 client，避免每个客户端重复序列化导致卡顿。"""

    web_telemetry = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebTelemetry.cpp").read_text(encoding="utf-8")

    # sendWebLogToSocket / pushWifiWebSocketData 必须用广播路径
    assert "wifiWebSocket.textAll(" in web_telemetry
    assert "wifiWebSocket.binaryAll(" in web_telemetry
    # 不再出现裸指针 ->text / ->binary，也不再按单 client id 发送
    assert "wifiWebSocketClient->text(" not in web_telemetry
    assert "wifiWebSocketClient->binary(" not in web_telemetry
    assert "wifiWebSocket.text(wifiWebSocketClientId," not in web_telemetry
    assert "wifiWebSocket.binary(wifiWebSocketClientId," not in web_telemetry


def test_websocket_supports_limited_multi_client():
    """v1.7.26：WebSocket 曲线通道应支持有限并发客户端（默认 2），避免第二个
    浏览器标签被强制关闭后回退到 HTTP 轮询，造成曲线卡顿。"""

    types_h = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h").read_text(encoding="utf-8")
    telemetry_cpp = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebTelemetry.cpp").read_text(encoding="utf-8")

    assert "WIFI_WEB_SOCKET_MAX_CLIENTS" in types_h
    assert "cleanupClients(WIFI_WEB_SOCKET_MAX_CLIENTS)" in telemetry_cpp
    assert "wifiWebSocket.count() > WIFI_WEB_SOCKET_MAX_CLIENTS" in telemetry_cpp


def test_firmware_version_is_current_and_changelog_is_ordered():
    build_info = BUILD_INFO.read_text(encoding="utf-8")
    changelog = CHANGELOG.read_text(encoding="utf-8")

    assert '#define MUS4_FIRMWARE_VERSION "v1.8.44"' in build_info
    assert "v1.8.44" in changelog
    # 注意：v1.8.43 被有意跳过（其它会话基于旧基点的构建正在车上运行、未以该形态回本仓库）
    assert "v1.8.42" in changelog
    assert "v1.8.41" in changelog
    assert "v1.8.39" in changelog
    assert "v1.8.38" in changelog
    assert "v1.8.37" in changelog
    assert "v1.8.36" in changelog
    assert "v1.8.35" in changelog
    assert "v1.8.33" in changelog
    assert "v1.8.32" in changelog
    assert "v1.8.31" in changelog
    assert "v1.8.30" in changelog
    assert "v1.8.29" in changelog
    assert "v1.8.28" in changelog
    assert "v1.8.27" in changelog
    assert "v1.8.26" in changelog
    assert "v1.8.25" in changelog
    assert "v1.8.24" in changelog
    assert "v1.8.23" in changelog
    assert "v1.8.22" in changelog
    assert "v1.8.20" in changelog
    assert "v1.8.19" in changelog
    assert "v1.8.18" in changelog
    assert "v1.8.17" in changelog
    assert "v1.8.16" in changelog
    assert "v1.8.15" in changelog
    assert "v1.8.14" in changelog
    assert "v1.8.13" in changelog
    assert "v1.8.12" in changelog
    assert "v1.8.11" in changelog
    assert "v1.8.8" in changelog
    assert "v1.8.7" in changelog
    assert "v1.8.6" in changelog
    assert "v1.8.4" in changelog
    assert "v1.8.3" in changelog
    assert "v1.8.2" in changelog
    assert "v1.8.1" in changelog
    assert "v1.8.0" in changelog
    assert "v1.7.99" in changelog
    assert "v1.7.98" in changelog
    assert "v1.7.97" in changelog
    assert "v1.7.96" in changelog
    assert "v1.7.95" in changelog
    assert "v1.7.94" in changelog
    assert "v1.7.93" in changelog
    assert "v1.7.92" in changelog
    assert "v1.7.91" in changelog
    assert "v1.7.90" in changelog
    assert "v1.7.89" in changelog
    assert "v1.7.88" in changelog
    assert "v1.7.87" in changelog
    assert "v1.7.86" in changelog
    assert "v1.7.85" in changelog
    assert "v1.7.84" in changelog
    assert "v1.7.83" in changelog
    assert "v1.7.82" in changelog
    assert "v1.7.80" in changelog
    assert "v1.7.79" in changelog
    assert "v1.7.78" in changelog
    assert "v1.7.77" in changelog
    assert "v1.7.76" in changelog
    assert "v1.7.75" in changelog
    assert "v1.7.74" in changelog
    assert "v1.7.73" in changelog
    # 条目顺序按日期+版本标题行比较（条目正文允许交叉引用其它版本号，不受影响）
    assert changelog.index("## 2026-08-22 v1.8.44") < changelog.index("## 2026-08-22 v1.8.42")
    assert changelog.index("## 2026-08-22 v1.8.42") < changelog.index("## 2026-08-22 v1.8.41")
    assert changelog.index("## 2026-08-22 v1.8.41") < changelog.index("## 2026-08-22 v1.8.39")
    assert changelog.index("## 2026-08-22 v1.8.39") < changelog.index("## 2026-08-21 v1.8.38")
    assert changelog.index("## 2026-08-21 v1.8.38") < changelog.index("## 2026-08-21 v1.8.37")
    assert changelog.index("## 2026-08-21 v1.8.37") < changelog.index("## 2026-08-21 v1.8.36")
    assert changelog.index("## 2026-08-21 v1.8.36") < changelog.index("## 2026-08-21 v1.8.35")
    assert changelog.index("## 2026-08-21 v1.8.35") < changelog.index("## 2026-08-21 v1.8.34")
    assert changelog.index("## 2026-08-21 v1.8.34") < changelog.index("## 2026-08-21 v1.8.33")
    assert changelog.index("## 2026-08-21 v1.8.33") < changelog.index("## 2026-08-21 v1.8.32")
    assert changelog.index("## 2026-08-21 v1.8.32") < changelog.index("## 2026-08-21 v1.8.31")
    assert changelog.index("## 2026-08-21 v1.8.31") < changelog.index("## 2026-08-21 v1.8.30")
    assert changelog.index("## 2026-08-21 v1.8.30") < changelog.index("## 2026-08-21 v1.8.29")
    assert changelog.index("## 2026-08-21 v1.8.29") < changelog.index("## 2026-08-21 v1.8.28")
    assert changelog.index("## 2026-08-21 v1.8.28") < changelog.index("## 2026-08-20 v1.8.27")
    assert changelog.index("## 2026-08-20 v1.8.27") < changelog.index("## 2026-08-20 v1.8.26")
    assert changelog.index("## 2026-08-20 v1.8.26") < changelog.index("## 2026-08-20 v1.8.25")
    assert changelog.index("## 2026-08-20 v1.8.25") < changelog.index("## 2026-08-20 v1.8.24")
    assert changelog.index("## 2026-08-20 v1.8.24") < changelog.index("## 2026-08-20 v1.8.23")
    assert changelog.index("## 2026-08-20 v1.8.23") < changelog.index("## 2026-08-20 v1.8.22")
    assert changelog.index("## 2026-08-20 v1.8.22") < changelog.index("## 2026-08-19 v1.8.20")
    assert changelog.index("## 2026-08-19 v1.8.20") < changelog.index("## 2026-08-19 v1.8.19")
    assert changelog.index("## 2026-08-19 v1.8.19") < changelog.index("## 2026-08-19 v1.8.18")
    assert changelog.index("## 2026-08-19 v1.8.18") < changelog.index("## 2026-08-19 v1.8.17")
    assert changelog.index("## 2026-08-19 v1.8.17") < changelog.index("## 2026-08-19 v1.8.16")
    assert changelog.index("## 2026-08-19 v1.8.16") < changelog.index("## 2026-08-19 v1.8.15")
    assert changelog.index("## 2026-08-19 v1.8.15") < changelog.index("## 2026-08-19 v1.8.14")
    assert changelog.index("## 2026-08-19 v1.8.14") < changelog.index("## 2026-08-19 v1.8.13")
    assert changelog.index("## 2026-08-19 v1.8.13") < changelog.index("## 2026-08-18 v1.8.12")
    assert changelog.index("## 2026-08-18 v1.8.12") < changelog.index("## 2026-08-18 v1.8.11")
    assert changelog.index("## 2026-08-18 v1.8.7") < changelog.index("## 2026-08-17 v1.8.6")
    assert changelog.index("## 2026-08-17 v1.8.6") < changelog.index("## 2026-08-17 v1.8.5")
    assert changelog.index("## 2026-08-17 v1.8.5") < changelog.index("## 2026-08-17 v1.8.4")
    assert changelog.index("## 2026-08-17 v1.8.4") < changelog.index("## 2026-08-17 v1.8.3")
    assert changelog.index("## 2026-08-17 v1.8.3") < changelog.index("## 2026-08-16 v1.8.2")
    assert changelog.index("## 2026-08-16 v1.8.2") < changelog.index("## 2026-08-16 v1.8.1")
    assert changelog.index("## 2026-08-16 v1.8.1") < changelog.index("## 2026-08-16 v1.8.0")
    assert changelog.index("## 2026-08-16 v1.8.0") < changelog.index("## 2026-08-16 v1.7.99")
    assert changelog.index("## 2026-08-16 v1.7.99") < changelog.index("## 2026-08-16 v1.7.98")
    assert changelog.index("## 2026-08-16 v1.7.98") < changelog.index("## 2026-08-16 v1.7.97")
    assert changelog.index("## 2026-08-16 v1.7.97") < changelog.index("## 2026-08-16 v1.7.96")
    assert changelog.index("## 2026-08-16 v1.7.96") < changelog.index("## 2026-08-15 v1.7.95")
    assert changelog.index("## 2026-08-15 v1.7.95") < changelog.index("## 2026-08-15 v1.7.94")
    assert changelog.index("## 2026-08-15 v1.7.94") < changelog.index("## 2026-08-15 v1.7.93")
    assert changelog.index("## 2026-08-15 v1.7.93") < changelog.index("## 2026-08-15 v1.7.92")
    assert changelog.index("## 2026-08-15 v1.7.92") < changelog.index("## 2026-08-15 v1.7.91")
    assert changelog.index("## 2026-08-15 v1.7.91") < changelog.index("## 2026-08-15 v1.7.90")
    assert changelog.index("## 2026-08-15 v1.7.90") < changelog.index("## 2026-08-15 v1.7.89")
    assert changelog.index("## 2026-08-15 v1.7.89") < changelog.index("## 2026-08-15 v1.7.88")
    assert changelog.index("## 2026-08-15 v1.7.88") < changelog.index("## 2026-08-15 v1.7.87")
    assert changelog.index("## 2026-08-15 v1.7.87") < changelog.index("## 2026-08-15 v1.7.86")
    assert changelog.index("## 2026-08-15 v1.7.86") < changelog.index("## 2026-08-15 v1.7.85")
    assert changelog.index("## 2026-08-15 v1.7.85") < changelog.index("## 2026-08-15 v1.7.84")
    assert changelog.index("## 2026-08-15 v1.7.84") < changelog.index("## 2026-08-15 v1.7.83")
    assert changelog.index("## 2026-08-15 v1.7.83") < changelog.index("## 2026-08-15 v1.7.82")
    assert changelog.index("## 2026-08-15 v1.7.82") < changelog.index("## 2026-08-15 v1.7.80")
    assert changelog.index("## 2026-08-15 v1.7.80") < changelog.index("## 2026-08-15 v1.7.79")
    assert changelog.index("## 2026-08-15 v1.7.79") < changelog.index("## 2026-08-15 v1.7.78")
    assert changelog.index("## 2026-08-15 v1.7.78") < changelog.index("## 2026-08-15 v1.7.77")
    assert changelog.index("## 2026-08-15 v1.7.77") < changelog.index("## 2026-08-15 v1.7.76")
    assert changelog.index("## 2026-08-15 v1.7.76") < changelog.index("## 2026-08-15 v1.7.75")


def test_mode_command_channel_and_arbitration():
    """Issue #111：固件支持通过命令设置车控模式，并与遥控器切换双向兼容。

    - ControlMixer 新增 setCarModeCommand + 后到者生效仲裁（hostMode/lastRcMode）。
    - CommandDispatcher 新增 MODE 命令（ACK:MODE / NACK:MODE_INVALID）。
    - WirelessConsole 放行模式命令（需认证，Park 下也允许）。
    - M:P 遥测帧提升为所有模式发送。
    """

    control_mixer_h = (PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "ControlMixer.h").read_text(encoding="utf-8")
    control_mixer_cpp = (PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "ControlMixer.cpp").read_text(encoding="utf-8")
    dispatcher_cpp = (PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "CommandDispatcher.cpp").read_text(encoding="utf-8")
    wireless_cpp = (PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "WirelessConsole.cpp").read_text(encoding="utf-8")
    sketch = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "bool setCarModeCommand(int mode)" in control_mixer_h
    assert "bool setCarModeCommand(int mode)" in control_mixer_cpp
    assert "hostMode" in control_mixer_cpp
    assert "lastRcMode" in control_mixer_cpp

    assert r'out.printf("ACK:MODE %d\n", m)' in dispatcher_cpp
    assert 'out.println("NACK:MODE_INVALID")' in dispatcher_cpp
    assert "setCarModeCommand(m)" in dispatcher_cpp

    assert "bool isWirelessModeCommand(const String& line)" in wireless_cpp
    assert "if (isWirelessModeCommand(line)) return true" in wireless_cpp

    assert r'"M%d:P%d\n"' in sketch
    assert "模式帧 (所有模式" in sketch


def test_host_ip_report_channel():
    """v1.7.39：Serial2 新增 HOSTIP|<ipv4> 上行帧，ESP32 存运行时状态并在
    /api/status 输出 host_ip/host_ip_age_s，Web Console Network 卡片新增 HOST 分页。"""

    sketch = MUS4_SKETCH.read_text(encoding="utf-8")
    server = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp").read_text(encoding="utf-8")
    assets = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(encoding="utf-8")

    # 固件侧：HOSTIP| 帧处理 + IPv4 严格校验
    assert 'strncmp(line, "HOSTIP|", 7)' in sketch
    assert "isValidIpv4Text(line + 7)" in sketch
    assert "hostReportedIpMs = millis();" in sketch

    # 状态输出：host_ip / host_ip_age_s 字段与运行时存储
    assert 'String hostReportedIp = "";' in server
    assert "host_ip=%s host_ip_age_s=%lu" in server
    assert "hostReportedIp.c_str()" in server

    # Web Console：Network 卡片 HOST 分页
    host_tab = '<button id="networkHostTab" type="button" onclick="setNetworkTab(' + chr(39) + 'host' + chr(39) + ')">HOST</button>'
    assert host_tab in assets
    assert "selected==='host'" in assets
    assert "s.host_ip||''" in assets

    # v1.7.41：HOST 分页隐藏齿轮设置按钮（AP/STA 分页保留，HOST 页无网络设置）
    assert '<button id="networkGear" class="gear"' in assets
    assert "networkGear.style.display=selected==='host'?'none':''" in assets


def test_apply_wifi_sta_credentials_restores_ap_before_begin():
    """v1.7.19 起：发起 STA 连接前必须确保 AP_STA + AP 服务在线，否则 STA 在
    STA-only 状态下保存错误密码后会彻底失联（既无 AP 也无 STA）。"""

    source = firmware_source_text()
    apply_body = re.search(
        r"(?:static )?void applyWifiStaCredentials\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")

    assert "WiFi.getMode() != WIFI_AP_STA" in apply_body
    assert "WiFi.mode(WIFI_AP_STA)" in apply_body
    assert "WiFi.softAPIP() == IPAddress(0, 0, 0, 0)" in apply_body
    assert 'startWifiApServices("AP restored for STA apply")' in apply_body
    assert apply_body.index("WiFi.mode(WIFI_AP_STA)") < apply_body.index("WiFi.begin")
    assert apply_body.index("startWifiApServices") < apply_body.index("WiFi.begin")
    # v1.7.21：mode 切换后需要一小段时间让 STA netif 完成重建，否则 WiFi.begin()
    # 拿不到信道导致 timeout。
    assert "delay(50)" in apply_body
    assert apply_body.index("WiFi.mode(WIFI_AP_STA)") < apply_body.index("delay(50)") < apply_body.index("WiFi.begin")


def test_web_console_serial_log_display_is_limited_to_20_lines():
    source = firmware_source_text()

    assert "#serialPanel .log{flex:1 1 auto;min-height:calc(5 * 1.35em + 16px);max-height:calc(20 * 1.35em + 16px)}" in source
    assert "#serialPanel .log{height:" not in source


def test_web_console_has_multi_source_log_selector_and_megabyte_buffers():
    source = firmware_source_text()

    assert 'id="cmdTarget"' in source
    assert 'id="logSource"' not in source
    assert '<option value="web">Web</option>' in source
    assert '<option value="serial">Serial</option>' in source
    assert '<option value="serial1">Serial1</option>' not in source
    # Serial（上位机终端）排第一位，是默认目标
    assert source.index('<option value="serial">Serial</option>') < \
        source.index('<option value="web">Web</option>')
    assert "const LOG_SOURCE_MAX_BYTES=1024*1024" in source
    assert "const LOG_DISPLAY_MAX_BYTES=16000" in source
    assert "sourceBuffers={web:'',serial:'',serial1:''}" in source
    assert "function appendLogLine(" in source
    assert "function switchLogSource(" in source
    assert "function trimLogDisplay(" not in source
    assert "cmdTarget.addEventListener('change'" in source
    assert "function canonicalLogSource(" in source
    assert "if(src==='serial'||src==='serial1')return src;return 'web';" in source
    assert "typeof e.data==='string'" in source
    assert "j.type==='log'" in source
    assert "appendLogLine('['+j.t+']['+j.src+'] '+j.line,j.src)" in source


def test_web_console_serial_option_is_host_terminal_with_persistent_default():
    """cmdTarget 的 Serial 选项 = 上位机终端（xterm.js iframe），浏览器式标签页。

    选择 Serial 时日志区切换为 iframe 嵌入的上位机终端页面
    （http://<host_ip>:8090/terminal，由上位机 Launcher 服务提供）；
    目标选择持久化到 localStorage，下次打开页面时恢复，默认 Serial。
    终端数据走局域网 WebSocket（不走 115200 串口，带宽不足以跑 TUI）。
    ➕ 位于标签条右端（v1.7.88 起，垃圾桶按钮移除），每点一次新增一个终端标签页
    （独立 iframe/PTY 会话）；每个标签左侧的 × 关闭对应终端（v1.7.87）。
    """
    source = firmware_source_text()

    # 终端视图容器与标签条（iframe 改为动态创建，不再有静态 #terminalFrame）
    assert 'id="terminalWrap"' in source
    assert 'id="terminalHint"' in source
    assert 'id="termTabs"' in source
    assert 'id="terminalFrame"' not in source
    # 终端 URL 由上位机 HOSTIP 上报自动发现（_launcherIp），不硬编码
    assert "function terminalUrl(){return 'http://'+_launcherIp+':8090/terminal';}" in source
    # 选择持久化：localStorage 键 + 写入/读取 + 默认 serial + 启动时恢复
    assert "const CMD_TARGET_KEY='donkeydrifter.ui.cmdTarget'" in source
    assert "localStorage.setItem(CMD_TARGET_KEY,src)" in source
    assert "localStorage.getItem(CMD_TARGET_KEY)" in source
    assert "applyCmdTarget(saved||'serial',false)" in source
    assert "restoreCmdTarget();" in source
    # 切换目标时显示终端与标签条、隐藏日志区；Serial 模式隐藏暂停/发送/输入框，
    # 显示"新建终端"加号按钮（v1.7.88 起位于标签条右端）；切回 Web 恢复
    # 日志视图与完整工具行；首次进入 Serial 自动建第一个
    # 标签（termInited），用户杀光后切回不自动重建
    assert "function applyCmdTarget(src,save)" in source
    assert "newTermBtn.style.display=term?'':'none';" in source
    assert "termTabs.style.display=term?'flex':'none';" in source
    assert "pauseBtn.style.display=term?'none':'';" in source
    assert "sendBtn.style.display=term?'none':'';" in source
    assert "cmd.style.display=term?'none':'';" in source
    assert "if(term){if(!termInited)addTerminalTab();fitTermTabLabels()}else switchLogSource('web');" in source
    assert "cmdTarget.addEventListener('change',e=>{applyCmdTarget(e.target.value)});" in source
    # 标签页管理：动态 iframe（保留 #57 白边修复 scrolling="no"）+ 探活后设 src，
    # 每个 iframe 独立 PTY 会话；点标签只切换显示（其余 display:none 保活）；
    # v1.7.88 起垃圾桶按钮移除（Serial 用标签上的 × 关闭，Web 清空日志按钮一并移除）；
    # v1.7.93 起 ➕ 移出标签滚动区、作为其右邻兄弟钉在行尾（始终靠右），新标签 appendChild 进滚动区
    assert 'id="newTermBtn"' in source
    assert 'id="clearBtn"' not in source
    assert "onClearBtn" not in source
    assert "killActiveTerminalTab" not in source
    assert "function addTerminalTab()" in source
    assert "termTabs.appendChild(b);" in source
    assert "document.createElement('iframe')" in source
    assert "f.setAttribute('scrolling','no')" in source
    assert "function selectTerminalTab(id)" in source
    # 标签按位置连续编号（v1.7.80）：新建用 termList.length+1，杀标签后剩余标签重编号；
    # 标签文字放在 .termTabLabel 子 span（v1.7.87 起）；v1.7.93 起默认名智能缩写（放得下显示"终端 N"、放不下缩写为 N）；
    # #90 修复：fitTermTabLabels 每次先按长名统一测量、溢出才缩写（原按改名前布局判 packed，
    # 临界宽度下长名↔短名振荡，用户看到长名+溢出"没生效"）
    assert "l.textContent=t('terminal.tab')+' '+(termList.length+1);" in source
    assert "function fitTermTabLabels()" in source
    assert "termTabs.scrollWidth>termTabs.clientWidth" in source
    assert "if(!x.name)x.l.textContent=t('terminal.tab')+' '+(j+1)" in source
    assert "if(termTabs.scrollWidth>termTabs.clientWidth)termList.forEach((x,j)=>{if(!x.name)x.l.textContent=''+(j+1)})" in source
    assert "packed?" not in source
    assert "window.addEventListener('resize',fitTermTabLabels)" in source
    # × 单独关闭钮（v1.7.87）：每个标签左侧一个 ×，按 id 杀对应终端，
    # 点击 stopPropagation 不触发标签切换
    assert "c.className='termTabClose'" in source
    assert "c.onclick=e=>{e.stopPropagation();killTerminalTab(id)};" in source
    assert "function killTerminalTab(id)" in source
    # 保底一个终端（v1.7.97）：仅剩一个标签时 × 关闭钮隐藏（updateTermTabClose），
    # killTerminalTab 入口守卫拒绝关闭最后一个；term 对象持 × 元素引用 c 以控制显隐
    assert "const term={id:id,f:f,b:b,l:l,c:c,name:null,state:'loading'}" in source
    assert "function updateTermTabClose(){const hide=termList.length<=1;" in source
    assert "x.c.style.display=hide?'none':''" in source
    assert "function killTerminalTab(id){if(termList.length<=1)return;" in source
    assert "fitTermTabLabels();updateTermTabClose();if(termList.length===0)" in source
    # 标签名跟随终端内输入命令（v1.7.90）：上位机终端页把每行输入的首词
    # postMessage 给父页，父页按 e.source 匹配 iframe 改名；重编号时自定义名优先；
    # v1.7.96 起首次命名后锁定：cur.name 非空即忽略后续上报
    #（如已命名 kimi，再输入 /web 标签仍保持 kimi 不变）
    assert "window.addEventListener('message',e=>{" in source
    assert "d.type!=='donkeydrifter.term.name'" in source
    assert "x.f.contentWindow===e.source" in source
    assert "if(!cur||cur.name)return;" in source
    assert "cur.name=d.name;cur.l.textContent=d.name;" in source
    assert ".termTabClose{" in source
    assert "I18N.zh['terminal.closeTab']" in source
    assert "I18N.en['terminal.closeTab']" in source
    # 标签浅色皮肤（v1.7.87）：未选中白底深字、选中蓝字浅蓝底；终端画布保持深色
    assert 'html[data-theme="light"] .termTab{' in source
    assert 'html[data-theme="light"] .termTab.active{' in source
    assert 'html[data-theme="light"] .termTabClose:hover{' in source
    # 新开浏览器标签的旧逻辑已删除
    assert "openNewTerminal" not in source
    assert "window.open(terminalUrl" not in source
    # .termFrame CSS 保留 #57 白边修复属性（标识符由 #terminalFrame 改为 .termFrame）
    assert ".termFrame{display:block;flex:1 1 auto;width:100%;min-height:0;border:0;border-radius:6px;background:#101318}" in source
    # 终端窗口全屏按钮（v1.7.99）：右下角图标按钮，UI/行为完全对齐 chartFullscreenBtn；
    # 按钮居 #terminalWrap DOM 末尾（#terminalHint 之后），压在 insertBefore 插入的 iframe 上；
    # #terminalWrap 加 position:relative 作定位父级；:fullscreen 抵消原 height/min-height/max-height 的 calc 钳制，
    # 并去掉 padding/border-radius、统一深色背景，消除终端全屏四周白边（不再给亮色主题单独设全屏背景）
    assert 'id="termFullscreenBtn"' in source
    assert 'onclick="toggleTerminalFullscreen()"' in source
    assert '#termFullscreenBtn{position:absolute;right:8px;bottom:8px;z-index:2}' in source
    assert '#terminalWrap:fullscreen{background:#101318;height:auto;min-height:0;max-height:none;padding:0;border-radius:0}' in source
    assert 'html[data-theme="light"] #terminalWrap:fullscreen' not in source
    assert 'function toggleTerminalFullscreen(){if(document.fullscreenElement===terminalWrap)document.exitFullscreen();else terminalWrap.requestFullscreen()}' in source
    assert "tf.innerHTML=document.fullscreenElement===terminalWrap?ICON_FULLSCREEN_EXIT:ICON_FULLSCREEN" in source
    # 标签条样式：横向滚动 + 选中态高亮
    assert "#termTabs{display:none;align-items:center;gap:4px;overflow-x:auto" in source
    assert ".termTab.active{" in source
    # v1.7.93：标签条隐藏滚动条（溢出时行高与布局不变、窗口比例不被修改）；➕ 钉在行尾右端
    assert "scrollbar-width:none" in source
    assert "#termTabs::-webkit-scrollbar{display:none}" in source
    assert "#newTermBtn{width:22px;height:22px;flex:0 0 auto}" in source
    assert "#termTabs .iconButton" not in source
    # i18n：新建/标签/关闭/空态词条 + 加载/失败提示（中英双语）；terminal.tab 保留（放得下时默认名"终端 N"）
    assert "I18N.zh['terminal.new']" in source
    assert "I18N.en['terminal.new']" in source
    assert "I18N.zh['terminal.tab']" in source
    assert "I18N.en['terminal.tab']" in source
    # v1.7.88 起 terminal.kill 词条随垃圾桶按钮一并移除
    assert "terminal.kill" not in source
    assert "I18N.zh['terminal.empty']" in source
    assert "I18N.en['terminal.empty']" in source
    assert "I18N.zh['terminal.loading']" in source
    assert "I18N.en['terminal.loading']" in source
    assert "I18N.zh['terminal.unreachable']" in source
    assert "I18N.en['terminal.unreachable']" in source
    # #89 修复：终端探测失败后周期重试（scheduleTermRetry 每 4s 刷新 _launcherIp 再重探），
    # host_ip 年龄>90s 视为过期并在提示中标注；探测逻辑抽为可重入的 probeTerminal
    assert "function probeTerminal(term)" in source
    assert "selectTerminalTab(id);fitTermTabLabels();updateTermTabClose();_fetchLauncherIp().then(()=>probeTerminal(term));}" in source
    assert "function scheduleTermRetry()" in source
    assert "_termRetryTimer=setInterval" in source
    assert "if(!termList.some(x=>x.state==='fail')){clearInterval(_termRetryTimer)" in source
    assert "await _fetchLauncherIp();termList.forEach(x=>{if(x.state==='fail')probeTerminal(x)})" in source
    assert "_applyLauncherStatus" in source
    assert "host_ip_age_s=(\\d+)" in source
    assert "_launcherIpAge" in source
    assert "function termFailHint()" in source
    assert "_launcherIpAge>90" in source
    assert "t('terminal.staleIp')" in source
    assert "I18N.zh['terminal.staleIp']" in source
    assert "I18N.en['terminal.staleIp']" in source
    # 首探等待真实 IP：addTerminalTab 探测前先 await _fetchLauncherIp()（.then 链式），
    # 消灭页面加载时用默认回退 IP 首探必败的窗口（DC 终端一直「正在连接」的根因）；
    # 从未收到上报（age=-1）时 termFailHint 明确提示 IP 未知，不显示误导性回退地址
    assert "_fetchLauncherIp().then(()=>probeTerminal(term))" in source
    assert "if(_launcherIpAge===-1)return t('terminal.unknownIp');" in source
    assert "I18N.zh['terminal.unknownIp']" in source
    assert "I18N.en['terminal.unknownIp']" in source
    # #101 修复：loading 态加超时兜底——no-cors fetch 因 IP 不可达长时间挂起时，
    # 未落定一律按 fail 处理（复用失败提示与 4s 自动重试），不再无限期停在「正在连接」；
    # AbortController 主动中止；探测序号防旧探测迟到结果覆盖新探测状态；
    # 超时由 10s 缩短为 5s，配合 4s 重试更快收敛
    assert "term._probe=(term._probe||0)+1" in source
    assert "const seq=term._probe,ctrl=new AbortController(),timer=setTimeout(()=>ctrl.abort(),5000)" in source
    assert "signal:ctrl.signal" in source
    assert "if(seq!==term._probe)return" in source
    assert ".then(()=>done('ok')).catch(()=>done('fail'))" in source


def test_web_console_screen_saver_activates_after_60_seconds():
    source = firmware_source_text()

    assert "now-parkLockedAt>=60000&&range<10" in source
    assert "now-parkLockedAt>=3000&&range<10" not in source


def test_web_console_keeps_original_ui_and_direct_curve_path():
    source = firmware_source_text()

    assert "Drifter Console" in source
    assert "DonkeyDrift Console" not in source
    assert "Donkey Console" not in source
    assert "MUS4 Web Console" not in source
    assert "MUS4 Compact Console" not in source
    assert "pendingPoints.push" not in source
    assert "const interp={...prev}" not in source
    assert "chartLatencyMs=160" not in source
    assert "ws.send('ping')" not in source
    assert "\"pong\"" not in source


def test_web_console_pages_share_embedded_png_favicon():
    """四个 Web 页面（Console/Judge/Drift/OTA）统一引用 /favicon.png，
    由 WebConsoleServer 以嵌入 PNG（WebConsoleFavicon.h）提供。"""
    assets = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(encoding="utf-8")
    server = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp").read_text(encoding="utf-8")
    favicon = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleFavicon.h").read_text(encoding="utf-8")

    icon_link = '<link rel="icon" type="image/png" href="/favicon.png">'
    for title in ("Drifter Console", "Drift Judge", "Drift Assist Tuning", "MUS4 OTA Update"):
        idx = assets.find(f"<title>{title}</title>")
        assert idx != -1, title
        # favicon <link> 必须紧跟在同页 <title> 之前，不能跨页面误匹配
        assert assets[max(0, idx - 200):idx].count(icon_link) == 1, title

    assert 'wifiWebServer.on("/favicon.png", HTTP_GET, handleWifiWebFavicon);' in server
    assert '"image/png"' in server
    assert "WEB_CONSOLE_FAVICON_PNG" in server

    # PNG 文件头魔数：‰PNG
    assert "0x89, 0x50, 0x4E, 0x47" in favicon
    assert "WEB_CONSOLE_FAVICON_PNG_LEN" in favicon


def test_web_console_has_help_floating_modal():
    source = firmware_source_text()

    assert 'id="helpFab"' in source
    assert 'id="helpOverlay"' in source
    assert 'id="helpModal"' in source
    assert 'class="helpClose"' in source
    assert "function openHelpModal()" in source
    assert "function closeHelpModal()" in source
    assert "状态卡片：查看模式、Park、OTA、连接状态" in source
    assert "Network：查看 AP/STA IP，配置 Wi-Fi" in source
    assert "Diagnostics：运行测试、回归、维护命令" in source
    assert "Serial Log：查看设备日志" in source
    assert "Tub JSON：记录并下载遥测样本" in source
    assert "OTA / DEV：固件更新与开发模式开关" in source
    assert "Status Cards: view mode, Park, OTA, and connection status" in source


def test_web_console_help_modal_mirrors_donkeydrifter_layout():
    source = firmware_source_text()

    # 帮助弹窗完全模仿 DonkeyDrifter：右下角锚定 + 蓝边渐变面板
    assert '.helpModal{position:fixed;right:18px;bottom:74px;width:min(340px,calc(100vw - 36px))' in source
    assert 'background:linear-gradient(135deg,#1c2430,#121821);border:1px solid #5cc8ff;border-radius:14px;padding:14px' in source
    # 幽灵关闭按钮（与 DonkeyDrifter 一致：透明底 + zinc-400 ×，hover zinc-800）
    assert '.helpClose{min-width:0;width:28px;height:28px;padding:0;border:none;border-radius:50%;background:transparent;color:#a1a1aa' in source
    assert '.helpClose:hover{background:#27272a;color:#f4f4f5}' in source
    # 功能分类 + 小标题（双语 i18n，uppercase 灰色小标题样式）
    assert 'class="helpSection"' in source
    assert '.helpSection h3{margin:0 0 8px;font-size:12px;font-weight:500;text-transform:uppercase;letter-spacing:.05em;color:#8fa1b5}' in source
    assert 'data-i18n="help.groupStatus"' in source
    assert 'data-i18n="help.groupNetwork"' in source
    assert 'data-i18n="help.groupData"' in source
    assert "'help.groupStatus':'状态与日志'" in source
    assert "'help.groupStatus':'Status & Logs'" in source
    assert "'help.groupNetwork':'Network & Diagnostics'" in source
    assert "'help.groupData':'Data & Maintenance'" in source
    # 功能说明内容条目保持不变
    assert 'data-i18n="help.statusCards"' in source
    assert "状态卡片：查看模式、Park、OTA、连接状态" in source


def test_web_console_has_collapsed_glow_fab_with_radial_actions():
    """FAB 展开组（v1.8.3，Issue #92）：语言入口移到顶栏单按钮 #langToggle 后，
    langFab/langMenu 弹出菜单入口及其 CSS/JS 一并移除，展开组只剩 helpFab；
    fabActions 容器与全局滚动/触摸收起监听保留。"""

    source = firmware_source_text()

    assert 'id="fabToggle"' in source
    assert 'class="fabToggle"' in source
    assert 'id="fabActions"' in source
    assert 'class="fabActions"' in source
    assert 'id="helpFab"' in source
    assert "?" in source
    assert "toggleFabActions" in source
    assert "collapseFabActions" in source
    assert "fabActions.classList.toggle('show')" in source
    assert "fabActions.classList.remove('show')" in source
    assert "window.addEventListener('scroll',collapseFabActions" in source
    assert "window.addEventListener('touchmove',collapseFabActions" in source
    assert ".fabToggle{position:fixed;right:24px;bottom:24px;width:18px;height:18px" in source
    assert "box-shadow:0 0 18px #5cc8ff,0 0 36px rgba(92,200,255,.55)" in source
    assert ".fabToggle:hover,.fabToggle:focus-visible,.fabToggle:active{background:#8bdcff;border-color:#8bdcff;" in source
    assert ".fabActions.show .helpFab" in source
    assert source.index('id="fabToggle"') < source.index('id="fabActions"') < source.index('id="helpFab"')
    # Issue #92：语言 FAB/弹出菜单死代码不残留
    assert "langFab" not in source
    assert "langMenu" not in source
    assert "🌐" not in source
    assert "toggleLanguageMenu" not in source
    assert "closeLanguageMenu" not in source


def test_web_console_language_selection_uses_local_storage_and_i18n_dictionary():
    source = firmware_source_text()

    assert "mus4.ui.lang" in source
    assert "localStorage.getItem" in source
    assert "localStorage.setItem" in source
    assert "const I18N" in source
    assert "zh:" in source
    assert "en:" in source
    assert "function applyLanguage" in source
    assert "function setLanguage" in source
    assert "document.documentElement.lang" in source
    assert "return lang==='en'?'en':'zh'" in source


def test_web_console_reads_dd_lang_url_param():
    """DD 内嵌 DC 时经 iframe src 的 `?lang=` 传入语言；DC 需优先读取
    该参数，跨源 localStorage / 车端 /api/language 各自独立时仍与 DD
    语言一致，避免“DD 已英文、DC 仍中文”。"""
    source = firmware_source_text()

    assert "readUrlLanguage" in source
    assert "window.location.search" in source
    assert "lang=(zh|en)" in source
    assert "let lang=readUrlLanguage()" in source


def test_web_console_static_core_copy_is_marked_for_i18n():
    source = firmware_source_text()

    assert "data-i18n=" in source
    assert "data-i18n-placeholder=" in source
    assert "data-i18n-aria=" in source
    assert 'data-i18n="state.mode"' in source
    assert 'data-i18n="state.park"' in source
    assert 'data-i18n="state.drift"' in source
    assert 'data-i18n="state.voltage"' in source
    assert 'data-i18n="state.network"' in source
    assert 'data-i18n="help.title"' in source


def test_web_console_english_dictionary_covers_core_interface_copy():
    source = firmware_source_text()

    assert "Language" in source
    assert "Status Cards" in source
    assert "Network" in source
    assert "Diagnostics" in source
    assert "Serial Log" in source
    assert "Tub JSON" in source
    assert "OTA / DEV" in source
    assert "Send" in source
    assert "Clear" in source
    assert "Pause" in source
    assert "Resume" in source
    assert "Fullscreen" in source
    assert "Split" in source
    assert "Copy IP" in source
    assert "STA connection failed" in source


def test_web_console_dynamic_visible_copy_uses_current_language():
    source = firmware_source_text()

    assert "function t(" in source
    assert "function refreshDynamicLabels" in source
    assert "pauseBtn" in source
    assert "chartBtn" in source
    assert "chartFullscreenBtn" in source
    assert "p.innerHTML=logPaused?ICON_PLAY:ICON_PAUSE" in source
    assert "f.innerHTML=document.fullscreenElement===chartPanel?ICON_FULLSCREEN_EXIT:ICON_FULLSCREEN" in source
    assert "showToast(t('toast.copyFailed')" in source
    assert "explainCommandError" in source
    assert "PARK_REQUIRED" in source
    assert "AUTH_REQUIRED" in source
    assert "UNAUTHORIZED" in source


def test_diagnostic_code_is_not_built_by_default():
    source = firmware_source_text()

    assert re.search(r"^//\s*#define\s+ENABLE_DIAGNOSTIC_COMMANDS\b", source, re.MULTILINE)
    assert re.search(r"^//\s*#define\s+ENABLE_BOOT_STEERING_SELF_TEST\b", source, re.MULTILINE)



def test_firmware_config_centralizes_core_compile_time_defaults():
    config = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "FirmwareConfig.h").read_text(encoding="utf-8")
    sketch = MUS4_SKETCH.read_text(encoding="utf-8")

    assert '#include "FirmwareConfig.h"' in sketch
    assert re.search(r"^#define\s+ENABLE_WIFI_CONSOLE\b", config, re.MULTILINE)
    assert re.search(r"^#define\s+ENABLE_WIFI_WEBSOCKET_TELEMETRY\b", config, re.MULTILINE)
    assert "#define CH1_PIN 36" in config
    assert "#define THROTTLE_PIN 25" in config
    assert "#define RC_CHANNEL_COUNT 6" in config
    assert "#define CAR_MODE_MANUAL 0" in config
    assert "#define CAR_MODE_SEMI_AUTO 1" in config
    assert "#define CAR_MODE_FULL_AUTO 2" in config
    assert "#define PWM_FILTER_SIZE 5" in config
    assert "#define I2C_SPEED 400000L" in config
    assert "#define UI_UPDATE_INTERVAL 2" in config
    assert "#define WAVE_WIDTH 20" in config
    assert "#define WAVE_HEIGHT 6" in config
    assert "#define CAR_MODE_MANUAL" not in (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "SharedTypes.h").read_text(encoding="utf-8")
    assert "#define WAVE_WIDTH" not in (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "SharedTypes.h").read_text(encoding="utf-8")
    assert "#define ENABLE_WIFI_CONSOLE" not in sketch
    assert "#define CH1_PIN 36" not in sketch
    assert "#define PWM_FILTER_SIZE 5" not in sketch



def test_firmware_entrypoints_remain_in_main_sketch():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert len(re.findall(r"^void\s+setup\s*\(", source, re.MULTILINE)) == 1
    assert len(re.findall(r"^void\s+loop\s*\(", source, re.MULTILINE)) == 1



def test_rc_filter_helpers_remain_available_after_module_split():
    source = firmware_source_text()

    for symbol in [
        "uint16_t medianFilter(uint16_t* buf, int size)",
        "bool isAuxiliaryRcChannel(int ch)",
        "bool isPrimaryRcChannel(int ch)",
        "uint16_t smoothPrimaryPWM(int ch, uint16_t value, bool valid)",
        "uint16_t stabilizeAuxiliaryPWM(int ch, uint16_t value, bool valid)",
        "bool runFilterTests()",
        "Running Filter Tests...",
    ]:
        assert symbol in source


def test_command_parser_helpers_remain_available_after_module_split():
    source = firmware_source_text()

    for symbol in [
        "uint8_t parseHex2(const char* s)",
        "uint8_t calcChecksum(const char* s, int n)",
        "bool parsePilotCommandLine(const String& line, int* throttle, int* steering, int* seq)",
        "bool processLine(const String& line, int* throttle, int* steering, int* seq)",
        "bool parseAndValidateCommand(String cmd, int* throttle, int* steering)",
        "bool runUnitTests()",
        "20:-20:255",
    ]:
        assert symbol in source

    parser_source = (PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "CommandParser.cpp").read_text(encoding="utf-8")
    local_commands_source = (PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "LocalCommands.cpp").read_text(encoding="utf-8")
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")
    assert "bool runUnitTests()" in parser_source
    assert "static bool runUnitTests()" not in sketch_source
    assert "bool processLine(const String& line, int* throttle, int* steering, int* seq)" in local_commands_source
    assert "Filter Debug: %s" in local_commands_source
    assert "bool processLine(const String& line, int* throttle, int* steering, int* seq)" not in sketch_source


def test_sketch_drops_legacy_tui_dirty_rectangle_state_after_module_split():
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")

    for symbol in [
        "cursorDownN",
        "cursorUpN",
        "cursorRightN",
        "cursorLeftN",
        "lastModePrinted",
        "lastParkPrinted",
        "lastCh1",
        "lastOutTh",
        "lastSensorsPrint",
        "lastINAStr",
        "lastMPUStr",
        "lastWaveTh",
        "lastWaveSt",
        "forceRedraw = false",
    ]:
        assert symbol not in sketch_source


def test_command_dispatcher_replaces_command_line_macro_after_module_split():
    source = firmware_source_text()
    dispatcher_header = (PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "CommandDispatcher.h").read_text(encoding="utf-8")
    dispatcher_source = (PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "CommandDispatcher.cpp").read_text(encoding="utf-8")
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "bool dispatchCommandLine(const String& line, Print& out, SerialBuf& sb, bool pilotSilent = false)" in dispatcher_header
    assert "bool dispatchCommandLine(const String& line, Print& out, SerialBuf& sb, bool pilotSilent)" in dispatcher_source
    assert "extern ControlData pilot_data;" in dispatcher_source
    assert "struct struct_message" not in dispatcher_source
    assert "ControlData esp_now_data" in sketch_source
    assert "ControlData rc_data" in sketch_source
    assert "ControlData pilot_data" in sketch_source
    assert "ControlData car_output" in sketch_source
    assert "struct struct_message" not in sketch_source
    assert "dispatchCommandLine(line, out, sb, /*pilotSilent=*/true);" in source
    assert "dispatchCommandLine(String(sb.buf), ser, sb);" not in source
    assert "#define PROCESS_COMMAND_LINE" not in sketch_source
    assert "PROCESS_COMMAND_LINE" not in sketch_source

    for symbol in [
        "ACK:LOG_WEB",
        "ACK:LOG_SERIAL",
        "ACK:JOYSTICK_SAVED",
        "NACK:JOYSTICK_SAVE_FAILED",
        "NACK:JOYSTICK_INVALID_RANGE",
        "NACK:JOYSTICK_NOT_DONE",
        "ACK:JOYSTICK_RETRY",
        "ACK:JOYSTICK_ABORTED",
        "ACK:JOYSTICK_RESET",
        "ACK:DEPRECATED_USE_JOYSTICK_CAL",
        "ACK:DEPRECATED_USE_JOYSTICK_SAVE",
        "ACK:DEPRECATED_USE_JOYSTICK_RETRY",
        "ACK:DEPRECATED_USE_JOYSTICK_ABORT",
        "ACK:DEPRECATED_USE_JOYSTICK_RESET",
        "ACK:%d\\n",
        "NACK:%d\\n",
    ]:
        assert symbol in source


def test_serial_line_reader_is_split_from_sketch():
    source = firmware_source_text()
    reader_header = (PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "SerialLineReader.h").read_text(encoding="utf-8")
    reader_source = (PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "SerialLineReader.cpp").read_text(encoding="utf-8")
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "void readSerialBuf(HardwareSerial& ser, SerialBuf& sb)" in reader_header
    assert "void readSerialBuf(HardwareSerial& ser, SerialBuf& sb)" in reader_source
    assert "dispatchCommandLine(line, out, sb, /*pilotSilent=*/true);" in reader_source
    assert "if (c == '\\r') continue;" in reader_source
    assert "if (c == '\\n')" in reader_source
    assert "sb.overflow = true;" in reader_source
    assert "#include \"SerialLineReader.h\"" in sketch_source
    assert "readSerialBuf(Serial, serial0Buf);" in sketch_source
    assert "readSerialBuf(Serial1, serial1Buf);" in sketch_source
    assert "static void readSerialBuf" not in sketch_source
    assert "dispatchCommandLine(String(sb.buf), ser, sb);" not in reader_source
    assert "dispatchCommandLine(String(sb.buf), ser, sb);" not in sketch_source
    assert "#include \"SerialLineReader.h\"" in source


def test_serial1_telemetry_has_dedicated_web_log_buffer():
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")
    web_log_header = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebLogBuffer.h").read_text(encoding="utf-8")
    web_log_source = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebLogBuffer.cpp").read_text(encoding="utf-8")
    wifi_types = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h").read_text(encoding="utf-8")

    # Serial1 telemetry is only emitted in MANUAL mode and only logged to the
    # web console when the hardware telemetry line is also emitted.
    assert "appendWebLog(\"serial1\", telem)" in sketch_source
    assert "if (shouldEmitSerial1Telemetry(otaRuntime)) {" in sketch_source
    # v1.7.33 起 T<t>S<s> 与 $IMU、M:P 统一拼入 s1Buf 后一次性 Serial1.write 发出，
    # 不再使用单独的 Serial1.print(telem)。
    assert "Serial1.write((const uint8_t*)s1Buf" in sketch_source
    assert "car_output.mode == CAR_MODE_MANUAL" in sketch_source

    # Dedicated compact buffer for high-rate Serial1 telemetry.
    assert "static const uint8_t SERIAL1_WEB_LOG_CAPACITY = 64;" in wifi_types
    assert "struct Serial1WebLogEntry" in web_log_source
    assert "s_serial1LogEntries[SERIAL1_WEB_LOG_CAPACITY]" in web_log_source
    assert "appendSerial1WebLog" in web_log_source

    # Real-time WebSocket sink for log streaming.
    assert "typedef void (*WebLogSocketSink)" in web_log_header
    assert "(uint32_t seq, unsigned long t, const char* source, const char* line)" in web_log_header
    assert "void webLogBufferSetSocketSink(WebLogSocketSink sink)" in web_log_header
    assert "webLogBufferSetSocketSink" in web_log_source


def test_serial1_uplink_matches_host_pilot_protocol():
    """v1.7.9 起 Serial1 上行协议对齐上位机 DonkeyCar `ArdImu` / `Arduino` part：

    - MANUAL 模式以 `T<t>S<s>\n` 推送人工油门转向（去掉历史 `:` 分隔符）。
    - MANUAL 模式以 `M<m>:P<p>\n` 推送模式 + Park，状态变化时立即发 + 1 Hz 心跳。
    - 所有模式以 `$IMU,seq,ts_ms,ax,ay,az,gx,gy,gz\n` 推送 MPU6050 6 轴，
      seq 用 `uint16_t` 自然回绕；MPU 未在线时静默不发。
    - 三类上行帧都受 `shouldEmitSerial1Telemetry(otaRuntime)` 闸门保护。
    """

    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")
    config_source = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "FirmwareConfig.h").read_text(encoding="utf-8")

    # 无冒号 T..S.. 字面量，旧的 ":S" 拼装必须消失。
    assert 'String("T") + car_output.throttle + "S" + car_output.steering' in sketch_source
    assert 'String("T") + car_output.throttle + ":S" + car_output.steering' not in sketch_source

    # M:P 触发条件：维护 last 状态 + 心跳节流。
    assert "lastEmittedMode" in sketch_source
    assert "lastEmittedPark" in sketch_source
    assert "lastModeParkEmitMs" in sketch_source
    assert "MODE_PARK_HEARTBEAT_MS" in sketch_source
    # 状态变化或 1Hz 心跳到期才会触发；保证两条件二选一。
    assert re.search(
        r"car_output\.mode\s*!=\s*lastEmittedMode|car_output\.park\s*!=\s*lastEmittedPark",
        sketch_source,
    )

    # $IMU 帧组装：固定前缀 + seq + ts_ms + 6 轴。
    # v1.7.33 起：T<t>S<s>、M<m>:P<p>、$IMU 三类帧统一拼入 char s1Buf[512]，
    # 通过一次 Serial1.write 发出，彻底消除多次 print/write 导致的 TX 缓冲区
    # 指针竞争与帧拼接。v1.7.34 进一步将 setTxBufferSize 提到 begin 之前，
    # 确保 1024B TX 缓冲区真正生效。
    assert '"$IMU,%u,%lu,' in sketch_source  # snprintf 格式串前缀（含逗号）
    assert "static uint16_t imuSeq" in sketch_source
    assert "lastImuEmitMs" in sketch_source
    assert "IMU_TELEMETRY_INTERVAL_MS" in sketch_source
    assert "char s1Buf[512]" in sketch_source
    assert "snprintf(s1Buf + s1Len, sizeof(s1Buf) - s1Len," in sketch_source
    assert "Serial1.write((const uint8_t*)s1Buf" in sketch_source
    # MPU 未在线时静默：必须显式判 valid。
    assert "mpu6050Data.valid" in sketch_source
    # $IMU 文本流不能镜像进 Web Console 日志窗口：100Hz JSON 会顶爆 AsyncWebSocket
    # 发送队列，导致前端 ws 不断断连重连、曲线卡顿。IMU 走 WebSocket 二进制
    # schema v2 的 latest 区即可（见 test_websocket_binary_frame_schema_v2_carries_imu_five_axes）。
    assert 'appendWebLog("serial1", imuLine)' not in sketch_source
    assert 'appendWebLog("serial1", imuBuf)' not in sketch_source

    # T..S.. 文本 web log 推送 100ms 节流（~10Hz）：Serial1 上行仍 60Hz 给上位机，
    # 浏览器日志窗口降频，避免和曲线二进制帧抢 AsyncWebSocket 队列。
    assert "lastTelemWebLogMs" in sketch_source
    assert "TELEM_WEB_LOG_INTERVAL_MS" in sketch_source

    # 节奏常量。
    assert re.search(r"^#define\s+IMU_TELEMETRY_INTERVAL_MS\s+10\b", config_source, re.MULTILINE)
    assert re.search(r"^#define\s+MODE_PARK_HEARTBEAT_MS\s+1000\b", config_source, re.MULTILINE)


def test_wireless_console_policy_mirrors_serial1_uplink_format():
    """桌面侧 `wireless_console_policy.py` 必须提供与固件一一对应的格式化函数，
    供 Tub 录制回放、上位机解析单测、桌面仿真使用。"""

    policy_source = (PROJECT_ROOT / "wireless_console_policy.py").read_text(encoding="utf-8")
    assert "def format_serial1_manual_frame(" in policy_source
    assert "def format_serial1_mode_park_frame(" in policy_source
    assert "def format_imu_telemetry_line(" in policy_source


def test_web_console_serial_targets_forward_to_serial2():
    server_source = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp").read_text(encoding="utf-8")

    # Web Console 的 serial/serial1 目标不再直接转发到硬件 Serial/Serial1，
    # 而是通过 Serial2 转发到 Linux 上位机（v1.7.29 配网协议），避免干扰
    # 车辆控制串口。
    assert "Serial.println(line);" not in server_source
    assert "Serial1.println(line);" not in server_source
    assert 'target.equalsIgnoreCase("serial")' in server_source
    assert "Serial2.print(cmdLine);" in server_source


def test_wifi_console_types_are_split_from_sketch():
    source = firmware_source_text()
    wifi_types = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h").read_text(encoding="utf-8")
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")

    for symbol in [
        "const char* WIFI_CONSOLE_AP_DEFAULT_SSID = \"MUS4-ESP\";",
        "const char* WIFI_CONSOLE_AP_PASSWORD = \"\";",
        "const uint16_t WIFI_CONSOLE_PORT = 2323;",
        "const uint16_t WIFI_WEB_CONSOLE_PORT = 80;",
        "const uint32_t WIFI_WEB_TELEMETRY_MIN_FREE_HEAP = 60000;",
        "const uint8_t WIFI_CONSOLE_CHANNEL = 6;",
        "const uint8_t WIFI_CONSOLE_MAX_CLIENTS = 1;",
        "const unsigned long WIFI_CONSOLE_RETRY_INTERVAL_MS = 5000;",
        "const unsigned long WIFI_STA_CONNECT_TIMEOUT_MS = 15000;",
        "const unsigned long WIFI_STA_APPLY_DELAY_MS = 800;",
        "const unsigned long WIFI_STA_IP_DISPLAY_MS = 60000;",
        "const char* WIFI_OTA_HOSTNAME = \"mus4-ota\";",
        "const char* WIFI_OTA_PASSWORD = \"mus4-debug\";",
        "const uint16_t WIFI_OTA_PORT = 3232;",
        "const unsigned long WIFI_OTA_WINDOW_MS = 120000UL;",
        "const uint8_t WIFI_AP_SSID_MAX_LEN = 32;",
        "const uint8_t WIFI_STA_SSID_MAX_LEN = 32;",
        "const uint8_t WIFI_STA_PASSWORD_MAX_LEN = 63;",
        "const uint8_t WIFI_STA_PASSWORD_MIN_LEN = 8;",
        "const uint8_t WIFI_WEB_LOG_CAPACITY = 64;",
        "const uint16_t WIFI_WEB_DATA_CAPACITY = 256;",
        "const unsigned long WIFI_WEB_DATA_INTERVAL_MS = 16;",
        "struct WebLogEntry",
        "struct WifiScanEntry",
        "struct WebDataPoint",
        "int rcChannels[RC_CHANNEL_COUNT];",
        "int actuatorSteeringDuty;",
        "int actuatorThrottleDuty;",
    ]:
        assert symbol in wifi_types

    # 密码为空时全通道免认证的 helper：空密码下 AUTH: 必然成功，门禁只剩摩擦。
    assert "static inline bool isWirelessConsoleAuthDisabled() { return WIFI_CONSOLE_AP_PASSWORD[0] == '\\0'; }" in wifi_types

    assert "#include \"WifiConsoleTypes.h\"" in sketch_source
    assert "WebLogEntry s_webLogEntries[WIFI_WEB_LOG_CAPACITY];" in source
    assert "static WebLogEntry s_webLogEntries[WIFI_WEB_LOG_CAPACITY];" in source
    assert "WifiScanEntry wifiScanCache[16];" in sketch_source
    assert "WebDataPoint wifiWebData[WIFI_WEB_DATA_CAPACITY];" in sketch_source
    assert "struct WebLogEntry" not in sketch_source
    assert "struct WifiScanEntry" not in sketch_source
    assert "struct WebDataPoint" not in sketch_source
    assert "#include \"WifiConsoleTypes.h\"" in source


def test_wifi_sta_config_command_entry_is_split_from_sketch():
    source = firmware_source_text()
    sta_header = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiStaConfig.h").read_text(encoding="utf-8")
    sta_source = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiStaConfig.cpp").read_text(encoding="utf-8")
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "bool processWifiStaConfigCommand(const String& line, Print& out, WifiRuntimeState& ws)" in sta_header
    assert "bool processWifiStaConfigCommand(const String& line, Print& out, WifiRuntimeState&" in sta_source
    assert "void printWifiStaStatus(Print& out)" in sta_header
    assert "void printWifiStaStatus(Print& out)" in sta_source
    assert "bool copyWifiStaSsid(const String& ssid)" in sta_header
    assert "bool copyWifiStaPassword(const String& password)" in sta_header
    assert "String wifiStaIpText()" in sta_header
    assert "void clearWifiStaLastError()" in sta_header
    assert "void setWifiStaLastError(const char* code, const char* message, bool timedOut)" in sta_header
    assert "void scheduleWifiStaApply()" in sta_header
    assert "bool saveWifiStaPreference(const String& ssid, const String& password)" in sta_header
    assert "bool saveWifiStaSsidPreference(const String& ssid)" in sta_header
    assert "bool saveWifiStaPasswordPreference(const String& password)" in sta_header
    assert "void clearWifiStaRuntimeStateWithoutDisconnect()" in sta_header
    assert "bool clearWifiStaPreference()" in sta_header
    assert "void loadWifiStaPreference()" in sta_header
    assert "bool copyWifiStaSsid(const String& ssid)" in sta_source
    assert "bool copyWifiStaPassword(const String& password)" in sta_source
    assert "String wifiStaIpText()" in sta_source
    assert "void clearWifiStaLastError()" in sta_source
    assert "void setWifiStaLastError(const char* code, const char* message, bool timedOut)" in sta_source
    assert "void scheduleWifiStaApply()" in sta_source
    assert "bool saveWifiStaPreference(const String& ssid, const String& password)" in sta_source
    assert "bool saveWifiStaSsidPreference(const String& ssid)" in sta_source
    assert "bool saveWifiStaPasswordPreference(const String& password)" in sta_source
    assert re.search(r"^void\s+clearWifiStaRuntimeStateWithoutDisconnect\s*\(\)", sta_source, re.MULTILINE)
    assert re.search(r"^bool\s+clearWifiStaPreference\s*\(\)", sta_source, re.MULTILINE)
    assert re.search(r"^void\s+loadWifiStaPreference\s*\(\)", sta_source, re.MULTILINE)
    assert "#include \"WifiStaConfig.h\"" in sketch_source
    assert "bool processWifiStaConfigCommand(const String& line, Print& out, WifiRuntimeState& ws)" not in sketch_source
    assert "void printWifiStaStatus(Print& out)" not in sketch_source
    assert "String wifiStaIpText()" not in sketch_source
    assert "static void clearWifiStaLastError" not in sketch_source
    assert "static void setWifiStaLastError" not in sketch_source
    assert "static void scheduleWifiStaApply" not in sketch_source
    assert "static bool saveWifiStaPreference" not in sketch_source
    assert "bool saveWifiStaSsidPreference(const String& ssid)" not in sketch_source
    assert "bool saveWifiStaPasswordPreference(const String& password)" not in sketch_source
    assert "static void clearWifiStaRuntimeStateWithoutDisconnect" not in sketch_source
    assert "bool clearWifiStaPreference()" not in sketch_source
    assert "static void loadWifiStaPreference" not in sketch_source
    assert "static bool copyWifiStaSsid" not in sketch_source
    assert "static bool copyWifiStaPassword" not in sketch_source

    for symbol in [
        "WIFI_STA configured=%d connected=%d timed_out=%d connecting=%d",
        "last_error_message=\\\"%s\\\"",
        "ssid.length() == 0 || ssid.length() > WIFI_STA_CONFIG_SSID_MAX_LEN",
        "password.length() > 0 && (password.length() < WIFI_STA_CONFIG_PASSWORD_MIN_LEN || password.length() > WIFI_STA_CONFIG_PASSWORD_MAX_LEN)",
        "ws().staPasswordSet = password.length() > 0",
        "ws().staConnected ? WiFi.localIP().toString() : String(\"0.0.0.0\")",
        "保留本轮连接的首个失败原因",
        "if (ws().staLastError[0] != 0) return",
        "snprintf(ws().staLastError, 24, \"%s\", code)",
        "snprintf(ws().staLastErrorMessage, 128, \"%s\", message)",
        "ws().staTimedOut = timedOut",
        "STA failed: %s",
        "ws().staApplyPending = true",
        "WIFI_STA_CONFIG_APPLY_DELAY_MS = 800",
        "ws().staApplyDeadlineMs = millis() + WIFI_STA_CONFIG_APPLY_DELAY_MS",
        "WIFI_STA_CONFIG_PREF_ENABLED_KEY = \"sta_en\"",
        "WIFI_STA_CONFIG_PREF_SSID_KEY = \"sta_ssid\"",
        "WIFI_STA_CONFIG_PREF_PASSWORD_KEY = \"sta_pass\"",
        "ws().prefs->putBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, false)",
        "ws().prefs->remove(WIFI_STA_CONFIG_PREF_SSID_KEY)",
        "ws().prefs->remove(WIFI_STA_CONFIG_PREF_PASSWORD_KEY)",
        "clearWifiStaRuntimeStateWithoutDisconnect()",
        "WIFI_STA_SSID",
        "WIFI_STA_PASSWORD",
        "STA disabled by preference",
        "STA config invalid",
        "ws().staConfigured = true",
        "WIFI_STA_STATUS",
        "WIFI_STA_SSID:",
        "WIFI_STA_PASSWORD:",
        "WIFI_STA_APPLY",
        "WIFI_STA_CLEAR",
        "WIFI_STA_SSID_SAVED configured=%d",
        "WIFI_STA_PASSWORD_SAVED password_set=%d",
        "NACK:WIFI_STA_NOT_CONFIGURED",
        "WIFI_STA_APPLY_OK ssid=\\\"%s\\\"",
        "Serial2.printf(\"WIFI|%s|%s\\n\", ws().staSsid, ws().staPassword);",
        "HOST_WIFI_PROVISIONING_SENT ssid=\\\"%s\\\"",
        "host wifi provisioning sent ssid=%s",
        "WIFI_STA_CLEARED",
    ]:
        assert symbol in sta_source

    assert "processWifiStaConfigCommand(line, out, wifiRuntime)" in source


def test_wireless_command_policy_helpers_are_split_from_sketch():
    source = firmware_source_text()
    wireless_header = (PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "WirelessConsole.h").read_text(encoding="utf-8")
    wireless_source = (PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "WirelessConsole.cpp").read_text(encoding="utf-8")
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "enum WirelessCommandOrigin" in wireless_header
    assert "bool isWirelessCommandAllowed(const String& line, WirelessCommandOrigin origin, WifiRuntimeState& ws)" in wireless_header
    assert "bool isParkLockedWirelessCommand(const String& line)" in wireless_header
    assert "bool isWirelessOtaOpenCommand(const String& line)" in wireless_header
    assert "bool isWirelessOtaStatusCommand(const String& line)" in wireless_header
    assert "bool isWirelessOtaCloseCommand(const String& line)" in wireless_header
    assert "bool isLocalOtaOpenCommand(const String& line)" in wireless_header
    assert "bool isWifiStaConfigCommand(const String& line)" in wireless_header
    assert "bool isWirelessControlCommand(const String& line)" in wireless_header
    assert "String redactWirelessConsoleLine(const String& line)" in wireless_header
    assert "String redactWirelessConsoleLine(const String& line)" in wireless_source
    assert "#include \"WirelessConsole.h\"" in sketch_source

    for symbol in [
        "line.equalsIgnoreCase(\"PING\")",
        "line.equalsIgnoreCase(\"STATUS\")",
        "line.startsWith(\"AUTH:\")",
        "car_output.park == PARK_LOCKED",
        "line.equalsIgnoreCase(\"FILTER_TEST\")",
        "line.startsWith(\"WIFI_STA_PASSWORD:\")",
        "AUTH:<redacted>",
        "WIFI_STA_PASSWORD:<redacted>",
        "return isWirelessControlCommand(line);",
    ]:
        assert symbol in wireless_source

    assert "static bool isWirelessCommandAllowed" not in sketch_source
    assert "static bool isWirelessControlCommand" not in sketch_source
    assert "static bool isParkLockedWirelessCommand" not in sketch_source
    assert "static String redactWirelessConsoleLine" not in sketch_source


def test_drift_assist_helpers_remain_available_after_module_split():
    source = firmware_source_text()

    for symbol in [
        "#define DRIFT_ASSIST_ENABLED     1",
        "void update_drift_assist_control(bool driftValid, bool driftScaleValid)",
        "int apply_drift_assist(int driver_steering)",
        "if (!drift_assist_enabled)",
        "constrain(final_steering, -100, 100)",
    ]:
        assert symbol in source


def test_steering_control_helpers_remain_available_after_module_split():
    source = firmware_source_text()

    for symbol in [
        "struct PIDConfig",
        "struct PIDState",
        "void reset_steering_filter()",
        "int process_steering_signal(int raw_pwm)",
        "constrain((int)pid_state.current_smooth_output, -100, 100)",
        "safe_mode_active = true",
        "mus4LogLine(\"steering\", \"ALARM: Steering Sensor Fault! Safe Mode Activated.\")",
        "void run_steering_tests()",
        "Starting Steering Signal Processing Unit Tests (PID Enabled)",
    ]:
        assert symbol in source



def test_diagnostic_helpers_remain_available_after_module_split():
    source = firmware_source_text()
    diagnostics_source = (PROJECT_ROOT / "libraries" / "mus4_diag" / "src" / "Diagnostics.cpp").read_text(encoding="utf-8")
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")

    for symbol in [
        "bool runBenchmarks()",
        "BENCH: loops=%lu duration=%lums",
        "bool runRegression()",
        "REGRESS: ok=%d",
        "bool runStress()",
        "STRESS: errors_delta=%lu",
        "void notifyDegrade()",
        "void evalDegrade()",
        "DEGRADED MODE ACTIVE",
    ]:
        assert symbol in source

    assert "bool runBenchmarks()" in diagnostics_source
    assert "static bool runBenchmarks()" not in sketch_source
    assert "bool runRegression()" in diagnostics_source
    assert "static bool runRegression()" not in sketch_source
    assert "bool runStress()" in diagnostics_source
    assert "static bool runStress()" not in sketch_source
    assert "void evalDegrade()" in diagnostics_source
    assert "static void evalDegrade()" not in sketch_source


def test_serial_buffer_type_is_shared_after_module_split():
    source = firmware_source_text()
    serial_buffer_types = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "SerialBufferTypes.h").read_text(encoding="utf-8")
    diagnostics_source = (PROJECT_ROOT / "libraries" / "mus4_diag" / "src" / "Diagnostics.cpp").read_text(encoding="utf-8")
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "struct SerialBuf" in serial_buffer_types
    assert "char buf[256]" in serial_buffer_types
    assert "uint32_t errors" in serial_buffer_types
    assert "#include \"SerialBufferTypes.h\"" in source
    assert "struct SerialBuf" not in diagnostics_source
    assert "struct SerialBuf" not in sketch_source



def test_gamepad_mode_remains_available_after_module_split():
    source = firmware_source_text()

    assert "void sendGamepadPacket()" in source
    assert "bleGamepad.setLeftThumb(0, ly)" in source
    assert "bleGamepad.setRightThumb(lx, 0)" in source



def test_sensor_helpers_remain_available_after_module_split():
    source = firmware_source_text()

    for symbol in [
        "void printLastI2CScanSummary()",
        "void read_ina219()",
        "void setup_ina219()",
        "void read_mpu6050()",
        "void scanI2CBus()",
        "bool tryInitMPU6050OnCurrentBus(uint8_t *activeAddress, int maxRetriesPerAddress)",
        "void setup_mpu6050()",
    ]:
        assert symbol in source



def test_joystick_calibration_remains_available_after_module_split():
    source = firmware_source_text()

    assert "struct AxisCalibration" in source
    assert "struct JoystickCalibrationData" in source
    assert "enum class JoystickCalState" in source
    assert "void loadJoystickCalibration()" in source
    assert "bool saveJoystickCalibration()" in source
    assert "void resetJoystickCalibration()" in source
    assert "int mapJoystickAxis(" in source
    assert "bool startJoystickCalibration(Print& out)" in source
    assert "void updateJoystickCalibration()" in source
    assert "MUS4_PREF_JOYSTICK_STEER_MIN_KEY" in source
    assert "MUS4_PREF_JOYSTICK_STEER_EN_KEY" in source
    assert "MUS4_PREF_STEER_MIN_KEY" in source
    assert "MUS4_PREF_STEER_CAL_EN_KEY" in source



def test_log_bridge_remains_available_after_module_split():
    source = firmware_source_text()

    assert "void mus4SetWebLogSink(Mus4LogSink sink)" in source
    assert "void setMus4LogTargetWeb()" in source
    assert "void mus4LogLine(const char* source, const String& line)" in source
    assert "void mus4Logf(const char* source, const char* fmt, ...)" in source
    assert "extern uint8_t mus4LogTarget" in source
    assert "mus4SetWebLogSink(appendWebLog)" in source


def test_web_log_buffer_is_split_from_sketch():
    sketch = MUS4_SKETCH.read_text(encoding="utf-8")
    source = firmware_source_text()

    assert "void appendWebLog(const char* source, const String& line)" in source
    assert "void appendWebLogLines(const char* source, const String& text)" in source
    assert "void writeWebLogsJson(String& response, uint32_t since)" in source
    assert "static void appendWifiWebLog" not in sketch
    assert "appendWifiWebLog(" not in sketch
    assert "void processWirelessConsoleLine(const String& line, Print& out, WirelessCommandOrigin origin)" in source
    assert "void printWirelessStatus(Print& out)" in source
    assert "static void processWirelessConsoleLine" not in sketch
    assert "static void printWirelessStatus" not in sketch



def test_i2c_and_led_helpers_remain_available_after_module_split():
    source = firmware_source_text()

    for symbol in [
        "bool I2CRead(uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t *Data)",
        "uint16_t I2CReadValue(uint8_t addr, uint8_t reg)",
        "void I2CWriteValue(uint8_t Address, uint8_t Register, uint16_t Data)",
        "const char *identifyI2CDeviceByAddress(uint8_t address)",
        "bool I2CReadRegister8(uint8_t address, uint8_t reg, uint8_t *value)",
        "bool probeMPU6050AtAddress(uint8_t address, uint8_t *whoAmI)",
        "void setLEDColor(CRGB targetColor)",
        "void setLEDToggle(CRGB color1, CRGB color2)",
        "void setLEDToggle(CRGB color1, CRGB color2, CRGB color3)",
        "void scanLEDToggle()",
        "void runLedPowerOnSelfTest()",
        "runLedPowerOnSelfTest();",
    ]:
        assert symbol in source



def test_rc_interrupt_state_keeps_iram_and_volatile_guards():
    source = firmware_source_text()

    assert re.search(r"^volatile\s+uint16_t\s+pwm_value\[RC_CHANNEL_COUNT\]", source, re.MULTILINE)
    assert re.search(r"^volatile\s+unsigned\s+long\s+last_valid_time\[RC_CHANNEL_COUNT\]", source, re.MULTILINE)
    assert "void IRAM_ATTR handle_interrupt" in source
    assert "void IRAM_ATTR CH1_interrupt()" in source
    assert "static bool IRAM_ATTR onRcModeCapture" in source
    assert "void setupRcPwmCapture()" in source


def test_rc_pwm_capture_is_split_from_sketch():
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")
    capture_source = (PROJECT_ROOT / "libraries" / "mus4_rc" / "src" / "RcPwmCapture.cpp").read_text(encoding="utf-8")

    assert "#include \"RcPwmCapture.h\"" in sketch_source
    assert "static void IRAM_ATTR acceptRcPulse" not in sketch_source
    assert "void IRAM_ATTR handle_interrupt" not in sketch_source
    assert "void IRAM_ATTR CH1_interrupt()" not in sketch_source
    assert "void (*isr_functions[RC_CHANNEL_COUNT])()" not in sketch_source
    assert "mcpwm_new_capture_timer" not in sketch_source
    assert "static void IRAM_ATTR acceptRcPulse" in capture_source
    assert "void IRAM_ATTR handle_interrupt" in capture_source
    assert "void IRAM_ATTR CH1_interrupt()" in capture_source
    assert "void (*isr_functions[RC_CHANNEL_COUNT])()" in capture_source
    assert "void setupRcPwmCapture()" in capture_source


def test_control_mixer_is_split_from_sketch():
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")
    mixer_source = (PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "ControlMixer.cpp").read_text(encoding="utf-8")

    assert "#include \"ControlMixer.h\"" in sketch_source
    assert "void mode_change(bool modeValid)" not in sketch_source
    assert "void updateControlOutput()" not in sketch_source
    assert "int lastCarMode = -1" not in sketch_source
    assert "int carOutputModeLast = -1" not in sketch_source
    assert "void mode_change(bool modeValid)" in mixer_source
    assert "void updateControlOutput()" in mixer_source
    assert "buzzer.playModeSound" in mixer_source
    assert "sendGamepadPacket()" in mixer_source
    assert "apply_drift_assist" in mixer_source


def test_safety_state_is_split_from_sketch():
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")
    safety_header = (PROJECT_ROOT / "libraries" / "mus4_safety" / "src" / "SafetyState.h").read_text(encoding="utf-8")
    safety_source = (PROJECT_ROOT / "libraries" / "mus4_safety" / "src" / "SafetyState.cpp").read_text(encoding="utf-8")

    assert "#include \"SafetyState.h\"" in sketch_source
    assert "void emergencyStop()" not in sketch_source
    assert "void park_change()" not in sketch_source
    assert "enum EmergencyStopState" not in sketch_source
    assert "EmergencyStopState emergencyStopState" not in sketch_source
    assert "enum EmergencyStopState" in safety_header
    assert "extern EmergencyStopState emergencyStopState" in safety_header
    assert "void emergencyStop()" in safety_source
    assert "void park_change()" in safety_source
    assert "EmergencyStopState emergencyStopState = EST_IDLE" in safety_source
    assert "buzzer.playParkUnlockSound()" in safety_source
    assert "buzzer.playParkLockSound()" in safety_source


def test_actuator_output_is_split_from_sketch():
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")
    actuator_source = (PROJECT_ROOT / "libraries" / "mus4_safety" / "src" / "ActuatorOutput.cpp").read_text(encoding="utf-8")

    assert "#include \"ActuatorOutput.h\"" in sketch_source
    assert "ledcAttachChannel(STEERING_PIN" not in sketch_source
    assert "ledcWriteChannel(CH_STEERING" not in sketch_source
    assert "ledcAttachChannel(THROTTLE_PIN" not in sketch_source
    assert "ledcWriteChannel(CH_THROTTLE" not in sketch_source
    assert "setupActuatorOutput()" in sketch_source
    assert "updateActuatorOutput()" in sketch_source
    assert "void setupActuatorOutput()" in actuator_source
    assert "void updateActuatorOutput()" in actuator_source
    assert "ledcAttachChannel(STEERING_PIN" in actuator_source
    assert "ledcWriteChannel(CH_STEERING" in actuator_source
    assert "int servo_mid_v = 7372" in actuator_source
    assert "int motor_mid_v = 7372" in actuator_source
    assert "extern const int SERVO_RANGE_V = 2458" in actuator_source
    assert "extern const int MOTOR_RANGE_V = 2458" in actuator_source


def test_wifi_ota_status_helpers_are_split_from_sketch():
    source = firmware_source_text()
    ota_header = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiOta.h").read_text(encoding="utf-8")
    ota_source = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiOta.cpp").read_text(encoding="utf-8")
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "#include \"WifiOta.h\"" in sketch_source
    assert "unsigned long wifiOtaTtlMs(OtaRuntimeState& os, WifiRuntimeState& ws)" in ota_header
    assert "void printWifiOtaStatus(Print& out, OtaRuntimeState& os, WifiRuntimeState& ws)" in ota_header
    assert "void closeWifiOtaWindow(const char* reason, OtaRuntimeState& os)" in ota_header
    assert "void forceWifiOtaParkLocked()" in ota_header
    assert "void keepDevModeOtaWindowActive(OtaRuntimeState& os, WifiRuntimeState& ws)" in ota_header
    assert "bool shouldEmitSerial1Telemetry(OtaRuntimeState& os)" in ota_header
    assert "void openLocalWifiOtaWindow(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os)" in ota_header
    assert "bool processLocalOtaMaintenanceCommand(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os, WifiRuntimeState& ws)" in ota_header
    assert "void openWifiOtaWindow(Print& out, WirelessCommandOrigin origin, OtaRuntimeState& os, WifiRuntimeState& ws)" in ota_header
    assert "void updateWifiOta(OtaRuntimeState& os, WifiRuntimeState& ws)" in ota_header
    assert re.search(r"^unsigned long\s+wifiOtaTtlMs\s*\(OtaRuntimeState& os, WifiRuntimeState& ws\)", ota_source, re.MULTILINE)
    assert re.search(r"^void\s+printWifiOtaStatus\s*\(Print& out, OtaRuntimeState& os, WifiRuntimeState& ws\)", ota_source, re.MULTILINE)
    assert re.search(r"^void\s+closeWifiOtaWindow\s*\(const char\* reason, OtaRuntimeState& os\)", ota_source, re.MULTILINE)
    assert re.search(r"^void\s+forceWifiOtaParkLocked\s*\(\)", ota_source, re.MULTILINE)
    assert re.search(r"^void\s+keepDevModeOtaWindowActive\s*\(OtaRuntimeState& os, WifiRuntimeState& ws\)", ota_source, re.MULTILINE)
    assert re.search(r"^bool\s+shouldEmitSerial1Telemetry\s*\(OtaRuntimeState& os\)", ota_source, re.MULTILINE)
    assert re.search(r"^void\s+openLocalWifiOtaWindow\s*\(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os\)", ota_source, re.MULTILINE)
    assert re.search(r"^bool\s+processLocalOtaMaintenanceCommand\s*\(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os, WifiRuntimeState& ws\)", ota_source, re.MULTILINE)
    assert re.search(r"^void\s+openWifiOtaWindow\s*\(Print& out, WirelessCommandOrigin origin, OtaRuntimeState& os, WifiRuntimeState& ws\)", ota_source, re.MULTILINE)
    assert re.search(r"^void\s+updateWifiOta\s*\(OtaRuntimeState& os, WifiRuntimeState& ws\)", ota_source, re.MULTILINE)
    assert "static void printWifiOtaStatus" not in sketch_source
    assert "static void closeWifiOtaWindow" not in sketch_source
    assert "static void forceWifiOtaParkLocked" not in sketch_source
    assert "static void keepDevModeOtaWindowActive" not in sketch_source
    assert "static bool shouldEmitSerial1Telemetry" not in sketch_source
    assert "static void openLocalWifiOtaWindow" not in sketch_source
    assert not re.search(r"^bool\s+processLocalOtaMaintenanceCommand\s*\(", sketch_source, re.MULTILINE)
    assert "static void openWifiOtaWindow" not in sketch_source
    assert "static void updateWifiOta" not in sketch_source
    assert "OTA_STATUS started=%d window=%d in_progress=%d ttl_ms=%lu progress=%u park=%d dev_mode=%d park_guard=%d" in ota_source
    assert "if (!os.windowOpen) return 0" in ota_source
    assert "WIFI_CONSOLE_AP_PASSWORD" in ota_source
    assert "WIFI_OTA_PORT" in ota_source
    assert "WIFI_OTA_WINDOW_MS" in ota_source
    assert "if (ws.devModeEnabled) return WIFI_OTA_WINDOW_MS" in ota_source
    assert "os.deadlineMs - now" in ota_source
    assert "os.started ? 1 : 0" in ota_source
    assert "os.lastProgressPct" in ota_source
    assert "car_output.park ? 1 : 0" in ota_source
    assert "os.parkGuardActive ? 1 : 0" in ota_source
    assert "os.windowOpen = false" in ota_source
    assert "os.deadlineMs = 0" in ota_source
    assert "os.inProgress = false" in ota_source
    assert "os.parkGuardActive = false" in ota_source
    assert "os.lastProgressPct = 0" in ota_source
    assert "ArduinoOTA.end()" in ota_source
    assert "os.started = false" in ota_source
    assert "mus4LogLine(\"ota\", String(\"closed: \") + reason)" in ota_source
    assert "rc_data.park = PARK_LOCKED" in ota_source
    assert "car_output.park = PARK_LOCKED" in ota_source
    assert "car_output.throttle = 0" in ota_source
    assert "if (!ws.devModeEnabled) return" in ota_source
    assert "ensureWifiOtaStarted()" in ota_source
    assert "os.windowOpen = true" in ota_source
    assert "os.deadlineMs = millis() + WIFI_OTA_WINDOW_MS" in ota_source
    # v1.7.8 起：仅在 OTA 真正传输期间暂停 Serial1，避免 DEV ON 时窗口长期打开阻塞通信。
    assert "return !os.inProgress;" in ota_source
    assert "line.substring(11).equals(WIFI_CONSOLE_AP_PASSWORD)" in ota_source
    assert "out.println(\"NACK:AUTH_REQUIRED\")" in ota_source
    assert "sb.errors++" in ota_source
    assert "os.lastProgressPct = 0" in ota_source
    assert "OTA_READY ip=%s port=%u ttl_ms=%lu" in ota_source
    assert "mus4LogLine(\"ota\", \"ready: local\")" in ota_source
    assert "isLocalOtaOpenCommand(line)" in ota_source
    assert "openLocalWifiOtaWindow(line, out, sb, os)" in ota_source
    assert "isWirelessOtaStatusCommand(line)" in ota_source
    assert "printWifiOtaStatus(out, os, ws)" in ota_source
    assert "isWirelessOtaCloseCommand(line)" in ota_source
    assert "closeWifiOtaWindow(\"LOCAL\", os)" in ota_source
    assert "out.println(\"OTA_CLOSED\")" in ota_source
    assert "return false" in ota_source
    assert "ws.devModeEnabled && origin == WIRELESS_ORIGIN_WEB" in ota_source
    assert "if (!webDevMode && !ws.consoleAuthenticated && !isWirelessConsoleAuthDisabled())" in ota_source
    assert "out.println(\"NACK:AUTH_REQUIRED\")" in ota_source
    assert "if (car_output.park != PARK_LOCKED)" in ota_source
    assert "out.println(\"NACK:PARK_REQUIRED\")" in ota_source
    assert "wifiConsoleBuf.errors++" in ota_source
    assert "mus4LogLine(\"ota\", webDevMode ? \"ready: web_dev\" : \"ready\")" in ota_source
    assert "if (ws.devModeEnabled) keepDevModeOtaWindowActive(os, ws)" in ota_source
    assert "if (!os.windowOpen) return" in ota_source
    assert "if (os.inProgress || os.parkGuardActive)" in ota_source
    assert "forceWifiOtaParkLocked()" in ota_source
    assert "unsigned long now = millis()" in ota_source
    assert "!ws.devModeEnabled && !os.inProgress && (long)(now - os.deadlineMs) >= 0" in ota_source
    assert "closeWifiOtaWindow(\"TIMEOUT\", os)" in ota_source
    assert "ArduinoOTA.handle()" in ota_source
    assert "inline bool shouldEmitSerial1Telemetry(OtaRuntimeState& os) { (void)os; return true; }" in ota_header
    assert "printWifiOtaStatus(out, otaRuntime, wifiRuntime)" in source
    assert "closeWifiOtaWindow(\"USER\", otaRuntime)" in source
    assert "closeWifiOtaWindow(\"DEV_MODE_OFF\", otaRuntime)" in source


def test_wireless_ota_and_control_safety_guards_remain_present():
    source = firmware_source_text()

    assert "bool parseAndValidateCommand(String cmd, int* throttle, int* steering)" in source
    assert "t < -100 || t > 100 || s < -100 || s > 100" in source
    assert "isWirelessOtaOpenCommand(line)" in source
    assert "car_output.park == PARK_LOCKED" in source
    # v1.7.8 起：Serial1 暂停语义仅看 inProgress（见 WifiOta.cpp::shouldEmitSerial1Telemetry）
    assert "return !os.inProgress;" in source
    assert "forceWifiOtaParkLocked" in source
    assert "AUTH:<redacted>" in source
    assert "WIFI_STA_PASSWORD:<redacted>" in source


def test_web_console_uses_dev_label_for_development_switch():
    """v1.8.24：DEV 开关由滑珠开关改为 DonkeyDrifter 同款文字胶囊按钮
    （#devModeToggle 直接显示 "DEV"，devOn 态 cyan 高亮）；滑珠 / devModeSwitchText /
    devModeCheck / requestDevModeToggle 死代码全部移除，状态改由 uiDevMode 维护。"""
    source = firmware_source_text()

    assert "DEV <b id=\"devModeSwitchText\">OFF</b>" not in source
    assert "devModeSwitchText" not in source
    assert "devModeCheck" not in source
    assert "requestDevModeToggle" not in source
    # v1.8.21：DEV 开关恢复至 DC 头部（PR #124 曾移至 DonkeyDrifter 顶栏，现加回）
    # v1.8.24：改为 DD 同款文字胶囊按钮，role=switch + aria-checked 由 renderDevMode 同步
    assert 'id="devModeToggle" class="devHint" onclick="toggleDevModeFromSwitch()" role="switch" aria-checked="false">DEV</button>' in source
    assert '#devModeToggle' in source
    assert '#devModeToggle.devOn{' in source
    assert "function renderDevMode(v){uiDevMode=!!v" in source
    assert "function toggleDevModeFromSwitch(){if(uiDevMode)" in source
    assert "DEV MODE <b id=\"devModeSwitchText\">OFF</b>" not in source
    assert "DEBUG MODE <b id=\"devModeSwitchText\">OFF</b>" not in source
    assert "Auto OTA <b id=\"devModeSwitchText\">OFF</b>" not in source


def test_web_console_header_and_state_cards_keep_compact_layout():
    source = firmware_source_text()

    # v1.8.21：OTA 按钮与 DEV 开关恢复至 DC 头部（PR #124 曾移至 DonkeyDrifter 顶栏，现加回）
    # v1.8.24：OTA 改为 .otaLink 文字胶囊，DEV 改为 #devModeToggle 文字胶囊（DD 同款）
    assert '.otaLink{display:inline-flex;align-items:center;justify-content:center;height:32px' in source
    assert '#devModeToggle{display:inline-flex;align-items:center;justify-content:center;height:32px' in source
    assert '#devModeToggle' in source
    # v1.8.20：主 DC 页标题行默认显示，仅在 DD 嵌入（?embedded=1）时经 body.embedded 隐藏
    assert ".headerRow{display:flex;align-items:center;" in source
    assert "body.embedded .headerRow{display:none}" in source
    assert ".toggleSwitch{position:relative;display:inline-flex;align-items:center;gap:8px;cursor:pointer}" in source
    assert ".otaLink" in source
    assert ".devHint{position:relative}" in source
    assert ".devHint:hover:after" in source
    assert "content:'开发模式会持久化；Web Console 免 AUTH，但仍保留 Park Locked 安全限制。OTA 传输期间会默认 Park Locked。'" in source
    assert ".version{color:#8fa1b5;font-size:12px;text-transform:uppercase;letter-spacing:.08em;display:inline-block}" in source
    assert ".ghLink{display:inline-flex;align-items:center;color:#8fa1b5;margin-left:6px}" in source
    assert ".stateGrid{display:grid;gap:10px;align-items:stretch;grid-template-columns:" in source
    assert "#modeCard{grid-area:mode}" in source
    assert "#parkCard{grid-area:park}" in source
    assert "#driftCard{grid-area:drift}" in source
    assert "#voltageCard{grid-area:voltage}" in source
    assert "#networkCard{grid-area:network}" in source
    assert 'grid-template-areas:"mode park drift voltage network"' in source
    assert 'grid-template-areas:"mode park drift" "voltage network network"' in source


def test_web_console_header_logo_left_of_title():
    """顶栏主标题左侧 logo：与 Donkey 页面（launcher）同款 .headerLogo
    （32x32、8px 圆角、1px 描边、align-self:center），直接复用固件内嵌的
    /favicon.png（与 Projects/logo.png 同一头盔图），浅色主题描边同步切换。"""

    source = firmware_source_text()

    assert ".headerLogo{width:32px;height:32px;border-radius:8px;border:1px solid #2b3441;align-self:center}" in source
    assert 'html[data-theme="light"] .headerLogo{border-color:#d5dce4}' in source
    # 位置：logo 在 headerRow 内、主标题 <h1> 左边
    header_pos = source.index('<div class="headerRow">')
    logo_pos = source.index('<img class="headerLogo" src="/favicon.png" alt="Drifter Console">')
    h1_pos = source.index('<h1><a class="titleLink" href="https://www.donkeydrift.com" target="_blank" rel="noopener" data-i18n="app.title">Drifter Console</a></h1>')
    assert header_pos < logo_pos < h1_pos
    # v1.7.97 起 logo 包锚点：点击在新标签页打开 donkeydrift.com，布局不变
    assert '<a class="logoLink" href="https://www.donkeydrift.com" target="_blank" rel="noopener"><img class="headerLogo" src="/favicon.png" alt="Drifter Console"></a>' in source
    assert ".logoLink{display:inline-flex}" in source
    # Issue #179 起标题文字也包锚点：与 logo 同 URL、新标签页打开，颜色继承标题、无下划线
    assert '<h1><a class="titleLink" href="https://www.donkeydrift.com" target="_blank" rel="noopener" data-i18n="app.title">Drifter Console</a></h1>' in source
    assert ".titleLink{color:inherit;text-decoration:none}" in source
    # v1.8.15：标题字号/字重/字色与间距对齐 DD 主导航（text-xl 20px / font-bold 700 /
    # zinc-100 前景色；标题↔功能 32px = gap 12 + margin-right 20）
    assert "h1{margin:0;font-size:1.25rem;font-weight:700;line-height:1.75rem}" in source
    assert ".headerRow h1{color:#e8edf2;margin:0 20px 0 0}" in source
    assert 'html[data-theme="light"] .headerRow h1{color:#1a2330}' in source


def test_web_console_mobile_header_layout():
    """窄屏（手机/平板竖屏，max-width:820px）头部重排为固定 4 行：
    第 1 行 logo + 标题 + GitHub + 版本号（紧跟 GitHub 右侧，整体左排）；
    第 2 行 打开 Donkey / 打开 DonkeyDrifter / 打开 Kimi Code Web / ZCode / 打开 DeepSeek Harness；
    第 3 行 红绿蓝（最左）+ OTA + 静音 + DEV（margin-left:auto 贴合最右端）；
    第 4 行 主题切换（左）+ 语言切换（右）。
    实现方式：headerRow 保持 flex-wrap，DOM 中三个 .rowBreak 分隔 span 桌面
    display:none，窄屏下 display:block + flex-basis:100% 强制换行，各元素用
    order 重排。桌面布局规则不动，仅靠媒体查询覆盖。
    v1.7.70 第 2 行新增"打开 Kimi Code Web"占位按钮，order 插入 8，后续顺移 +1。
    v1.8.7 第 2 行再新增"打开 DeepSeek Harness"按钮，order 插入 9，后续顺移 +1。
    v1.8.42 第 2 行再新增"ZCode"按钮（#openZCodeBtn，插在 Kimi Code Web 与
    DeepSeek Harness 之间），order 插入 9，后续顺移 +1。"""

    source = firmware_source_text()

    assert ".rowBreak{display:none}" in source
    assert '<span class="rowBreak br1"></span>' in source
    # v1.8.21：Donkey / OTA / DEV 恢复至 DC 头部，br2/br3 随之加回
    assert '<span class="rowBreak br2"></span>' in source
    assert '<span class="rowBreak br3"></span>' in source
    assert '<span class="version" id="versionLabel">--</span><span class="rowBreak br1"></span>' in source
    assert "@media (max-width:820px){.headerRow{align-items:center;gap:8px}" in source
    assert ".rowBreak{display:block;flex-basis:100%;height:0}" in source
    # 第 1 行：logo + 标题 + GitHub + 版本号
    assert ".headerLogo{order:1}" in source
    assert ".headerRow h1{order:2}" in source
    assert ".ghLink{order:3}" in source
    assert "#versionLabel{order:4}" in source
    assert "#versionLabel{order:4;margin-left:auto}" not in source
    assert ".br1{order:5}" in source
    # 第 2 行：Donkey / DonkeyDrifter / Kimi Code Web / ZCode / DeepSeek Harness
    assert "#enterDonkeyBtn{order:6}" in source
    assert "#enterDonkeyDrifterBtn{order:7}" in source
    assert "#openKimiCodeWebBtn{order:8}" in source
    assert "#openZCodeBtn{order:9}" in source
    assert "#openDshBtn{order:10}" in source
    # 第 3 行：静音（桌面右推 margin-left:auto 复位）+ 主题 + 语言（右对齐）
    assert ".headerRow .otaLink{order:13}" in source
    assert "#muteToggle{order:14;margin-left:0}" in source
    assert "#devModeToggle{order:15;margin-left:auto}" in source
    assert "#themeToggle{order:17}" in source
    assert "#langToggle{order:18;margin-left:auto}" in source


def test_web_console_language_tabs_wired_to_set_language():
    """Issue #92（v1.8.3）：顶栏语言切换改为静音式单按钮 #langToggle
    （形态参照 #muteToggle/.muteButton），单击即在中↔英间来回切换，不弹菜单；
    按钮文字反映当前语言（中文态显"中"、英文态显"EN"，由 renderLangButton 渲染）。
    默认语言跟随浏览器：readStoredLanguage 对非 zh/en 存值回退
    detectBrowserLanguage()（navigator.language zh* → zh，其余 → en）；
    手动选择仍经 setLanguage 持久化 localStorage（mus4.ui.lang）。
    原 .langSwitch 中文/English 分段控件移除；.langTabs 仅供 #ledBlinkTabs 使用。"""

    source = firmware_source_text()

    assert ".langTabs{display:inline-flex;align-items:center;gap:2px;background:#171c24;border:1px solid #344154;border-radius:999px;padding:0 2px;height:24px;box-sizing:border-box;box-shadow:inset 0 0 0 1px #2b3441}" in source
    # 外大椭圆（box-sizing:border-box；v1.7.94 起外圈 border 1px #344154 +
    # 内嵌 box-shadow 1px #2b3441 = DC 粗框语言，inset 描边不占布局）
    # + 内连体分段（#ledBlinkTabs 覆写容器 34px 高 + 4px 纵向 padding，border 占 2px，
    # 分段保持 24px 满高，蓝色选中段与 OTA 按钮蓝对蓝），
    # 与 DonkeyDrifter Web UI 手动/自动模式切换条同款内外嵌套语言；
    # v1.7.78 起 .langTabs 仅供 #ledBlinkTabs 使用
    assert ".langTabs button{padding:0 10px;height:24px;min-width:0;border:none;border-radius:999px;" in source
    assert ".langTabs button.active{background:#5cc8ff;color:#061019}" in source
    # Issue #92 后续样式统一：三页面（DC/D/DD）语言按钮配色对齐 DC/D 深浅切换
    # （themeButton）——32×32 圆形、深色 #111820 底 + #344154 边框 + inset 1px
    # #2b3441 内圈、字色 #b9c5d3、hover #e8edf2；浅色 #f4f6f9/#ccd5df/#d5dce4/
    # #3f4f63、hover #1a2330；字体栈沿用 DD index.css :root（含 font-synthesis/
    # text-rendering/font-smoothing）；hover 补 background 锁定，抵消 DC 通用
    # button:hover 的背景覆盖
    assert ".langButton{display:inline-flex;align-items:center;justify-content:center;width:32px;height:32px;min-width:0;padding:0;border:1px solid #344154;border-radius:9999px;background:#111820;box-shadow:inset 0 0 0 1px #2b3441;color:#b9c5d3;font-family:-apple-system,BlinkMacSystemFont,\"Segoe UI\",\"Noto Sans\",Helvetica,Arial,sans-serif,\"Apple Color Emoji\",\"Segoe UI Emoji\";font-synthesis:none;text-rendering:optimizeLegibility;-webkit-font-smoothing:antialiased;-moz-osx-font-smoothing:grayscale;font-size:12px;font-weight:600;line-height:1;cursor:pointer;transition:color .15s cubic-bezier(.4,0,.2,1),background-color .15s cubic-bezier(.4,0,.2,1),border-color .15s cubic-bezier(.4,0,.2,1)}" in source
    assert ".langButton:hover,.langButton:focus-visible{color:#e8edf2;background:#111820}" in source
    assert 'html[data-theme="light"] .langButton{background:#f4f6f9;border-color:#ccd5df;box-shadow:inset 0 0 0 1px #d5dce4;color:#3f4f63}' in source
    assert 'html[data-theme="light"] .langButton:hover,html[data-theme="light"] .langButton:focus-visible{color:#1a2330;background:#f4f6f9}' in source
    # DOM：单按钮 + 单击切换 + aria 标题走 i18n（沿用 language.title 词条）
    assert '<button type="button" id="langToggle" class="langButton" onclick="toggleLanguage()" aria-label="语言" data-i18n-aria="language.title">中</button>' in source
    # 分段控件死代码不残留（.langSwitch、data-lang 双按钮已随 Issue #92 移除）
    assert "langSwitch" not in source
    assert 'data-lang="zh"' not in source
    assert 'data-lang="en"' not in source
    assert ">English</button>" not in source
    # v1.8.21：OTA 按钮恢复至 DC 头部，otaLink 随之加回
    assert '<a href="/update" class="otaLink"' in source
    assert ".otaLink" in source
    # v1.8.3：单击切换 + 按钮文字渲染（applyLanguage 内调用 renderLangButton）
    assert "function toggleLanguage(){setLanguage(uiLang==='zh'?'en':'zh')}" in source
    assert "function renderLangButton(){const b=document.getElementById('langToggle');if(b)b.textContent=uiLang==='zh'?'中':'EN'}" in source
    assert "renderLangButton();refreshDynamicLabels()" in source
    # 默认语言跟随浏览器：存值非法/缺失时回退 detectBrowserLanguage（zh* → zh，其余 → en）
    assert "function readStoredLanguage(){try{const v=localStorage.getItem(LANG_STORAGE_KEY);return v==='zh'||v==='en'?v:detectBrowserLanguage()}catch(e){return detectBrowserLanguage()}}" in source
    assert "function detectBrowserLanguage(){try{return String(navigator.language||'').toLowerCase().indexOf('zh')===0?'zh':'en'}catch(e){return 'zh'}}" in source
    assert "function setLanguage(lang){uiLang=normalizeLanguage(lang);writeStoredLanguage(uiLang);applyLanguage(uiLang);fetch('/api/language?lang='+uiLang,{method:'POST'}).catch(()=>{})}" in source
    assert 'grid-template-areas:"mode park voltage" "drift drift drift" "network network network"' in source
    assert "minmax(160px,.56fr)" in source
    assert "grid-template-columns:84px 154px 100px" in source
    assert "#modeCard .stateValue,#parkCard .stateValue,#voltageCard .stateValue,#driftCard .stateValue,#networkCard .stateValue{font-size:18px}" in source
    assert "#modeCard .stateSub,#parkCard .stateSub,#driftCard .stateSub{font-size:11px}" in source
    assert "#voltageCard .stateMeta span,#networkCard .stateMeta span{font-size:13px}" in source
    assert "@media(max-width:620px){" in source
    assert ".rcGrid{grid-template-columns:repeat(3,minmax(72px,1fr))}" in source
    assert ".stateCard{position:relative;overflow:hidden;border:1px solid #344154;border-radius:10px;padding:12px" in source
    assert ".stateValue{font-size:24px;font-weight:800;margin-top:4px;white-space:normal;overflow:visible;text-overflow:clip;word-break:normal;overflow-wrap:normal;line-height:1.08}" in source
    assert ".stateMeta span{font-size:15px;font-weight:700;white-space:normal;overflow:visible;text-overflow:clip;word-break:normal;overflow-wrap:normal;line-height:1.2}" in source
    assert "text-overflow:ellipsis" not in source


def test_web_console_header_github_link_replaces_version_label():
    """标题右侧（原版本号位置）放 GitHub 图标链接，新标签页打开
    DonkeyDrift/Firmware 仓库；图标用 currentColor 内嵌 SVG，
    默认暗色 #8fa1b5、悬停 ESP32 蓝 #5cc8ff。"""

    source = firmware_source_text()

    assert '<a class="ghLink" href="https://github.com/DonkeyDrift/Firmware" target="_blank" rel="noopener"' in source
    assert 'aria-label="GitHub: DonkeyDrift/Firmware"' in source
    assert '<svg viewBox="0 0 16 16" width="20" height="20" fill="currentColor" aria-hidden="true"><path d="M8 0C3.58' in source
    # 位置：紧跟主标题 </h1>（原版本号位置）、在语言单按钮 langToggle 左边
    assert source.index('<h1><a class="titleLink" href="https://www.donkeydrift.com" target="_blank" rel="noopener" data-i18n="app.title">Drifter Console</a></h1>') < source.index('<a class="ghLink"') < source.index('id="langToggle"')
    assert '.ghLink{display:inline-flex;align-items:center;color:#8fa1b5;' in source
    assert '.ghLink:hover{color:#5cc8ff}' in source


def test_web_console_drift_card_tune_link_left_of_state_dot():
    """DRIFT 卡片右上角：Tune 文字与状态灯包在 .tunePair 行内容器里。状态灯
    inline-block + vertical-align:middle、无额外位移，圆点中心与其它卡片
    状态灯同为 cy=83 保持平行（圆点位置不再调整）；Tune 文字用
    vertical-align:-1px 下移 1px，使其视觉中心与圆点对齐（文字偏上的反馈）。
    卡片末尾不再有独立状态灯，避免与 Tune 文字挤在同一角落。"""

    source = firmware_source_text()

    drift = source[source.index('id="driftCard"'):source.index('id="voltageCard"')]
    assert '<span class="tunePair"><a href="/drift" id="driftTuneLink">Tune</a><span class="stateDot"></span></span>' in drift
    assert 'id="driftNeedle"></i></div><span class="stateDot"></span></div>' not in drift
    assert '.tunePair{position:absolute;right:12px;top:11px;font-size:11px;line-height:10px;' in source
    assert '.tunePair a{color:#5cc8ff;text-decoration:none;vertical-align:-1px}' in source
    # 圆点无 transform 位移，保持与其它卡片平行；只调 Tune 文字
    assert '.tunePair .stateDot{position:static;display:inline-block;vertical-align:middle;margin-left:5px;width:10px;height:10px}' in source
    assert '.stateDot{position:absolute;right:12px;top:12px;width:10px;height:10px;' in source


def test_web_console_places_voltage_before_combined_network_card():
    source = firmware_source_text()

    drift_index = source.index('id="driftCard"')
    voltage_index = source.index('id="voltageCard"')
    network_index = source.index('id="networkCard"')

    assert 'id="apCard"' not in source
    assert 'id="staCard"' not in source
    assert drift_index < voltage_index < network_index


def test_web_console_network_card_uses_ap_sta_tabs_with_ssid_and_ip():
    source = firmware_source_text()

    assert 'id="networkApTab"' in source
    assert 'id="networkStaTab"' in source
    assert 'id="networkSsidValue"' in source
    assert 'id="networkMdnsValue"' not in source
    assert 'id="networkIpValue"' not in source
    assert 'id="networkSub"' not in source
    assert '<b data-i18n="state.ssid">SSID</b><span id="networkSsidValue">--</span>' in source
    assert '<b>LAN</b><span id="networkMdnsValue" onclick="openNetworkLanUrl()">--</span>' not in source
    assert '<b data-i18n="state.remain">REMAIN</b><span id="voltageSub">--</span>' in source
    assert 'onclick="event.stopPropagation();openNetworkSettings()"' in source
    # v1.7.42：无可复制 IP 时去掉 copyValue，悬停不再出现"点击复制 IP"；
    # v1.7.44 起统一抽为 netIpValid()（含大写 Disabled——固件 ap_ip 直报 Disabled）
    assert "function netIpValid(v){return !!v&&v!=='--'&&v!=='0.0.0.0'&&v.toLowerCase()!=='disabled'}" in source
    assert "networkValue.classList.toggle('copyValue',netIpValid(networkCopyIp))" in source
    # v1.7.44：AP Disabled（STA-only 关 AP）与 HOST 未上报时卡片边框与小点变红（#ff6b6b）
    assert ".netDown{border-color:#ff6b6b}" in source
    assert ".netDown .stateDot{background:#ff6b6b}" in source
    assert "'stateCard '+(hostIp?'mode0':'netDown')" in source
    assert "'stateCard '+(netIpValid(ap)?'mode0':'netDown')" in source
    assert "'stateCard '+(hostIp?'mode0':'driftOff')" not in source
    assert "networkCard.className='stateCard mode0'" not in source
    assert '<button class="gear" onclick="event.stopPropagation();openWifiStaModal()">' not in source
    assert 'ap_ssid=\\"%s\\"' in source
    assert 'sta_ssid=\\"%s\\"' in source
    assert "networkTabPinned" in source
    assert "staConnected?'sta':'ap'" in source
    assert ".netTabs{position:absolute;right:28px;top:8px;" in source
    assert "networkSub.textContent" not in source
    assert "networkIpValue.textContent" not in source
    assert "networkMdnsValue" not in source
    assert "openNetworkLanUrl" not in source
    assert ".local 打不开时请使用 STA IP" not in source
    assert "LAN 名称不可用，请使用 STA IP" not in source
    assert "v.toFixed(1)+'V'" in source
    assert "if(!isNaN(v)&&v>=5)" in source
    assert "voltageValue.textContent='--'" in source
    assert "voltageSub.textContent='--'" in source
    assert "if(!isNaN(v)&&v>0)" not in source
    assert "v.toFixed(2)+'V'" not in source


def test_web_console_ap_ssid_modal_and_api_are_present():
    source = firmware_source_text()

    assert 'id="wifiApModal"' in source
    assert 'AP 名称配置' in source
    assert 'id="apSsid"' in source
    assert '保存并重启 AP' in source
    assert 'openNetworkSettings()' in source
    assert 'openWifiApModal()' in source
    assert "selected==='ap'?openWifiApModal():openWifiStaModal()" in source
    assert "fetch('/api/wifi-ap')" in source
    assert "fetch('/api/wifi-ap',{method:'POST'" in source
    assert 'wifiWebServer.on("/api/wifi-ap", HTTP_GET, handleWifiWebAp)' in source
    assert 'wifiWebServer.on("/api/wifi-ap", HTTP_POST, handleWifiWebApSet)' in source
    assert 'MUS4_PREF_AP_SSID_KEY' in source
    assert 'wifiApSsid' in source
    assert 'scheduleWifiApRestart()' in source
    assert 'restartWifiAp()' in source
    assert 'WIFI_CONSOLE_AP_DEFAULT_SSID' in source
    assert 'WIFI_CONSOLE_AP_SSID' not in source


def test_wifi_ap_ssid_is_restricted_to_mdns_safe_hostname():
    source = firmware_source_text()
    identity_header = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiIdentity.h").read_text(encoding="utf-8")
    identity_source = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiIdentity.cpp").read_text(encoding="utf-8")
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "bool isMdnsSafeHostnameChar(char c)" in identity_header
    assert "bool isMdnsSafeHostname(const String& value)" in identity_header
    assert "bool copyWifiApSsid(const String& ssid)" in identity_header
    assert "bool isValidApSsidPrefix(const String& value)" in identity_header
    assert "String wifiMdnsHostText()" in identity_header
    assert "String wifiMdnsUrlText()" in identity_header
    assert "#include \"WifiIdentity.h\"" in sketch_source
    assert "if (!isMdnsSafeHostname(ssid)) return false;" in identity_source
    assert "bool isValidApSsidPrefix" in identity_source
    assert "WIFI_IDENTITY_AP_SSID_MAX_LEN = 32" in identity_source
    assert "c >= 'A' && c <= 'Z'" in identity_source
    assert "c >= 'a' && c <= 'z'" in identity_source
    assert "c >= '0' && c <= '9'" in identity_source
    assert "c == '-'" in identity_source
    assert "value[0] == '-'" in identity_source
    assert "value[value.length() - 1] == '-'" in identity_source
    assert "host.toLowerCase()" in identity_source
    assert "String(\"http://\") + wifiMdnsHostText() + \".local/\"" in identity_source
    assert "static bool isMdnsSafeHostname" not in sketch_source
    assert "static String wifiMdnsHostText" not in sketch_source
    assert "前缀只能使用大小写字母和数字" in source
    assert "invalid_ssid" in source


def test_wifi_ap_ssid_prefix_is_limited_to_six_chars():
    """v1.7.22 起：AP/STA 互斥切换下 AP 与 STA 不会同时广播，派生 SSID 失去意义，
    getActiveWifiApSsid() 简化为直接返回基础 wifiApSsid；
    `buildWifiDevApSsid` / `wifiStaSsidShortUpper` / `wifiStaIpTailText` 三个
    辅助函数与对应文案一并退役。"""

    wifi_types = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h").read_text(encoding="utf-8")
    manager_header = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.h").read_text(encoding="utf-8")
    manager_source = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.cpp").read_text(encoding="utf-8")
    assets_source = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(encoding="utf-8")
    source = firmware_source_text()

    # 基础 AP SSID 命名规则保留：6 位前缀 + "-ESP" 后缀
    assert 'const uint8_t WIFI_AP_SSID_PREFIX_MAX_LEN = 6;' in wifi_types
    assert 'const char* WIFI_AP_SSID_SUFFIX = "-ESP";' in wifi_types
    assert "String getActiveWifiApSsid()" in manager_header
    assert "String getActiveWifiApSsid()" in manager_source

    # 派生函数与派生分支均被移除
    assert "buildWifiDevApSsid" not in manager_source
    assert "wifiStaSsidShortUpper" not in manager_source
    assert "wifiStaIpTailText" not in manager_source
    assert "if (!wifiStaConnected) return String(wifiApSsid);" not in manager_source

    # 前端输入校验保留
    assert 'maxlength="6"' in assets_source
    assert '/^([A-Za-z0-9]{1,6})$/' in assets_source
    assert "长度为 1-6 位" in assets_source

    # UI 文案中派生说明已删除
    assert "前 3 位大写" not in source
    assert "last two IP octets" not in source
    assert "追加 STA" not in source
    assert "appends the first 3 uppercase chars of STA SSID" not in source


def test_web_console_exposes_ap_name_mdns_lan_console_entry():
    source = firmware_source_text()

    assert "#include <ESPmDNS.h>" in source
    assert "bool& wifiMdnsStarted" in source
    assert "void startWifiMdnsIfNeeded()" in source
    assert "void stopWifiMdnsIfNeeded()" in source
    assert "String wifiMdnsHostText()" in source
    assert "String wifiMdnsUrlText()" in source
    assert "MDNS.begin(wifiMdnsHostText().c_str())" in source
    assert 'MDNS.addService("http", "tcp", WIFI_WEB_CONSOLE_PORT)' in source
    assert "ESP.getEfuseMac" not in source


def test_web_status_and_sta_api_include_ap_name_mdns_console_url():
    source = firmware_source_text()

    status_body = re.search(
        r"void printWirelessStatus\(Print& out\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    sta_json_body = re.search(
        r"static String wifiStaJson\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")

    assert "mdns_host=\\\"%s\\\"" in status_body
    assert "mdns_url=%s" in status_body
    assert "mdns_started=%d" in status_body
    assert "web_log_dropped=%lu" in status_body
    assert "wifiMdnsHostText().c_str()" in status_body
    assert "wifiMdnsUrlText().c_str()" in status_body
    assert "wifiRuntime.mdnsStarted ? 1 : 0" in status_body
    assert "webLogBufferDropped()" in status_body
    assert "\\\"mdns_host\\\"" in sta_json_body
    assert "\\\"mdns_url\\\"" in sta_json_body
    assert "\\\"mdns_started\\\"" in sta_json_body
    assert "wifiMdnsHostText().c_str()" in sta_json_body
    assert "wifiMdnsUrlText().c_str()" in sta_json_body
    assert "host.toLowerCase()" in source
    assert "String(\"http://\") + wifiMdnsHostText() + \".local/\"" in source


def test_wifi_sta_to_sta_handoff_keeps_ap_as_transition_page():
    """v1.7.18 起 AP/STA 互斥切换：STA→STA 切换不再走旧的 handoff 三态共存路径——
    新保存的 SSID 由 applyWifiStaCredentials() 走「短暂回到 AP_STA → 1s grace 后切
    STA_ONLY」的统一链路。下列三个接口被保留为 no-op，只为减小调用方扰动。"""

    source = firmware_source_text()

    assert "WIFI_STA_HANDOFF_AP_KEEP_MS" not in source
    assert "WIFI_AP_STOP_AFTER_STA_CONNECTED_DELAY_MS" not in source
    assert "bool& wifiStaHandoffActive" in source
    assert "bool& wifiStaApplyFromAp" in source
    assert "char* const wifiStaHandoffTargetSsid" in source
    assert "void startWifiStaHandoff" in source
    assert "void finishWifiStaHandoff" in source
    assert "void clearWifiStaHandoff" in source

    handoff_body = re.search(
        r"void startWifiStaHandoff\(const String& /\*targetSsid\*/\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    # handoff 已退役 → no-op，只调用 clearWifiStaHandoff() 清残留字段
    assert "clearWifiStaHandoff()" in handoff_body
    assert "ensureWifiApAvailable()" not in handoff_body
    assert "restartWifiAp()" not in handoff_body

    # 调用方仍保留，HTTP POST 时按需触发但实际只是 no-op；source 仅作为后端 AP 判定的提示。
    assert "function wifiStaSaveSource()" in source
    assert "body.set('source',wifiStaSaveSource())" in source
    assert "wifiWebServer.arg(\"source\")" in source
    assert "startWifiStaHandoff(ssid)" in source


def test_wifi_sta_handoff_status_api_and_web_prompt_are_present():
    """v1.7.18 起：JSON 的 handoff_* 字段保留以避免前端解析报错；
    modal UI 复用为 STA 成功后的局域网访问提示，不恢复旧 handoff 状态机。"""

    source = firmware_source_text()
    sta_json_body = re.search(
        r"static String wifiStaJson\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")

    assert "handoff_active" in sta_json_body
    assert "handoff_target_ssid" in sta_json_body
    assert "handoff_sta_ip" in sta_json_body
    assert "handoff_ap_ssid" in sta_json_body
    assert "handoff_ap_url" in sta_json_body
    assert "handoff_mdns_url" in sta_json_body
    assert "http://192.168.4.1/" in source
    assert "恢复 AP" in source
    assert "打开新地址" in source
    assert "复制地址" in source



def test_web_console_sta_success_shows_lan_url_before_ap_closes():
    source = firmware_source_text()

    wait_body = re.search(
        r"async function waitWifiStaConnectionResult\(\)\{(?P<body>.*?)\}\nasync function saveWifiSta",
        source,
        re.DOTALL,
    ).group("body")
    handoff_url_body = re.search(
        r"function handoffStaUrl\(j\)\{(?P<body>.*?)\}\nfunction showWifiStaHandoffModal",
        source,
        re.DOTALL,
    ).group("body")
    handoff_modal_body = re.search(
        r"function showWifiStaHandoffModal\(j\)\{(?P<body>.*?)\}\nasync function copyHandoffIp",
        source,
        re.DOTALL,
    ).group("body")
    copy_body = re.search(
        r"async function copyHandoffIp\(\)\{(?P<body>.*?)\}\nasync function openHandoffUrl",
        source,
        re.DOTALL,
    ).group("body")

    assert "if(j&&j.connected)" in wait_body
    assert "j.sta_ip&&j.sta_ip!=='0.0.0.0'" in wait_body
    assert "showWifiStaHandoffModal(j)" in wait_body
    assert "redirectToStaConsole(j.sta_ip)" not in wait_body
    assert "if(j.handoff_active){showWifiStaHandoffModal(j);return true}" not in wait_body
    assert "staNotice.textContent=t('wifi.staConnected')+'\\n'+t('wifi.handoffLanIp')+j.sta_ip+'\\n'+t('wifi.handoffUrl')+'http://'+j.sta_ip+'/'" in wait_body
    assert "wifiStaModal.classList.remove('show')" not in wait_body
    assert "closeWifiStaModal();return true" not in wait_body
    assert "await new Promise(resolve=>setTimeout(resolve,800))" in wait_body
    assert "await new Promise(resolve=>setTimeout(resolve,1000))" not in wait_body

    assert "(j&&j.sta_ip)||(j&&j.handoff_sta_ip)||''" in handoff_url_body
    assert "'http://'+ip+'/'" in handoff_url_body

    # v1.7.46：handoff 弹窗文案走 i18n（t()），中英文案在 I18N 字典
    assert "t('wifi.handoffLanIp')" in handoff_modal_body
    assert "t('wifi.handoffUrl')" in handoff_modal_body
    assert "t('wifi.handoffSwitch')" in handoff_modal_body
    assert "t('wifi.handoffConnecting')" in handoff_modal_body
    assert "t('wifi.handoffHint')" in handoff_modal_body
    assert "I18N.zh['wifi.handoffSwitch']='请将电脑/手机切换到 Wi-Fi：'" in source
    assert "I18N.zh['wifi.handoffLanIp']='局域网 IP：'" in source
    # 带 IP AP 名新方案：引导去 Wi-Fi 列表看 MUS4-<设备IP>，不再让用户连 AP 开 192.168.4.1
    assert "MUS4-" in source
    assert "Wi-Fi 列表" in source
    assert "恢复 AP" in source
    assert "连接设备 AP" not in handoff_modal_body
    assert "192.168.4.1" not in handoff_modal_body

    assert 'data-i18n="button.copyUrl"' in source
    assert "'button.copyUrl':'复制地址'" in source
    assert "'button.copyUrl':'Copy URL'" in source
    assert "'toast.copiedUrl':'已复制地址：'" in source
    assert "'toast.copiedUrl':'Copied URL: '" in source
    assert "navigator.clipboard.writeText(url)" in copy_body
    assert "fallbackCopyText(url)" in copy_body
    assert "t('toast.copiedUrl')+url" in copy_body


def test_web_console_header_ota_button_and_log_area_are_compact():
    source = firmware_source_text()
    assets_source = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(encoding="utf-8")
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "static const char WIFI_WEB_UPDATE_HTML[] PROGMEM" in assets_source
    assert "static const char WIFI_WEB_JUDGE_HTML[] PROGMEM" in assets_source
    assert "WebSocket first / pseudoSpeed monitor-first scoring" in assets_source
    assert "MUS4 HTTP OTA" in assets_source
    assert "static const char WIFI_WEB_UPDATE_HTML[] PROGMEM" not in sketch_source
    assert source.index('<section class="panel" id="chartPanel">') < source.index('<section class="panel" id="serialPanel">')
    assert '<section class="panel" id="serialPanel">' in source
    assert "#serialPanel{display:flex;flex-direction:column;gap:8px;padding-bottom:6px}" in source
    assert "#serialPanel .log{flex:1 1 auto;min-height:calc(5 * 1.35em + 16px);max-height:calc(20 * 1.35em + 16px)}" in source
    assert "@media(min-width:900px){.grid{grid-template-columns:minmax(0,2fr) minmax(0,1fr)}.wide{grid-column:1/-1}#diagnosticsPanel{grid-column:1/-1}}" in source
    # Issue #90：grid 列必须 minmax(0,…) 可收缩，否则终端标签条永远不会溢出、智能缩写不触发
    assert ".grid{display:grid;grid-template-columns:minmax(0,1fr);gap:10px}" in source
    assert "grid-template-columns:2fr 1fr}" not in source
    assert "canvas{width:100%;height:auto;aspect-ratio:38/13;" in source
    assert "#chartPanel:fullscreen .chartCanvasWrap{width:min(100%,calc((100vh - 118px) * 38 / 13))}" in source
    assert "#chartPanel:fullscreen canvas{width:100%;height:auto;max-height:calc(100vh - 118px);aspect-ratio:38/13}" in source
    assert '<div class="chartCanvasWrap">' in source
    assert '.chartCanvasWrap{position:relative}' in source
    assert '#chartFullscreenBtn{position:absolute;right:8px;bottom:8px;z-index:2}' in source
    assert "dataMeta.textContent=transport+' realtime seq='+lastDataSeq+' +'+added" not in source
    assert "dataMeta.textContent=transport+' +'+added" not in source
    assert 'id="dataMeta"' not in source
    assert "data ready" not in source
    assert "dataMeta" not in source
    assert "@media(min-width:900px){.grid{grid-template-columns:2fr 1fr}.wide{grid-column:1/-1}}" not in source
    assert "@media(min-width:900px){.grid{grid-template-columns:1fr 2fr}.wide{grid-column:1/-1}}" not in source
    assert '.chartFooter{display:flex;gap:10px;align-items:center;justify-content:space-between;flex-wrap:wrap;margin-top:8px}.chartToolbar{display:flex;gap:6px;align-items:center}.chartTools{display:flex;gap:6px;flex-wrap:wrap;justify-content:flex-end;margin-top:8px}' in source
    assert '.chartControls' not in source
    assert 'class="iconButton" onclick="toggleChart()" id="chartBtn" title="暂停"' in source
    assert 'class="iconButton" onclick="clearChart()" title="清空"' in source
    assert 'class="iconButton" onclick="toggleChartFullscreen()" id="chartFullscreenBtn" title="全屏"' in source
    assert 'class="iconButton" onclick="toggleTub()" id="tubRecordBtn"' in source
    assert 'class="iconButton" onclick="td()" id="tubDownloadBtn"' in source
    assert 'ICON_RECORD=' in source
    assert 'ICON_RECORDING=' in source
    assert 'ICON_DOWNLOAD=' in source
    assert 'tubRecordBtn.classList.toggle' in source
    assert 'id="tubMeta"' in source
    assert 'class="recMeta"' in source
    assert 'function updateTubMeta()' in source
    assert 'tubMeta.textContent=tubSamples.length' in source
    assert '<span class="recMeta"><span data-i18n="tub.recorded">录制量</span><b id="tubMeta">0</b></span>' in source
    assert 'function clearChart(){pointHead=0;pointCount=0;points.fill(null);scrollOffset=0;smoothedDt=16;gridReady=false;tubSamples=[];tubStartedMs=0;tubStoppedMs=0;tubLastSeq=0;tubRecording=false;updateTubMeta();draw()}' in source
    assert "c.innerHTML=chartPaused?ICON_PLAY:ICON_PAUSE" in source
    assert "f.innerHTML=document.fullscreenElement===chartPanel?ICON_FULLSCREEN_EXIT:ICON_FULLSCREEN" in source
    assert '<button onclick="clearChart()">清空曲线</button>' not in source
    assert '<button onclick="toggleChart()" id="chartBtn">暂停曲线</button>' not in source
    assert "'暂停曲线'" not in source
    assert "'继续曲线'" not in source
    assert "'退出全屏'" not in source
    assert "'全屏曲线'" not in source
    # v1.8.21：OTA 按钮与 DEV 开关恢复至 DC 头部（PR #124 曾移至 DonkeyDrifter 顶栏，现加回）
    assert '<a href="/update" class="otaLink"' in source
    assert 'id="devModeToggle"' in source
    assert '<select id="cmdTarget"><option value="serial">Serial</option><option value="web">Web</option></select><div id="termTabs"></div><button class="iconButton" onclick="addTerminalTab()" id="newTermBtn" title="新建终端" data-i18n-title="terminal.new"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M12 5v14M5 12h14"/></svg></button><button class="iconButton" onclick="togglePause()" id="pauseBtn" title="暂停"><svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="4" width="4" height="16" rx="1"/><rect x="14" y="4" width="4" height="16" rx="1"/></svg></button><button class="iconButton" onclick="sendCmd()" id="sendBtn" title="发送"><svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M2 21l21-9L2 3v7l15 2-15 2v7z"/></svg></button><input id="cmd">' in source
    assert 'placeholder="PING / STATUS / AUTH:mus4-debug / 0:0"' not in source
    assert "input{flex:0 1 180px;min-width:120px;max-width:220px}" in source
    assert "p.innerHTML=logPaused?ICON_PLAY:ICON_PAUSE" in source
    assert '>暂停日志</button>' not in source
    assert "'继续日志'" not in source
    assert "'暂停日志'" not in source
    assert '<button>OTA Upload</button>' not in source
    assert '<button onclick="quick(\'PING\')">PING</button>' not in source
    assert '<button onclick="quick(\'STATUS\')">STATUS</button>' not in source
    assert '<button onclick="quick(\'AUTH:mus4-debug\')">AUTH</button>' not in source
    assert '<button onclick="quick(\'ENABLE_OTA\')">ENABLE_OTA</button>' not in source
    assert '<button onclick="quick(\'OTA_STATUS\')">OTA_STATUS</button>' not in source
    assert '<div class="muted" style="margin:8px 0">开发模式会持久化；Web Console 免 AUTH，但仍保留 Park Locked 安全限制。OTA 传输期间会默认 Park Locked。</div>' not in source
    assert ".log{height:calc(5 * 1.35em + 16px);" in source
    assert ".log{height:280px;" not in source
    assert '<span class="version" id="versionLabel">--</span>' in source
    assert "versionLabel.textContent" in source
    assert source.index('<a class="ghLink"') < source.index('id="versionLabel"') < source.index('id="muteToggle"')
    assert '<div id="log" class="log"></div>' in source
    assert 'id="logMeta"' not in source
    assert "logMeta" not in source
    assert "log ready" not in source
    assert "logMeta.textContent='seq='+lastLogSeq+' dropped='+j.dropped" not in source


def test_web_console_network_ip_click_copies_with_non_blocking_toast():
    source = firmware_source_text()

    assert 'id="networkValue" onclick="copyNetworkIp()"' in source
    assert 'title="点击复制 IP"' not in source
    assert 'id="toast" class="toast"' in source
    assert ".copyValue{cursor:pointer;position:relative}" in source
    assert ".copyValue:hover:after{content:'点击复制 IP';position:absolute;left:72px;top:-26px;background:#111820;border:1px solid #5cc8ff;border-radius:8px;padding:4px 8px;color:#dbeafe;font-size:12px;font-weight:600;white-space:nowrap;pointer-events:none;z-index:4}" in source
    assert ".gear{position:absolute;right:10px;top:32px;width:30px;height:30px;min-width:0;padding:0;border-radius:50%;font-size:16px;line-height:1;z-index:6}" in source
    assert "text-decoration:underline" not in source
    assert "text-decoration-style:dotted" not in source
    assert "text-underline-offset" not in source
    assert ".toast.show" in source
    assert "toastTimer=0" in source
    assert "networkCopyIp" in source
    assert "function showToast" in source
    assert "async function copyNetworkIp()" in source
    assert "navigator.clipboard.writeText" in source
    assert "document.execCommand('copy')" in source
    assert "复制失败，请手动选择 IP" in source
    assert "alert('已复制" not in source


def test_web_console_groups_rc_and_status_into_collapsible_sections():
    source = firmware_source_text()

    state_panel = '<section class="panel wide">'
    chart_panel = '<section class="panel" id="chartPanel">'
    serial_panel = '<section class="panel" id="serialPanel">'
    diagnostics_panel = '<section class="panel wide" id="diagnosticsPanel">'

    assert 'id="rcFold" class="fold"' in source
    assert 'id="statusFold" class="fold"' in source
    assert '<span class="foldIcon">▸</span><span data-i18n="panel.rcChannels">RC Channels</span>' in source
    assert '<span class="foldIcon">▸</span><span data-i18n="panel.statusDetails">STATUS Details</span>' in source
    assert 'aria-expanded="false"><span class="foldIcon">▸</span><span data-i18n="panel.rcChannels">RC Channels</span>' in source
    assert 'id="servoDutyValue"' in source
    assert 'id="escDutyValue"' in source
    assert '.fold:not(.open) .foldBody{display:none}' in source
    assert diagnostics_panel in source
    assert source.index(state_panel) < source.index(chart_panel)
    assert source.index(chart_panel) < source.index(serial_panel)
    assert source.index(serial_panel) < source.index(diagnostics_panel)
    assert source.index(diagnostics_panel) < source.index('id="rcFold" class="fold"')
    assert source.index('id="rcFold" class="fold"') < source.index('id="statusFold" class="fold"')
    assert '@media(min-width:900px){.grid{grid-template-columns:minmax(0,2fr) minmax(0,1fr)}.wide{grid-column:1/-1}#diagnosticsPanel{grid-column:1/-1}}' in source
    assert "function toggleFold(id)" in source
    assert "function renderStatus(t)" in source
    assert "function parseStatusPairs(t)" in source
    assert "statusBox.textContent=t;updateNetworkCard" not in source


def test_web_console_settings_view_shows_rc_channels_panel():
    """Issue #234（v1.8.41）：?settings=1 / ?wifi=1 内嵌视图选择性显示 .grid，
    只露出 Diagnostics 面板里的 #rcFold（RC Channels 校准面板，含中点 Set 与
    油门 Min/Max 滑块），供 DD Car Connector 内嵌同屏调整 RC 校准。"""
    source = firmware_source_text()

    assert 'body.settings .grid,body.wifi .grid{display:block}' in source
    assert 'body.settings .grid>section.panel,body.wifi .grid>section.panel{display:none}' in source
    # 基础规则 #serialPanel{display:flex}（ID 特异性）会穿透 .grid>section.panel 的整藏，需单独按 ID 藏
    assert 'body.settings #serialPanel,body.wifi #serialPanel{display:none}' in source
    assert 'body.settings #diagnosticsPanel,body.wifi #diagnosticsPanel{display:block}' in source
    assert 'body.settings #diagnosticsPanel>div,body.wifi #diagnosticsPanel>div{display:none}' in source
    assert 'body.settings #diagnosticsPanel #rcFold,body.wifi #diagnosticsPanel #rcFold{display:block}' in source
    # 旧的整藏规则已移除（与 display:block 冲突，同特异性后者胜出会导致 grid 仍被藏）
    assert 'body.settings .grid{display:none}' not in source
    assert 'body.wifi .grid{display:none}' not in source
    # settings/wifi 内嵌视图加载时自动展开 RC Channels 折叠面板
    assert "if(location.search.indexOf('settings=1')>=0||location.search.indexOf('wifi=1')>=0)toggleFold('rcFold');" in source
    # settingsView（车辆设置标题 + 调校行）移到 .grid 之前，内嵌视图里排在 RC Channels 面板前
    assert source.index('<div id="settingsView" class="settingsView">') < source.index('<div class="grid">')
    # v1.8.42：wifi 内嵌视图里 AP/STA 两个配网板块融合为单卡片（上卡去底边/底圆角，下卡去顶圆角）
    assert 'body.wifi #wifiApModal .dialog{margin:0;border-bottom:none;border-radius:14px 14px 0 0;padding-bottom:8px;box-shadow:none}' in source
    assert 'body.wifi #wifiStaModal .dialog{margin:0 0 14px;border-radius:0 0 14px 14px;padding-top:4px}' in source


def test_web_console_embedded_main_view_hides_settings_panels():
    """Issue #234 后续（v1.8.44）：DD 内嵌主视图（?embedded=1 且无 settings/wifi 参数）
    隐藏设置类板块/入口——RC Channels 校准面板（#rcFold）、手柄校准/漂移设置按钮行
    （#diagSettingsRow）、Network 卡 ⚙ 配网入口（#networkGear）、Drift 卡 Tune 链接
    （#driftTuneLink）——这些已全部移至 DD Car Connector 的车辆设置。
    规则必须带 :not(.settings):not(.wifi) 守卫，避免误伤 DD Car Connector 的
    settings/wifi 内嵌视图（其 URL 同样带 embedded=1）；车端独立 DC 页面（无参数）不受影响。"""
    source = firmware_source_text()

    guard = 'body.embedded:not(.settings):not(.wifi)'
    assert f'{guard} #rcFold{{display:none}}' in source
    assert f'{guard} #diagSettingsRow{{display:none}}' in source
    assert f'{guard} #networkGear{{display:none}}' in source
    assert f'{guard} #driftTuneLink{{display:none}}' in source
    # 按钮行加了 id 供 CSS 定点隐藏（原本是无 id 的裸 div）
    assert '<div id="diagSettingsRow" style="margin:10px 0">' in source
    # 守卫必须存在：不允许出现不带 :not 的裸 body.embedded 隐藏规则（会误伤 settings/wifi 内嵌视图里的 rcFold）
    assert 'body.embedded #rcFold{display:none}' not in source
    assert 'body.embedded #diagSettingsRow{display:none}' not in source
    assert 'body.embedded #networkGear{display:none}' not in source
    assert 'body.embedded #driftTuneLink{display:none}' not in source


def test_web_console_status_parser_preserves_quoted_values_with_spaces():
    source = firmware_source_text()

    assert "t.trim().split(/\\s+/)" not in source
    assert "while(i<n&&t[i]!==q)" in source
    assert "parseStatusPairs(t).forEach" in source


def test_web_console_status_details_use_responsive_columns():
    source = firmware_source_text()

    assert ".statusTable{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));" in source
    assert "@media(max-width:900px){.statusTable{grid-template-columns:repeat(2,minmax(0,1fr))}}" in source
    assert "@media(max-width:560px){.statusTable{grid-template-columns:1fr}}" in source


def test_web_console_explains_auth_and_park_rejections():
    source = firmware_source_text()

    assert "function explainCommandError(text)" in source
    assert "'error.parkRequired':'当前操作需要 Park Locked。请将 CH3/Park 切到锁定状态后重试。'" in source
    assert "'error.authRequired':'当前操作需要授权。请先 AUTH，或开启 DEV MODE 后重试。'" in source
    assert "请先 AUTH，或开启 DEBUG MODE 后重试" not in source
    assert "alert(msg)" in source


def test_web_console_tub_recorder_is_browser_side_and_reuses_telemetry_points():
    source = firmware_source_text()

    assert "mus4.web_data_point.tub.v2" in source
    assert "tubRecording" in source
    assert "tubSamples" in source
    assert "function ts()" in source
    assert "function te()" in source
    assert "function td()" in source
    assert "function tp(p)" in source
    assert "function toggleTub()" in source
    assert "function updateTubMeta()" in source
    assert "handleDataPayload" in source
    assert "tp(latest)" in source
    assert "TUB_MAX_SAMPLES" in source
    assert "tubRecordBtn" in source
    assert "tubDownloadBtn" in source
    assert "LittleFS" not in source
    assert "SPIFFS" not in source


def test_web_console_sta_refresh_does_not_overwrite_open_modal_input():
    source = firmware_source_text()

    assert "function isWifiStaModalOpen()" in source
    assert "async function refreshWifiSta(forceFill=false)" in source
    assert "if(forceFill||(!isWifiStaModalOpen()&&document.activeElement!==staSsid))" in source
    assert "async function openWifiStaModal()" in source
    assert "await refreshWifiSta(true)" in source
    assert "wifiStaModal.classList.add('show')" in source


def test_web_console_sta_modal_defaults_host_provisioning_on():
    """v1.7.43：STA 配网弹窗默认开启"上位机配网"——绑定新 Wi-Fi 时默认
    同步把凭据发给 Linux 上位机，避免车辆换网后与上位机失联。"""

    source = firmware_source_text()

    assert "document.getElementById('hostWifiToggle').checked=true;onHostWifiToggle()" in source
    assert "document.getElementById('hostWifiToggle').checked=false" not in source
    # v1.7.46：按钮文案走 i18n
    assert "connectBtn.textContent=t('wifi.hostSendBtn')" in source
    assert "I18N.zh['wifi.hostSendBtn']='发送到上位机'" in source
    assert "I18N.en['wifi.hostSendBtn']='Send to host'" in source
    assert "connectBtn.onclick=saveHostWifi" in source


def test_web_console_sta_settings_support_scan_and_password_visibility():
    source = firmware_source_text()

    assert 'id="staNotice"' in source
    assert "注意只能连接2.4G WiFi" in source
    assert "Wi-Fi 列表" in source
    # v1.7.46：STA 结果提示走 i18n（t() 拼接）
    assert "staNotice.textContent=t('wifi.staConnected')+'\\n'+t('wifi.handoffLanIp')+j.sta_ip+'\\n'+t('wifi.handoffUrl')+'http://'+j.sta_ip+'/'" in source
    assert "staNotice.textContent=t('wifi.connectFailed')" in source
    assert ">连接</button>" in source
    assert ">保存并连接</button>" not in source
    assert "保存前请先 AUTH；密码不会回显，凭据会保存到设备 NVS。" not in source
    assert 'id="staSsidSearchBtn"' in source
    assert 'id="wifiScanPopover"' in source
    assert 'id="wifiScanList"' in source
    assert "function openWifiScanPopover" in source
    assert "function closeWifiScanPopover" in source
    assert "async function refreshWifiScan" in source
    assert "function selectWifiSsid" in source
    assert "setInterval(refreshWifiScan,1000)" in source
    assert "fetch('/api/wifi-sta/scan')" in source
    select_body = re.search(
        r"function selectWifiSsid\(ssid,channel\)\{(?P<body>.*?)\}\n",
        source,
        re.DOTALL,
    ).group("body")
    assert "staSsid.value=ssid" in select_body
    assert "staPassword.value=''" in select_body
    assert "staPasswordPlaceholder=false" in select_body
    assert "staPasswordDirty=false" in select_body
    assert "staPasswordVisible=false" in select_body
    assert "staSavedPassword=''" in select_body
    assert "staSavedPasswordKnown=false" in select_body
    assert "updateStaPasswordEye()" in select_body
    assert '<label for="staSsid">SSID</label>' in source
    assert '<label for="staPassword" data-i18n="wifi.passwordLabel">密码</label>' in source
    assert 'id="staPasswordEye"' in source
    assert 'onclick="toggleStaPasswordVisibility()"' in source
    assert "onmousedown=\"showStaPassword()\"" not in source
    assert "onmouseup=\"hideStaPassword()\"" not in source
    assert "ontouchstart=\"showStaPassword()\"" not in source
    assert "ontouchend=\"hideStaPassword()\"" not in source
    assert "staPasswordPlaceholder" in source
    assert "staPasswordDirty" in source
    assert "staPasswordVisible" in source
    assert "staSavedPassword" in source
    assert "staSavedPasswordKnown" in source
    assert "staPassword.value='*'.repeat(Number(j.password_len||0))" in source
    assert "keep_password" in source
    assert "function toggleStaPasswordVisibility" in source
    assert "async function fetchSavedStaPassword" in source
    assert "function maskStaPassword" in source
    assert "function updateStaPasswordEye" in source
    assert "staPasswordEye.textContent=staPasswordVisible?'🙈':'👁'" in source
    assert "fetch('/api/wifi-sta/password')" in source
    assert "staPassword.type='text'" in source
    assert "staPassword.type='password'" in source


def test_web_console_sta_password_endpoint_is_protected_and_public_state_has_no_secret():
    source = firmware_source_text()

    assert "static void handleWifiWebStaPassword()" in source
    assert 'wifiWebServer.on("/api/wifi-sta/password", HTTP_GET, handleWifiWebStaPassword)' in source
    assert "if (!ws.consoleAuthenticated && !ws.devModeEnabled && !isWirelessConsoleAuthDisabled())" in source
    assert "\\\"password_len\\\":" in source
    assert "appendJsonString(response, ws.staPassword)" in source

    public_body = re.search(
        r"static String wifiStaJson\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "password_len" in public_body
    assert "appendJsonString(response, wifiStaPassword)" not in public_body
    assert "\"password\":" not in public_body


def test_web_console_sta_scan_api_uses_async_wifi_scan():
    source = firmware_source_text()

    assert "static void handleWifiWebStaScan()" in source
    assert 'wifiWebServer.on("/api/wifi-sta/scan", HTTP_GET, handleWifiWebStaScan)' in source
    assert "WiFi.scanNetworks(true" in source
    assert "WiFi.scanComplete()" in source
    assert "WiFi.scanDelete()" in source
    assert "WiFi.RSSI" in source
    assert "WiFi.channel" in source
    assert "\\\"rssi\\\":" in source
    assert "\\\"channel\\\":" in source


def test_web_console_closes_ap_after_sta_grace():
    """STA 进入 WL_CONNECTED 后等待 grace，由 updateWifiSta 调用 stopWifiApForStaOnly()
    停用 SoftAP；底层保持 WIFI_AP_STA 不变，避免 AP↔AP_STA 切换重置接口、踢掉客户端。"""

    source = firmware_source_text()
    wifi_types = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h").read_text(encoding="utf-8")

    assert "WIFI_STA_GRACE_UP_MS = 1000" in wifi_types
    assert "WIFI_STA_GRACE_DOWN_MS = 1000" in wifi_types
    assert "WIFI_STA_IP_DISPLAY_MS = 60000" in wifi_types

    stop_body = re.search(
        r"static void stopWifiApForStaOnly\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "WiFi.softAPdisconnect(true)" in stop_body
    # Pre-existing: stopWifiApForStaOnly() switches to WIFI_STA to fully shut down SoftAP.
    assert "WiFi.mode(WIFI_STA)" in stop_body
    assert "wifiWebServer.close()" in stop_body
    assert "wifiWebServer.begin()" in stop_body
    assert "wifiInApOnlyMode = false" in stop_body
    assert "wifiCaptiveDnsServer.stop()" in stop_body
    assert "AP stopped after STA connected" in stop_body

    update_sta_body = re.search(
        r"(?:static )?void updateWifiSta\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    connected_branch = update_sta_body.split("if (status == WL_CONNECTED)", 1)[1].split("if (wifiStaConnected)", 1)[0]
    assert "wifiStaUpGraceDeadlineMs = millis() + WIFI_STA_IP_DISPLAY_MS" in connected_branch
    assert "showStaIpInApName()" in connected_branch
    assert "stopWifiApForStaOnly()" in connected_branch
    assert "finishWifiStaHandoff()" in connected_branch
    # 不应该在连接成功分支里再次调度 AP 重启——AP 即将被关闭，没有重启意义。
    assert "scheduleWifiApRestart" not in connected_branch

    restart_body = re.search(
        r"(?:static )?bool restartWifiAp\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "wifiCaptiveDnsServer.stop()" in restart_body
    assert "WiFi.softAPdisconnect(true)" in restart_body


def test_web_console_keeps_ap_available_when_wifi_sta_connection_fails():
    source = firmware_source_text()

    failure_body = re.search(
        r"(?:static )?void setWifiStaLastError\(const char\* code, const char\* message, bool timedOut\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "wifiApStopPending" not in source
    assert "保留本轮连接的首个失败原因" in failure_body
    assert "if (ws().staLastError[0] != 0) return" in failure_body
    assert "ws().staConnecting = false" in failure_body
    assert "ws().staConnected = false" in failure_body


def test_web_console_redirects_to_sta_ip_after_successful_wifi_sta_connection():
    source = firmware_source_text()

    assert "async function probeStaConsoleUrl(url)" in source
    assert "async function redirectToStaConsole(ip)" in source
    assert "mode:'no-cors'" in source
    assert "cache:'no-store'" in source
    assert "await new Promise(resolve=>setTimeout(resolve,2000))" not in source
    assert "await new Promise(resolve=>setTimeout(resolve,300))" not in source
    assert "setTimeout(()=>{location.href=url},100)" not in source
    assert "redirectToStaConsole(j.sta_ip)" not in source
    assert "j.sta_ip&&j.sta_ip!=='0.0.0.0'" in source
    assert "const url='http://'+ip+'/'" in source
    # v1.7.46：跳转提示走 i18n
    assert "staNotice.textContent=t('wifi.staConnectedIp')+ip+t('wifi.staSwitchAndOpen')+url" in source
    assert "I18N.zh['wifi.staConnectedIp']='STA 已连接，IP：'" in source
    assert "I18N.zh['wifi.staSwitchAndOpen']='，请切换到该 Wi-Fi 后打开 '" in source


def test_web_console_sta_save_defers_wifi_reconnect_until_after_http_response():
    source = firmware_source_text()

    save_body = re.search(
        r"(?:static )?bool saveWifiStaPreference\(const String& ssid, const String& password\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    handler_body = re.search(
        r"static void handleWifiWebStaSet\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")

    assert "applyWifiStaCredentials" not in save_body
    assert "scheduleWifiStaApply()" in handler_body
    assert handler_body.index("wifiWebServer.send(200") < handler_body.index("scheduleWifiStaApply()")
    assert "wifiStaApplyPending" in source
    assert "WIFI_STA_APPLY_DELAY_MS" in source



def test_ap_sta_configuration_keeps_ap_open_long_enough_to_show_ip():
    source = firmware_source_text()
    runtime_header = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "RuntimeState.h").read_text(encoding="utf-8")
    wifi_types = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h").read_text(encoding="utf-8")

    assert "bool staApplyFromAp = false" in runtime_header
    assert "bool& wifiStaApplyFromAp = wifiRuntime.staApplyFromAp" in source
    assert "WIFI_STA_IP_DISPLAY_MS = 60000" in wifi_types

    handler_body = re.search(
        r"static void handleWifiWebStaSet\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    connected_branch = re.search(
        r"if \(status == WL_CONNECTED\) \{(?P<body>.*?)if \(wifiStaUpGraceDeadlineMs != 0",
        source,
        re.DOTALL,
    ).group("body")
    stop_body = re.search(
        r"static void stopWifiApForStaOnly\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    restore_body = re.search(
        r"static void restoreApAfterStaLost\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")

    assert "wifiStaApplyFromAp = sourceArg == \"ap\"" not in handler_body
    request_source_body = re.search(
        r"static bool isWifiWebRequestFromAp\(const String& sourceArg\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "sourceArg == \"ap\"" in request_source_body
    assert "WiFi.softAPIP()" in request_source_body
    assert "IPAddress(0, 0, 0, 0)" in request_source_body
    assert "WiFi.softAPgetStationNum() > 0" in request_source_body
    assert "!ws.staConnected" in request_source_body
    assert "bool requestFromAp = isWifiWebRequestFromAp(sourceArg)" in handler_body
    assert "wifiStaApplyFromAp = requestFromAp" in handler_body
    assert "function wifiStaSaveSource()" in source
    assert "body.set('source',wifiStaSaveSource())" in source
    assert "location.hostname==='192.168.4.1'?'ap':'sta'" not in source
    assert "wifiStaUpGraceDeadlineMs = millis() + WIFI_STA_IP_DISPLAY_MS" in connected_branch
    assert "showStaIpInApName()" in connected_branch
    assert "wifiStaApplyFromAp = false" in stop_body
    assert "wifiStaApplyFromAp = false" in restore_body


def test_sta_ip_is_encoded_into_ap_name_for_ten_seconds():
    source = firmware_source_text()
    wifi_types = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h").read_text(encoding="utf-8")

    assert "WIFI_STA_IP_DISPLAY_MS = 60000" in wifi_types
    assert "WIFI_STA_IP_AP_PREFIX = \"MUS4-\"" in wifi_types
    assert "WIFI_STA_AP_CONFIG_SUCCESS_GRACE_MS" not in wifi_types

    show_body = re.search(
        r"static void showStaIpInApName\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "WiFi.localIP().toString()" in show_body
    assert "String(WIFI_STA_IP_AP_PREFIX)" in show_body
    assert "WiFi.channel()" in show_body
    assert "startWifiApServices(" in show_body

    # getActiveWifiApSsid 在显示窗口返回带 IP 名（override），关闭后恢复基础名
    active_body = re.search(
        r"String getActiveWifiApSsid\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "g_staIpApSsidOverride" in active_body

    stop_body = re.search(
        r"static void stopWifiApForStaOnly\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    restore_body = re.search(
        r"static void restoreApAfterStaLost\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "g_staIpApSsidOverride = \"\"" in stop_body
    assert "g_staIpApSsidOverride = \"\"" in restore_body


def test_ap_sta_configuration_prealigns_softap_to_scan_channel_before_begin():
    source = firmware_source_text()
    runtime_header = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "RuntimeState.h").read_text(encoding="utf-8")
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")

    # 运行时一次性目标信道
    assert "uint8_t staTargetChannel = 0" in runtime_header
    assert "uint8_t& wifiStaTargetChannel = wifiRuntime.staTargetChannel" in sketch_source

    # 前端扫描结果携带 channel 并提交
    assert "let staSelectedChannel=0" in source
    assert "selectWifiSsid(n.ssid,n.channel" in source
    assert "function selectWifiSsid(ssid,channel)" in source
    assert "staSelectedChannel=Number(channel)||0" in source
    assert "body.set('channel',String(staSelectedChannel))" in source
    assert "staSelectedChannel=0" in source

    # 后端读取并仅在 AP 配网路径记录目标信道
    handler_body = re.search(
        r"static void handleWifiWebStaSet\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "wifiWebServer.arg(\"channel\")" in handler_body
    assert "wifiStaTargetChannel = (requestFromAp && targetChannel >= 1 && targetChannel <= 14)" in handler_body

    # SoftAP 启动支持指定 channel
    start_ap_body = re.search(
        r"bool startWifiApServices\(const char\* logPrefix, uint8_t channel\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "WIFI_CONSOLE_CHANNEL" in start_ap_body
    assert "WiFi.softAP(" in start_ap_body
    assert "apChannel" in start_ap_body

    # 预对齐函数在 WiFi.begin 之前
    apply_body = re.search(
        r"(?:static )?void applyWifiStaCredentials\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "prealignWifiApChannelForStaApply()" in apply_body
    assert apply_body.index("prealignWifiApChannelForStaApply()") < apply_body.index("WiFi.begin")

    prealign_body = re.search(
        r"static void prealignWifiApChannelForStaApply\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "wifiStaApplyFromAp" in prealign_body
    assert "wifiStaTargetChannel" in prealign_body
    assert "restartWifiApOnChannel" in prealign_body

    restart_channel_body = re.search(
        r"static bool restartWifiApOnChannel\(uint8_t channel\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "isValidWifiChannel(channel)" in restart_channel_body
    assert "startWifiApServices(\"AP channel prealigned for STA\", channel)" in restart_channel_body

    # 成功关闭 AP 与失败恢复 AP 路径清理目标信道
    stop_body = re.search(
        r"static void stopWifiApForStaOnly\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    restore_body = re.search(
        r"static void restoreApAfterStaLost\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "wifiStaTargetChannel = 0" in stop_body
    assert "wifiStaTargetChannel = 0" in restore_body


def test_web_console_sta_setup_guides_wifi_list_and_drops_mdns_probe():
    source = firmware_source_text()

    # mDNS 跨网探测与 CORS 已移除
    sta_get_body = re.search(
        r"static void handleWifiWebSta\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "Access-Control-Allow-Origin" not in sta_get_body
    assert "Access-Control-Allow-Private-Network" not in sta_get_body
    assert "staProbeMdnsUrl" not in source
    assert "mode:'cors'" not in source

    # 保存提示引导带外查看 Wi-Fi 列表里的 MUS4-<IP>
    save_body = re.search(
        r"async function saveWifiSta\(\)\{(?P<body>.*?)\}\n\s*async function clearWifiSta",
        source,
        re.DOTALL,
    ).group("body")
    # v1.7.46：引导文案走 i18n，Wi-Fi 列表/MUS4-<IP> 在字典值里
    assert "staNotice.textContent=t('wifi.staConnecting')" in save_body
    assert "I18N.zh['wifi.staConnecting']='设备正在连接 Wi-Fi。连上后请在电脑/手机的 Wi-Fi 列表中查看名为 MUS4-<设备IP> 的网络（约 60 秒后该 AP 自动关闭）'" in source


def test_boot_button_long_press_clears_sta_and_restores_ap_without_restart():
    source = firmware_source_text()
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")
    wifi_types = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h").read_text(encoding="utf-8")
    wifi_manager_header = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.h").read_text(encoding="utf-8")

    assert "WIFI_BOOT_RESET_PIN = 0" in wifi_types
    assert "WIFI_BOOT_RESET_HOLD_MS = 3000" in wifi_types
    assert "pinMode(WIFI_BOOT_RESET_PIN, INPUT_PULLUP)" in sketch_source
    assert "updateWifiBootResetButton()" in sketch_source
    assert "bool clearWifiStaAndRestoreAp()" in wifi_manager_header
    assert "void updateWifiBootResetButton()" in wifi_manager_header

    reset_body = re.search(
        r"void updateWifiBootResetButton\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    clear_body = re.search(
        r"bool clearWifiStaAndRestoreAp\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")

    assert "digitalRead(WIFI_BOOT_RESET_PIN)" in reset_body
    assert "WIFI_BOOT_RESET_HOLD_MS" in reset_body
    assert "bootWifiResetPressedAtMs" in source
    assert "bootWifiResetTriggered" in source
    assert "clearWifiStaAndRestoreAp()" in reset_body
    assert "wifiOtaInProgress" in reset_body
    assert "ESP.restart()" not in reset_body
    assert "clearWifiStaPreference()" in clear_body
    assert "restoreApAfterStaLost()" in clear_body
    assert "STA cleared by BOOT long press" in clear_body


def test_web_console_sta_failure_uses_page_modal_and_waits_for_result():
    source = firmware_source_text()

    assert "id=\"wifiStaFailureModal\"" in source
    assert "function showWifiStaFailureModal" in source
    assert "function waitWifiStaConnectionResult" in source
    assert "last_error_message" in source
    assert "STA 连接失败" in source

    save_body = re.search(
        r"async function saveWifiSta\(\)\{(?P<body>.*?)\}\n\s*async function clearWifiSta",
        source,
        re.DOTALL,
    ).group("body")
    wait_body = re.search(
        r"async function waitWifiStaConnectionResult\(\)\{(?P<body>.*?)\}\nasync function saveWifiSta",
        source,
        re.DOTALL,
    ).group("body")
    assert "setTimeout(resolve,1000)" not in save_body
    assert "waitWifiStaConnectionResult()" in save_body
    assert "await refreshWifiSta(true);refreshStatus();await waitWifiStaConnectionResult()" in save_body
    assert "showCommandError(t)" not in save_body
    assert "await refreshStatus();cmd.value=''" in wait_body
    assert "staNotice.textContent=t('wifi.staConnected')+'\\n'+t('wifi.handoffLanIp')+j.sta_ip+'\\n'+t('wifi.handoffUrl')+'http://'+j.sta_ip+'/'" in wait_body
    assert "AP 可能已关闭，STA 可能已连接" not in wait_body
    assert "showWifiStaFailureModal({ssid:staSsid.value.trim(),last_error_message:'AP 可能已关闭" not in wait_body
    assert "Date.now()+17000" not in wait_body
    assert "Date.now()+22000" not in wait_body
    assert "Date.now()+60000" in wait_body
    assert "[sta-debug] poll null, retry" in wait_body
    save_prompt_body = re.search(
        r"async function saveWifiSta\(\)\{(?P<body>.*?)\}\n\s*async function clearWifiSta",
        source,
        re.DOTALL,
    ).group("body")
    # v1.7.46：引导文案走 i18n，Wi-Fi 列表/MUS4-<IP> 在字典值里
    assert "staNotice.textContent=t('wifi.staConnecting')" in save_prompt_body
    assert "Wi-Fi 列表中查看名为 MUS4-" in source
    assert wait_body.index("staNotice.textContent=t('wifi.staConnected')") < wait_body.index("await refreshStatus();cmd.value=''")


def test_wifi_mdns_lifecycle_follows_sta_connection():
    source = firmware_source_text()

    apply_body = re.search(
        r"(?:static )?void applyWifiStaCredentials\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    update_sta_body = re.search(
        r"(?:static )?void updateWifiSta\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    connected_branch = update_sta_body.split("if (status == WL_CONNECTED)", 1)[1].split("if (wifiStaConnected)", 1)[0]
    disconnected_branch = update_sta_body.split("if (wifiStaConnected)", 1)[1].split("if (!wifiStaConnecting)", 1)[0]

    assert "stopWifiMdnsIfNeeded()" in apply_body
    assert "startWifiMdnsIfNeeded()" in connected_branch
    # v1.7.18 起：mDNS 停止迁到 restoreApAfterStaLost()，disconnected_branch
    # 只武装 down grace；grace 通过后由 restore 函数统一停 mDNS。
    restore_body = re.search(
        r"static void restoreApAfterStaLost\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "stopWifiMdnsIfNeeded()" in restore_body
    assert "restoreApAfterStaLost()" in disconnected_branch
    assert "stopWifiApAfterStaConnected" not in source


def test_wifi_console_applies_sta_after_console_is_ready():
    source = firmware_source_text()

    setup_body = re.search(
        r"(?:static )?void setupWifiConsole\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    console_services_body = re.search(
        r"(?:static )?bool startWifiConsoleServices\(const char\* logPrefix\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "setupWifiWebConsole()" in setup_body
    assert setup_body.index("setupWifiWebConsole()") < setup_body.index("startWifiApServices(")
    assert console_services_body.index("wifiCaptiveDnsServer.start(53, \"*\", WiFi.softAPIP())") < console_services_body.index("wifiConsoleServer.begin()")
    assert console_services_body.index("wifiConsoleServer.begin()") < console_services_body.index("wifiWebServer.begin()")
    assert console_services_body.index("wifiWebServer.begin()") < console_services_body.index("wifiConsoleStarted = true")
    assert setup_body.index("startWifiApServices(") < setup_body.index("applyWifiStaCredentials()")


def test_wifi_softap_uses_explicit_ipv4_gateway_configuration():
    source = firmware_source_text()

    restart_body = re.search(
        r"(?:static )?bool restartWifiAp\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    setup_body = re.search(
        r"(?:static )?void setupWifiConsole\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")

    assert "bool configureWifiSoftApNetwork()" in source
    assert "IPAddress apIp(192, 168, 4, 1)" in source
    assert "IPAddress subnet(255, 255, 255, 0)" in source
    assert "WiFi.softAPConfig(apIp, apIp, subnet)" in source
    start_services_body = re.search(
        r"(?:static )?bool startWifiApServices\(const char\* logPrefix, uint8_t channel\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "configureWifiSoftApNetwork()" in start_services_body
    assert start_services_body.index("configureWifiSoftApNetwork()") < start_services_body.index("WiFi.softAP(")
    assert "startWifiApServices(\"AP restarted\")" in restart_body
    assert "startWifiApServices(\"AP started\"" in setup_body


def test_sta_disconnect_restores_ap_after_grace():
    """STA 脱离 WL_CONNECTED 后等待 WIFI_STA_GRACE_DOWN_MS=1000ms，由 updateWifiSta
    调用 restoreApAfterStaLost() 确保 AP 兜底；底层保持 WIFI_AP_STA 不变，避免
    AP↔AP_STA 切换重置接口、踢掉客户端；grace 期间链路恢复则取消重启。"""

    source = firmware_source_text()

    restore_body = re.search(
        r"static void restoreApAfterStaLost\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    update_sta_body = re.search(
        r"(?:static )?void updateWifiSta\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    disconnected_branch = update_sta_body.split("if (wifiStaConnected)", 1)[1].split("if (!wifiStaConnecting)", 1)[0]

    assert "WiFi.mode(WIFI_AP_STA)" in restore_body
    assert "wifiInApOnlyMode = true" in restore_body
    assert "ensureWifiApAvailable()" in restore_body
    assert "esp_wifi_disconnect()" in restore_body
    # 链路恢复路径在 grace 内取消 down deadline
    connected_branch = update_sta_body.split("if (status == WL_CONNECTED)", 1)[1].split("if (wifiStaConnected)", 1)[0]
    assert "STA recovered within grace window" in connected_branch

    assert "wifiStaDownGraceDeadlineMs = millis() + WIFI_STA_GRACE_DOWN_MS" in disconnected_branch
    assert "restoreApAfterStaLost()" in disconnected_branch
    assert "STA link lost, arming down grace" in disconnected_branch

    start_services_body = re.search(
        r"(?:static )?bool startWifiApServices\(const char\* logPrefix, uint8_t channel\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    console_services_body = re.search(
        r"(?:static )?bool startWifiConsoleServices\(const char\* logPrefix\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "WiFi.softAP(" in start_services_body
    assert "wifiCaptiveDnsServer.start(53, \"*\", WiFi.softAPIP())" in console_services_body
    assert "wifiConsoleServer.begin()" in console_services_body
    assert "wifiConsoleServer.setNoDelay(true)" in console_services_body
    assert "wifiWebServer.begin()" in console_services_body
    assert "wifiConsoleStarted = true" in console_services_body


def test_update_wifi_sta_failure_paths_restore_ap():
    """STA 失败三条路径（no_ssid / auth_failed / timeout）以及 WIFI_STA_CLEAR
    路径都必须调 restoreApAfterStaLost() 恢复 AP 兜底；函数内保持 WIFI_AP_STA 不变，
    只启停接口，避免 AP↔AP_STA 切换重置 SoftAP、踢掉配置客户端。"""

    source = firmware_source_text()
    update_sta_body = re.search(
        r"(?:static )?void updateWifiSta\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")

    # 四条收敛路径都必须调 restoreApAfterStaLost()（注释里出现的也算字符串匹配；
    # 这里只保证 ≥4 次，分别由四个独立分支断言精确覆盖位置）。
    assert update_sta_body.count("restoreApAfterStaLost()") >= 4

    # WL_NO_SSID_AVAIL 路径
    no_ssid_block = update_sta_body.split("if (status == WL_NO_SSID_AVAIL)", 1)[1].split("if (status == WL_CONNECT_FAILED)", 1)[0]
    assert 'setWifiStaLastError("no_ssid"' in no_ssid_block
    assert "restoreApAfterStaLost()" in no_ssid_block
    assert no_ssid_block.index("setWifiStaLastError") < no_ssid_block.index("restoreApAfterStaLost")

    # WL_CONNECT_FAILED 路径
    auth_failed_block = update_sta_body.split("if (status == WL_CONNECT_FAILED)", 1)[1].split("if (!wifiStaTimedOut", 1)[0]
    assert 'setWifiStaLastError("auth_failed"' in auth_failed_block
    assert "restoreApAfterStaLost()" in auth_failed_block

    # 超时路径
    timeout_block = update_sta_body.split("if (!wifiStaTimedOut && millis() - wifiStaConnectStartMs", 1)[1]
    assert 'setWifiStaLastError("timeout"' in timeout_block
    assert "restoreApAfterStaLost()" in timeout_block

    # WIFI_STA_CLEAR / staConfigured=false 路径
    cleared_block = update_sta_body.split("if (!wifiStaConfigured)", 1)[1].split("wl_status_t status", 1)[0]
    assert "restoreApAfterStaLost()" in cleared_block
    assert "AP restored after STA cleared" in cleared_block

    # restoreApAfterStaLost() 内不应再写 lastError；错误码由调用方按场景写入。
    restore_body = re.search(
        r"static void restoreApAfterStaLost\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "setWifiStaLastError(" not in restore_body
    assert "esp_wifi_disconnect()" in restore_body
    assert "WiFi.mode(WIFI_AP_STA)" in restore_body
    assert "ensureWifiApAvailable()" in restore_body


def test_runtime_sta_disconnect_does_not_reset_soft_ap():
    source = firmware_source_text()

    disconnect_body = re.search(
        r"(?:static )?void disconnectWifiStaOnly\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    apply_body = re.search(
        r"(?:static )?void applyWifiStaCredentials\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    runtime_clear_body = re.search(
        r"(?:static )?void clearWifiStaRuntimeStateWithoutDisconnect\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    clear_body = re.search(
        r"(?:static )?bool clearWifiStaPreference\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    setup_body = re.search(
        r"(?:static )?void setupWifiConsole\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")

    assert "void disconnectWifiStaOnly()" in source
    assert "esp_wifi_disconnect()" in disconnect_body
    assert "disconnectWifiStaOnly()" in apply_body
    assert "clearWifiStaRuntimeStateWithoutDisconnect()" in clear_body
    assert "disconnectWifiStaOnly()" not in clear_body
    assert "esp_wifi_disconnect()" not in clear_body
    assert "WiFi.disconnect(" not in clear_body
    assert "WiFi.mode(" not in clear_body
    assert "WiFi.softAP(" not in clear_body
    assert "disconnectWifiStaOnly" not in runtime_clear_body
    assert "esp_wifi_disconnect" not in runtime_clear_body
    assert "WiFi.disconnect" not in runtime_clear_body
    assert "WiFi.mode" not in runtime_clear_body
    assert "ws().staConfigured = false" in runtime_clear_body
    assert "ws().staConnected = false" in runtime_clear_body
    assert "ws().staConnecting = false" in runtime_clear_body
    assert "ws().staApplyPending = false" in runtime_clear_body
    assert "clearWifiStaLastError()" in runtime_clear_body
    # v1.7.31：WiFi.disconnect(true, true) 被替换为 WiFi.mode(WIFI_OFF)，
    # 避免擦除 Wi-Fi 驱动层 NVS 配置导致 STA 接口状态异常。
    assert "WiFi.mode(WIFI_OFF)" in setup_body
    assert "WiFi.disconnect(true, true)" not in setup_body


def test_soft_ap_disconnect_is_limited_to_explicit_ap_restart():
    """v1.7.18 起 AP/STA 互斥切换：softAPdisconnect(true) 允许出现两次，
    一次在 restartWifiAp（AP SSID 修改），一次在 stopWifiApForStaOnly
    （STA 稳定后关 AP）。setupWifiConsole、applyWifiStaCredentials、
    updateWifiSta 主体里不应直接出现这条调用。WiFi.mode(WIFI_STA) /
    WiFi.mode(WIFI_AP) 出现是允许的——分别对应 STA_ONLY 与恢复 AP-only。"""

    source = firmware_source_text()

    restart_body = re.search(
        r"(?:static )?bool restartWifiAp\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    setup_body = re.search(
        r"(?:static )?void setupWifiConsole\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    apply_body = re.search(
        r"(?:static )?void applyWifiStaCredentials\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    update_sta_body = re.search(
        r"(?:static )?void updateWifiSta\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    stop_body = re.search(
        r"static void stopWifiApForStaOnly\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")

    assert source.count("WiFi.softAPdisconnect(true)") == 2
    assert "WiFi.softAPdisconnect(true)" in restart_body
    assert "WiFi.softAPdisconnect(true)" in stop_body
    assert "WiFi.softAPdisconnect(true)" not in setup_body
    assert "WiFi.softAPdisconnect(true)" not in apply_body
    assert "WiFi.softAPdisconnect(true)" not in update_sta_body
    assert "WiFi.mode(WIFI_OFF)" not in restart_body
    assert "WiFi.mode(WIFI_OFF)" not in apply_body
    assert "WiFi.mode(WIFI_OFF)" not in update_sta_body


def test_web_console_handles_common_captive_portal_probes_locally():
    source = firmware_source_text()

    assert "#include <DNSServer.h>" in source
    assert "DNSServer wifiCaptiveDnsServer" in source
    assert "wifiCaptiveDnsServer.start" in source
    assert "wifiCaptiveDnsServer.processNextRequest()" in source
    assert "static void redirectWifiWebCaptivePortalToRoot()" in source
    assert "static void handleWifiWebCaptivePortal()" in source
    assert "static void handleWifiWebCaptivePortalRedirectPage()" in source
    assert "static void handleWifiWebCaptivePortalNotFound()" in source
    assert "handleWifiWebWindowsConnectTest" in source
    assert "handleWifiWebWindowsNcsi" in source
    assert "Microsoft Connect Test" in source
    assert "Microsoft NCSI" in source
    assert "String url = String(\"http://\") + WiFi.softAPIP().toString() + \"/\"" in source
    assert "wifiWebServer.sendHeader(\"Location\", url)" in source
    assert "wifiWebServer.send(302, \"text/plain\", \"\")" in source
    assert 'wifiWebServer.on("/connecttest.txt", HTTP_GET, handleWifiWebWindowsConnectTest)' in source
    assert 'wifiWebServer.on("/ncsi.txt", HTTP_GET, handleWifiWebWindowsNcsi)' in source
    assert 'wifiWebServer.on("/redirect", HTTP_GET, handleWifiWebCaptivePortalRedirectPage)' in source
    assert 'wifiWebServer.on("/hotspot-detect.html", HTTP_GET, handleWifiWebCaptivePortal)' in source
    assert 'wifiWebServer.on("/library/test/success.html", HTTP_GET, handleWifiWebCaptivePortal)' in source
    assert 'wifiWebServer.on("/success.txt", HTTP_GET, handleWifiWebCaptivePortal)' in source
    assert 'wifiWebServer.on("/generate_204", HTTP_GET, handleWifiWebCaptivePortal)' in source
    assert 'wifiWebServer.on("/gen_204", HTTP_GET, handleWifiWebCaptivePortal)' in source
    assert 'wifiWebServer.on("/mobile/status.php", HTTP_GET, handleWifiWebCaptivePortal)' in source
    assert 'wifiWebServer.on("/connectivity-check.html", HTTP_GET, handleWifiWebCaptivePortal)' in source
    assert "wifiWebServer.onNotFound(handleWifiWebCaptivePortalNotFound)" in source

    redirect_body = re.search(
        r"static void handleWifiWebCaptivePortalRedirectPage\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "location.replace" not in redirect_body
    assert "http-equiv=\\\"refresh\\\"" not in redirect_body
    assert "WIFI_WEB_CONSOLE_HTML" in redirect_body
    assert "send_P" in redirect_body

    not_found_body = re.search(
        r"static void handleWifiWebCaptivePortalNotFound\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert 'uri.startsWith("/api/")' in not_found_body
    assert 'wifiWebServer.send(404, "application/json", "{\\"error\\":\\"not_found\\"}")' in not_found_body
    assert "redirectWifiWebCaptivePortalToRoot()" in not_found_body


def test_wifi_discovery_compile_switches_exist():
    source = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "FirmwareConfig.h").read_text(encoding="utf-8")
    # v1.7.14 起默认关闭三种主机名发现（mDNS / NetBIOS / LLMNR），避免它们在弱 Wi-Fi
    # 下的查询风暴 + 周期重启对 AsyncWebSocket 的干扰。代码路径与 STATUS 字段全部保留，
    # 通过 DISABLE_WIFI_NAME_DISCOVERY 这一个总开关短路实际启动。
    assert "#define DISABLE_WIFI_NAME_DISCOVERY" in source
    # NetBIOS / LLMNR 编译开关仍存在，但被 DISABLE_WIFI_NAME_DISCOVERY 包住，
    # 默认编译不再生成相关 ISR/包处理代码。
    assert "#ifndef DISABLE_WIFI_NAME_DISCOVERY" in source
    netbios_idx = source.index("#define ENABLE_WIFI_NETBIOS_DISCOVERY")
    llmnr_idx = source.index("#define ENABLE_WIFI_LLMNR_DISCOVERY")
    gate_idx = source.index("#ifndef DISABLE_WIFI_NAME_DISCOVERY")
    assert gate_idx < netbios_idx < llmnr_idx, (
        "NETBIOS / LLMNR define 必须位于 DISABLE_WIFI_NAME_DISCOVERY gate 之内"
    )


def test_wifi_mdns_startup_short_circuits_when_name_discovery_disabled():
    """v1.7.14：startWifiMdnsIfNeeded 在 DISABLE_WIFI_NAME_DISCOVERY 下短路返回，
    保留函数与状态字段 (wifiMdnsStarted=0 / mdns_url="") 让 STATUS 不变。"""

    wifi_source = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.cpp").read_text(encoding="utf-8")
    # 函数仍存在
    assert "void startWifiMdnsIfNeeded()" in wifi_source
    # 短路：在 begin 之前显式 return
    short_circuit_pattern = (
        "#ifdef DISABLE_WIFI_NAME_DISCOVERY\n"
        "    return;\n"
        "#endif"
    )
    assert short_circuit_pattern in wifi_source, "startWifiMdnsIfNeeded 必须在 DISABLE_WIFI_NAME_DISCOVERY 下首行短路"


def test_wifi_netbios_lifecycle_follows_sta_connection():
    source = firmware_source_text()

    apply_body = re.search(
        r"(?:static )?void applyWifiStaCredentials\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    update_sta_body = re.search(
        r"(?:static )?void updateWifiSta\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    connected_branch = update_sta_body.split("if (status == WL_CONNECTED)", 1)[1].split("if (wifiStaConnected)", 1)[0]
    disconnected_branch = update_sta_body.split("if (wifiStaConnected)", 1)[1].split("if (!wifiStaConnecting)", 1)[0]

    assert "stopWifiNetbiosIfNeeded()" in apply_body
    assert "startWifiNetbiosIfNeeded()" in connected_branch
    # v1.7.18 起：NetBIOS 停止迁到 restoreApAfterStaLost()，disconnected_branch
    # 只武装 down grace；grace 通过后由 restore 函数统一停 NetBIOS。
    restore_body = re.search(
        r"static void restoreApAfterStaLost\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "stopWifiNetbiosIfNeeded()" in restore_body
    assert "restoreApAfterStaLost()" in disconnected_branch


def test_wifi_llmnr_lifecycle_follows_sta_connection():
    source = firmware_source_text()

    apply_body = re.search(
        r"(?:static )?void applyWifiStaCredentials\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    update_sta_body = re.search(
        r"(?:static )?void updateWifiSta\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    update_console_body = re.search(
        r"(?:static )?void updateWifiConsole\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    connected_branch = update_sta_body.split("if (status == WL_CONNECTED)", 1)[1].split("if (wifiStaConnected)", 1)[0]
    disconnected_branch = update_sta_body.split("if (wifiStaConnected)", 1)[1].split("if (!wifiStaConnecting)", 1)[0]

    assert "stopWifiLlmnrIfNeeded()" in apply_body
    assert "startWifiLlmnrIfNeeded()" in connected_branch
    # v1.7.18 起：LLMNR 停止迁到 restoreApAfterStaLost()，disconnected_branch
    # 只武装 down grace；grace 通过后由 restore 函数统一停 LLMNR。
    restore_body = re.search(
        r"static void restoreApAfterStaLost\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "stopWifiLlmnrIfNeeded()" in restore_body
    assert "restoreApAfterStaLost()" in disconnected_branch
    assert "processLlmnrPacket()" in update_console_body


def test_wifi_sta_credentials_set_dhcp_hostname():
    source = firmware_source_text()

    apply_body = re.search(
        r"(?:static )?void applyWifiStaCredentials\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")

    assert "WiFi.setHostname(" in apply_body
    assert "wifiMdnsHostText().c_str()" in apply_body
    assert apply_body.index("WiFi.setHostname(") < apply_body.index("WiFi.begin(")


def test_web_data_point_carries_imu_five_axes_for_tub_export():
    """
    刀 1 (v1.7.9)：为 DonkeyDrift 漂移模型数据采集做铺垫，
    WebDataPoint 必须把 mpu6050Data 里已经在采的 gyroX/gyroY/accelX/accelY/accelZ
    透传到 Web 遥测缓冲。结构体扩字段后，sampleWifiWebData 必须把这 5 个 float
    一并写入，否则 HTTP/WS/Tub 三条链路都拿不到完整 IMU 数据。
    """
    wifi_console_types = (
        PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h"
    ).read_text(encoding="utf-8")
    sketch = MUS4_SKETCH.read_text(encoding="utf-8")

    # WebDataPoint 结构体新增 5 轴
    for field in ("float gyroX;", "float gyroY;", "float accelX;", "float accelY;", "float accelZ;"):
        assert field in wifi_console_types, f"WifiConsoleTypes.h 缺少字段 {field}"

    # sampleWifiWebData 把 mpu6050Data 的 5 轴抄进 point
    for assignment in (
        "point.gyroX = mpu6050Data.gyroX;",
        "point.gyroY = mpu6050Data.gyroY;",
        "point.accelX = mpu6050Data.accelX;",
        "point.accelY = mpu6050Data.accelY;",
        "point.accelZ = mpu6050Data.accelZ;",
    ):
        assert assignment in sketch, f"MUS4_FW.ino sampleWifiWebData 缺少赋值 {assignment}"


def test_http_api_data_latest_exposes_imu_five_axes():
    """
    刀 2 (v1.7.10)：/api/data 的 latest 对象必须把 gx/gy/ax/ay/az 五个 float
    缩写键透出，前端 tp(latest) 写入 tubSamples 后，下载的 mus4-tub.json 在
    polling 路径下立刻能携带漂移建模所需的 IMU 通道。
    采用 String(.., 3) 三位小数与现有 gz 保持精度一致。
    """
    server = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp"
    ).read_text(encoding="utf-8")

    state_body = re.search(
        r"appendWifiWebStateJson\(String& response, WebDataPoint& point\)\s*\{(?P<body>.*?)\n\}",
        server,
        re.DOTALL,
    )
    assert state_body, "找不到 appendWifiWebStateJson 函数体"
    body = state_body.group("body")

    for key in ('\\"gx\\":', '\\"gy\\":', '\\"ax\\":', '\\"ay\\":', '\\"az\\":'):
        assert key in body, f"appendWifiWebStateJson 缺少 JSON 键 {key}"

    # 五个新字段都要走 String(..., 3) 三位小数（与 gz 保持一致）
    for field in ("point.gyroX", "point.gyroY", "point.accelX", "point.accelY", "point.accelZ"):
        assert f"String({field}, 3)" in body, f"appendWifiWebStateJson 缺少 String({field}, 3)"

    # 新字段必须出现在 gz 之后、de 之前，保证 JSON 顺序贴近 IMU 块
    assert body.index('\\"gx\\":') > body.index('\\"gz\\":')
    assert body.index('\\"az\\":') < body.index('\\"de\\":')


def test_web_data_point_and_http_latest_expose_pseudo_speed():
    """
    刀 4：为漂移裁判系统提供统一的速度代理量。
    WebDataPoint 必须包含 pseudoSpeed，sampleWifiWebData 必须写入它，
    /api/data latest 也必须把该字段透出给后续 Judge 页面消费。
    """
    wifi_console_types = (
        PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h"
    ).read_text(encoding="utf-8")
    sketch = MUS4_SKETCH.read_text(encoding="utf-8")
    server = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp"
    ).read_text(encoding="utf-8")

    assert "float pseudoSpeed;" in wifi_console_types
    assert "static float computePseudoSpeed()" in sketch
    assert "point.pseudoSpeed = computePseudoSpeed();" in sketch

    state_body = re.search(
        r"appendWifiWebStateJson\(String& response, WebDataPoint& point\)\s*\{(?P<body>.*?)\n\}",
        server,
        re.DOTALL,
    )
    assert state_body, "找不到 appendWifiWebStateJson 函数体"
    body = state_body.group("body")
    assert '\\"pseudoSpeed\\":' in body
    assert 'String(point.pseudoSpeed, 1)' in body


def test_websocket_binary_frame_schema_v2_carries_imu_five_axes():
    """
    刀 3 (v1.7.11)：WS 二进制遥测帧升级到 schema v2，latest 区在 gyroZ 之后追加
    gyroX/gyroY/accelX/accelY/accelZ 五个 float32；缓冲区扩容到 ≥384 B，避免
    header+latest(v2) ≈100B + 8 个点×24B 共 ≈292 B 超过原 256 B 限制。
    前端 decodeBinaryDataPayload 同步升级 version 检查并解出新字段，保证 WS 路径
    下 tp(latest) 也能拿到完整 IMU 通道（Tub 录制不再依赖 polling 路径）。
    """
    telemetry = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebTelemetry.cpp"
    ).read_text(encoding="utf-8")
    assets = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h"
    ).read_text(encoding="utf-8")

    # schema version 1 → 2
    assert "writeU8(2);" in telemetry, "WebTelemetry.cpp 应把 schema version 写为 2"
    assert "writeU8(1);" not in telemetry, "WebTelemetry.cpp 还残留 writeU8(1) 的旧 schema"

    # latest 区追加 5 个 float
    for write in (
        "writeF32(latest.gyroX);",
        "writeF32(latest.gyroY);",
        "writeF32(latest.accelX);",
        "writeF32(latest.accelY);",
        "writeF32(latest.accelZ);",
    ):
        assert write in telemetry, f"WebTelemetry.cpp 缺少 {write}"

    # 缓冲区扩容到 ≥384 B
    capacity_match = re.search(r"wifiWebSocketBinaryPayload\[(\d+)\]", telemetry)
    assert capacity_match, "找不到 wifiWebSocketBinaryPayload 容量声明"
    capacity = int(capacity_match.group(1))
    assert capacity >= 384, f"wifiWebSocketBinaryPayload 容量 {capacity} < 384"

    # 前端解码同步升级
    assert "version!==2" in assets, "前端 decodeBinaryDataPayload 应严格校验 version!==2"
    assert "version!==1" not in assets, "前端仍残留旧的 version!==1 判断"
    # 与原 gz=f32() 一致的逗号链赋值风格
    for token in ("gx=f32()", "gy=f32()", "ax=f32()", "ay=f32()", "az=f32()"):
        assert token in assets, f"前端 decodeBinaryDataPayload 缺少 {token}"
    # 解码后必须注入 latest 对象供 tp(latest) 录制
    for key in (",gx,", ",gy,", ",ax,", ",ay,", ",az,"):
        assert key in assets, f"前端 latest 对象缺少键 {key}"


def test_websocket_and_http_assets_carry_pseudo_speed_for_judge():
    telemetry = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebTelemetry.cpp"
    ).read_text(encoding="utf-8")
    assets = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h"
    ).read_text(encoding="utf-8")
    server = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp"
    ).read_text(encoding="utf-8")
    wifi_manager = (
        PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.cpp"
    ).read_text(encoding="utf-8")
    runtime_state = (
        PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "RuntimeState.h"
    ).read_text(encoding="utf-8")
    wifi_console_types = (
        PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h"
    ).read_text(encoding="utf-8")

    assert "writeF32(latest.pseudoSpeed);" in telemetry
    assert "pseudoSpeed:f32()" in assets
    assert "function connectJudgeSocket()" in assets
    assert "new WebSocket(dataWsUrl())" in assets
    assert "function pollJudgeData()" in assets
    assert 'fetch(\'/api/data?since=\'+lastSeq' in assets
    # v1.7.46 起按钮文案走 i18n
    assert "startBtn.textContent=t('judge.button.stop')" in assets
    assert "I18N.zh['judge.button.stop']='结束计分'" in assets
    assert "I18N.en['judge.button.stop']='Stop Scoring'" in assets
    assert "function resetScore()" in assets
    assert "function updateScore(latest)" in assets
    assert "dim1-fill" in assets
    assert 'fetch(\'/api/judge-config\'' in assets
    assert "function saveJudgeConfig()" in assets
    assert "function resetJudgeConfigToDefault()" in assets
    assert "judgeConfigSummary" in assets
    assert "基础阈值" in assets
    assert "评分参数" in assets
    assert "基础阈值管判定边界，评分参数管评分手感" in assets
    assert "越小越容易触发碰撞。" in assets
    assert "越大越容易因为 pseudoSpeed 波动掉分。" in assets
    assert "已写入设备，设备重启后仍保留；后续样本立即生效" in assets
    assert "已恢复默认值并写回设备，设备重启后仍保留" in assets
    assert "lowestDimension" in assets
    assert "weakestTrend" in assets
    assert "weakestTrendReason" in assets
    assert "当前最低项：" in assets
    assert "最近拖分项：" in assets
    assert "拖分原因：" in assets
    assert "dim1-trend" in assets
    assert "function computeDimensionTrend(values)" in assets
    assert "function refreshScoreBreakdown()" in assets
    assert "function getWeakestTrendReason(key)" in assets
    assert "SCORE_TREND_WINDOW=8" in assets
    assert "速度稳定敏感度" in assets
    assert "大弯阈值" in assets
    assert "collisionThresholdInput" in assets
    assert "bigTurnThresholdInput" in assets
    assert "windowSizeInput" in assets
    assert "collisionPenaltyInput" in assets
    assert "turnSmoothnessWeightInput" in assets
    assert "rangeMatchWeightInput" in assets
    assert "gyroStabilityWeightInput" in assets
    assert "bigTurnStabilityWeightInput" in assets
    assert "speedStabilityWeightInput" in assets
    assert "throttleStabilityWeightInput" in assets
    assert "judgeConfig.collisionPenalty" in assets
    assert "judgeConfig.turnSmoothnessWeight" in assets
    assert "judgeConfig.rangeMatchWeight" in assets
    assert "judgeConfig.gyroStabilityWeight" in assets
    assert "judgeConfig.bigTurnStabilityWeight" in assets
    assert "judgeConfig.speedStabilityWeight" in assets
    assert "judgeConfig.throttleStabilityWeight" in assets
    assert "static void handleWifiWebJudge()" in server
    assert "static void handleWifiWebJudgeConfig()" in server
    assert "static void handleWifiWebJudgeConfigSet()" in server
    assert "static void handleWifiWebJudgeConfigReset()" in server
    assert 'wifiWebServer.on("/judge", HTTP_GET, handleWifiWebJudge);' in server
    assert 'wifiWebServer.send_P(200, "text/html", WIFI_WEB_JUDGE_HTML);' in server
    assert 'wifiWebServer.on("/api/judge-config", HTTP_GET, handleWifiWebJudgeConfig);' in server
    assert 'wifiWebServer.on("/api/judge-config", HTTP_POST, handleWifiWebJudgeConfigSet);' in server
    assert 'wifiWebServer.on("/api/judge-config/reset", HTTP_POST, handleWifiWebJudgeConfigReset);' in server
    assert "JudgeConfig judgeConfig" in runtime_state
    assert "void loadJudgeConfigPreference()" in wifi_manager
    assert "bool saveJudgeConfigPreference(const JudgeConfig& config)" in wifi_manager
    assert "bool resetJudgeConfigPreference()" in wifi_manager
    assert "MUS4_PREF_JUDGE_COLLISION_THRESHOLD_KEY" in wifi_manager
    assert "MUS4_PREF_JUDGE_COLLISION_PENALTY_KEY" in wifi_manager
    assert "turnSmoothnessWeight" in wifi_console_types
    assert "rangeMatchWeight" in wifi_console_types
    assert "gyroStabilityWeight" in wifi_console_types
    assert "bigTurnStabilityWeight" in wifi_console_types
    assert "speedStabilityWeight" in wifi_console_types
    assert "throttleStabilityWeight" in wifi_console_types
    assert '\\"collisionPenalty\\":' in server
    assert '\\"turnSmoothnessWeight\\":' in server
    assert '\\"rangeMatchWeight\\":' in server


def test_tub_schema_bumps_to_v2_with_imu_five_axes():
    """
    刀 4 (v1.7.12)：前端 Tub schema 升级到 v2 显式宣告字段集合扩展，
    避免下游训练脚本误把 v1 与 v2 混在同一批次（v1 缺 gx/gy/ax/ay/az）。
    """
    assets = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h"
    ).read_text(encoding="utf-8")

    assert "TUB_SCHEMA='mus4.web_data_point.tub.v2'" in assets, "前端 TUB_SCHEMA 应升级到 v2"
    assert "TUB_SCHEMA='mus4.web_data_point.tub.v1'" not in assets, "前端仍残留旧版 v1 TUB_SCHEMA"


def test_joystick_cal_modal_handles_auth_and_nack():
    """
    手柄校准浮窗的按钮需要在未认证或 Park 未锁定时给出明确反馈，
    而不是静默失败。
    """
    assets = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h"
    ).read_text(encoding="utf-8")

    assert "function postJoystickCalAction" in assets, "前端缺少 postJoystickCalAction 统一处理"
    assert "function sendAuthCommand" in assets, "前端缺少 sendAuthCommand 认证辅助"
    assert "text.startsWith('NACK')" in assets, "前端应识别 NACK 响应并停止刷新状态"
    assert "prompt(t('cal.prompt.auth'))" in assets, "未认证时应提示用户输入 AP 密码"
    assert "showCommandError(text)" in assets, "NACK 响应应通过 showCommandError 提示用户"
    assert "I18N.zh['cal.prompt.auth']" in assets, "缺少中文 cal.prompt.auth 提示文案"
    assert "I18N.en['cal.prompt.auth']" in assets, "缺少英文 cal.prompt.auth 提示文案"
    assert "I18N.zh['error.authRequired']" in assets, "缺少中文 error.authRequired 错误文案"
    assert "I18N.en['error.authRequired']" in assets, "缺少英文 error.authRequired 错误文案"


def test_drift_settings_button_next_to_joystick_calibration():
    """
    Diagnostics 面板的"手柄校准"按钮旁需提供"漂移设置"入口，
    方便用户携带当前主题跳转到 ESP32 自身的漂移配置页 /drift。
    """
    assets = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h"
    ).read_text(encoding="utf-8")

    assert "button.driftSettings" in assets, "前端缺少漂移设置按钮的 data-i18n key"
    assert "window.open('/drift?theme='+resolvedTheme(),'_blank')" in assets, "漂移设置按钮应携带当前主题跳转到 ESP32 自身 drift 配置页"
    assert "I18N.zh['button.driftSettings']" in assets, "缺少中文漂移设置文案"
    assert "I18N.en['button.driftSettings']" in assets, "缺少英文漂移设置文案"
    # 漂移设置按钮应与手柄校准按钮位于同一容器（margin:10px 0 的 div）
    cal_btn = assets.index("openJoystickCalModal()")
    drift_btn = assets.index("button.driftSettings")
    assert abs(cal_btn - drift_btn) < 200, "漂移设置按钮应紧邻手柄校准按钮"


def test_drift_page_theme_and_title_hints():
    """漂移调参页（/drift）应跟随控制台深浅色主题，且所有标题采用悬停灰字提示样式：

    - <head> 内有防闪烁主题脚本（读 ?theme= URL 参数，缺省按系统 prefers-color-scheme）。
    - 控制台三处 /drift 入口（Drift 卡 Tune 链接 / Diagnostics 漂移设置按钮 /
      设置视图漂移设置按钮）都携带当前主题参数，实现"跟随 Drifter Console 深浅色"。
    - 漂移页不自带主题切换按钮（v1.8.39 起删除）——主题完全跟随控制台
      ?theme= 参数传递，缺省 auto 跟随系统，内存态不持久化。
    - 大标题（h1）与各级小标题（面板 .label、小节 .sectionTitle）的描述文字改为
      悬停时从标题右侧滑出的灰字提示（.titleHint + .hintSpan），参考 DonkeyDrifter
      的 group-hover 标题提示样式。
    - 「返回 Drifter Console」链接已删除。
    """
    assets = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h"
    ).read_text(encoding="utf-8")
    page = _page_region(assets, "WIFI_WEB_DRIFT_HTML")

    # 主题：防闪烁脚本 + 浅色覆盖 + 内存态 JS（v1.8.39 起无切换按钮，完全跟随控制台 ?theme= 参数）
    assert "document.documentElement.dataset.theme" in page, "漂移页缺少防闪烁主题脚本"
    assert 'html[data-theme="light"] body{' in page, "漂移页缺少浅色主题覆盖"
    assert "function initTheme()" in page, "漂移页缺少 initTheme"
    assert "function readUrlTheme()" in page, "漂移页缺少 ?theme= 参数解析"
    assert "function initTheme(){uiTheme=readUrlTheme()||'auto';applyTheme();" in page, "漂移页 initTheme 应以 ?theme= 参数优先、缺省 auto"
    assert "mus4.ui.theme" not in page, "漂移页主题不应写 localStorage（与控制台一致的内存态）"
    # v1.8.39：漂移页不再自带切换按钮——主题完全跟随 Drifter Console（经 ?theme= 参数传递）
    assert 'id="themeToggle"' not in page, "漂移页不应再有主题切换按钮（应跟随控制台）"
    assert "function toggleTheme()" not in page, "漂移页 toggleTheme 应已删除"
    assert "function setTheme(theme)" not in page, "漂移页 setTheme 应已删除"
    assert "drift.theme.title" not in page, "漂移页主题按钮 i18n 键应已删除"

    # 控制台三处入口携带主题参数
    assert "href=\"/drift\" id=\"driftTuneLink\"" in assets, "Drift 卡 Tune 链接缺少 driftTuneLink id"
    assert "dl.href='/drift?theme='+resolvedTheme()" in assets, "applyTheme 未同步 Tune 链接主题参数"
    assert "window.open('/drift?theme='+resolvedTheme(),'_blank')" in assets
    assert "location.href='/drift?theme='+resolvedTheme()" in assets

    # 标题悬停提示结构
    assert page.count('class="titleHint"') == 4, "漂移页应有 4 处 titleHint（h1/Status/Steering/Throttle）"
    assert page.count('class="hintSpan"') == 4, "漂移页应有 4 处 hintSpan"
    assert ".titleHint:hover .hintSpan{" in page, "缺少悬停展开提示的 CSS"
    assert 'data-i18n="drift.versionTag"' in page, "h1 的版本小字应保留为悬停提示"
    assert 'data-i18n="drift.status.desc"' in page
    assert 'data-i18n="drift.steering.desc"' in page
    assert 'data-i18n="drift.throttle.desc"' in page

    # 返回链接已删除
    assert "drift.backLink" not in page, "返回 Drifter Console 链接及其 i18n 键应已删除"
    assert 'href="/"' not in page, "漂移页不应再有返回首页链接"


def test_ota_closes_websocket_during_upload():
    """
    OTA 窗口打开或 HTTP OTA 上传开始时，应请求主循环关闭并发的 WebSocket
    遥测连接，避免 WS 数据流与 OTA 挤占 AsyncTCP 资源导致传输中断。
    """
    runtime_state = (
        PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "RuntimeState.h"
    ).read_text(encoding="utf-8")
    ota_cpp = (
        PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiOta.cpp"
    ).read_text(encoding="utf-8")
    telemetry_cpp = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebTelemetry.cpp"
    ).read_text(encoding="utf-8")
    server_cpp = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp"
    ).read_text(encoding="utf-8")

    assert "bool closeWsPending = false;" in runtime_state, "OtaRuntimeState 缺少 closeWsPending 标志"
    assert "os.closeWsPending = true;" in ota_cpp, "OTA 窗口打开时应设置 closeWsPending"
    assert "os.closeWsPending = false;" in ota_cpp, "OTA 窗口关闭时应清除 closeWsPending"
    assert "os.closeWsPending = true;" in server_cpp, "HTTP OTA 上传开始时应设置 closeWsPending"
    assert "otaRuntime.closeWsPending" in telemetry_cpp, "WebTelemetry 应消费 closeWsPending 标志"
    assert "wifiWebSocket.closeAll(" in telemetry_cpp, "WebTelemetry 应在主循环中广播关闭所有 WS 客户端"


def test_http_ota_sets_longer_client_timeout():
    """v1.7.27：HTTP OTA 上传开始时把同步 WebServer 客户端 read timeout 从默认
    5000ms 提高到 30000ms，避免 Flash 写入导致 TCP 零窗口期间触发 read timeout。"""
    server_cpp = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp"
    ).read_text(encoding="utf-8")
    upload_body = re.search(
        r"static void handleWifiWebUpdateUpload\(\)\s*\{(?P<body>.*?)\n\}",
        server_cpp,
        re.DOTALL,
    ).group("body")

    assert "upload.status == UPLOAD_FILE_START" in upload_body
    assert "wifiWebServer.client().setTimeout(30000)" in upload_body
    # timeout 设置必须在上传实际开始（Update.begin）之前生效
    assert upload_body.index("setTimeout(30000)") < upload_body.index("Update.begin")


def test_arduino_ota_sets_longer_timeout():
    """v1.7.27：ArduinoOTA 默认 read timeout 仅 1000ms，OTA 期间 Flash 写入容易
    触发 OTA_RECEIVE_ERROR。setupWifiOtaCallbacks 中应设置为 30000ms。"""
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")
    setup_body = re.search(
        r"static void setupWifiOtaCallbacks\(\)\s*\{(?P<body>.*?)\n\}",
        sketch_source,
        re.DOTALL,
    ).group("body")

    assert "ArduinoOTA.setTimeout(30000)" in setup_body


def test_ota_mark_app_valid_cancel_rollback_on_boot():
    """v1.7.28：启动后必须调用 esp_ota_mark_app_valid_cancel_rollback()，
    否则新固件分区长期处于 PENDING_VERIFY，下次 reset 可能被 bootloader
    回滚到旧固件，导致 OTA 反复失败。"""
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")
    setup_body = re.search(
        r"void setup\(\)\s*\{(?P<body>.*?)\n\}",
        sketch_source,
        re.DOTALL,
    ).group("body")

    assert "#include <esp_ota_ops.h>" in sketch_source
    assert "esp_ota_mark_app_valid_cancel_rollback()" in setup_body
    assert "cleanupInvalidOtaPartition()" in setup_body
    assert setup_body.index("cleanupInvalidOtaPartition()") < setup_body.index(
        "esp_ota_mark_app_valid_cancel_rollback()"
    )


def test_http_ota_resets_state_after_failed_upload():
    """v1.7.28：HTTP OTA 上传失败后必须完整清理 otaRuntime 状态并 abort Update
    对象，否则 Update 可能卡在 running 状态，后续 Update.begin() 报
    already running；otaRuntime 标志也会长期占用。"""
    server_cpp = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp"
    ).read_text(encoding="utf-8")
    post_body = re.search(
        r"static void handleWifiWebUpdatePost\(\)\s*\{(?P<body>.*?)\n\}",
        server_cpp,
        re.DOTALL,
    ).group("body")
    upload_body = re.search(
        r"static void handleWifiWebUpdateUpload\(\)\s*\{(?P<body>.*?)\n\}",
        server_cpp,
        re.DOTALL,
    ).group("body")

    assert "resetOtaAfterFailedUpload()" in post_body
    # 错误路径：先清理状态，再发送 500 响应
    assert post_body.index("resetOtaAfterFailedUpload()") < post_body.index(
        "wifiWebServer.send(500"
    )
    # Update.abort() 用于释放 Updater 内部 buffer
    assert "Update.abort()" in server_cpp
    assert "Update.isRunning()" in upload_body
    # 关键状态必须被重置
    assert "os.inProgress = false" in server_cpp
    assert "os.parkGuardActive = false" in server_cpp
    assert "os.closeWsPending = false" in server_cpp


def test_ota_blocks_non_update_handlers_with_503():
    """v1.7.29：Web Console 打开后浏览器轮询会占用 TCP/WebServer 资源，
    HTTP OTA 上传期间应通过 middleware 对非 /update 请求返回 503，
    让浏览器立即释放连接，避免 OTA 被挤占。"""
    server_cpp = (
        PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp"
    ).read_text(encoding="utf-8")
    setup_body = re.search(
        r"void setupWebConsoleServer\(\)\s*\{(?P<body>.*?)\n\}",
        server_cpp,
        re.DOTALL,
    ).group("body")

    assert "addMiddleware" in setup_body
    assert "otaRuntime.inProgress" in setup_body
    assert 'server.uri() != "/update"' in setup_body
    assert "503" in setup_body


def test_ota_button_opens_in_same_tab():
    """v1.7.29：OTA 按钮原本 target=\"_blank\"，导致主页面在后台持续轮询，
    与 OTA 上传竞争资源。应改为当前标签页打开 /update。"""
    assets = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(
        encoding="utf-8"
    )

    # v1.8.21：头部 OTA 链接恢复至 DC（href="/update" 当前标签页打开，无 target=_blank）；
    # OTA 上传表单仍通过 XHR POST /update 在当前页完成
    assert 'href="/update"' in assets
    assert "xhr.open('POST','/update')" in assets
    assert 'target="_blank" class="otaLink"' not in assets


# ============================================================================
# mus4_auth — eFuse 芯片 ID 身份识别服务 源码断言
# ============================================================================

def test_auth_service_compile_switch_enabled():
    """ENABLE_AUTH_SERVICE 宏应在 FirmwareConfig.h 中定义且未注释。"""
    fw_config = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "FirmwareConfig.h").read_text(
        encoding="utf-8"
    )
    # 必须存在未注释的 #define ENABLE_AUTH_SERVICE
    assert re.search(r'^#define ENABLE_AUTH_SERVICE', fw_config, re.MULTILINE)


def test_auth_service_includes_efuse_mac_api():
    """AuthService.cpp 应调用 esp_efuse_mac_get_default() 读取 eFuse MAC。"""
    cpp = (PROJECT_ROOT / "libraries" / "mus4_auth" / "src" / "AuthService.cpp").read_text(
        encoding="utf-8"
    )
    assert "esp_efuse_mac_get_default" in cpp


def test_auth_service_nvs_namespace():
    """AuthService.cpp 应使用独立的 \"auth\" 命名空间存储 user_id。"""
    cpp = (PROJECT_ROOT / "libraries" / "mus4_auth" / "src" / "AuthService.cpp").read_text(
        encoding="utf-8"
    )
    assert '"auth"' in cpp
    assert '"user_id"' in cpp


def test_auth_service_handles_all_four_commands():
    """AuthService.cpp 应处理全部 4 条 Auth 命令。"""
    cpp = (PROJECT_ROOT / "libraries" / "mus4_auth" / "src" / "AuthService.cpp").read_text(
        encoding="utf-8"
    )
    assert "READ_HW_ID" in cpp
    assert "READ_UID" in cpp
    assert "WRITE_UID" in cpp
    assert "CLEAR_UID" in cpp


def test_auth_service_has_error_codes():
    """AuthService.cpp 应包含全部 5 个错误码（01-05）。"""
    cpp = (PROJECT_ROOT / "libraries" / "mus4_auth" / "src" / "AuthService.cpp").read_text(
        encoding="utf-8"
    )
    assert "ERR:01" in cpp  # unknown command
    assert "ERR:02" in cpp  # invalid argument
    assert "ERR:03" in cpp  # NVS write/erase fail
    assert "ERR:04" in cpp  # NVS read fail
    assert "ERR:05" in cpp  # timeout waiting for argument


def test_auth_service_multi_line_state_machine():
    """AuthService.cpp 应有处理 CMD:WRITE_UID + ARG:... 多行协议的状态机。"""
    cpp = (PROJECT_ROOT / "libraries" / "mus4_auth" / "src" / "AuthService.cpp").read_text(
        encoding="utf-8"
    )
    assert "AUTH_WAIT_ARG" in cpp
    assert "AUTH_ARG_TIMEOUT_MS" in cpp


def test_command_dispatcher_includes_auth_service():
    """CommandDispatcher.cpp 应在 ENABLE_AUTH_SERVICE 下调用 processAuthCommand。"""
    cpp = (PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "CommandDispatcher.cpp").read_text(
        encoding="utf-8"
    )
    assert "processAuthCommand" in cpp


def test_command_dispatcher_header_includes_auth():
    """CommandDispatcher.h 应在 ENABLE_AUTH_SERVICE 下包含 AuthService.h。"""
    hdr = (PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "CommandDispatcher.h").read_text(
        encoding="utf-8"
    )
    assert "AuthService.h" in hdr


def test_auth_library_properties_exists():
    """mus4_auth 的 library.properties 应存在且声明 esp32 架构。"""
    props = (PROJECT_ROOT / "libraries" / "mus4_auth" / "library.properties").read_text(
        encoding="utf-8"
    )
    assert "mus4_auth" in props
    assert "architectures=esp32" in props


def test_handle_serial2_includes_auth_service():
    """MUS4_FW.ino 的 handleSerial2() 应在 ENABLE_AUTH_SERVICE 下调用 processAuthCommand。"""
    ino = (PROJECT_ROOT / "MUS4_FW.ino").read_text(encoding="utf-8")
    assert "processAuthCommand" in ino
    # 验证三路路由结构存在（PING → Auth → ECHO）
    assert 'strncmp(line, "CMD:", 4)' in ino
    assert 'strncmp(line, "ARG:", 4)' in ino
    assert 'strncmp(line, "PING,", 5)' in ino


def test_handle_serial2_parses_host_wifi_responses():
    """handleSerial2() 应解析上位机配网响应 STATUS|/OK|/FAIL| 三路。"""
    ino = (PROJECT_ROOT / "MUS4_FW.ino").read_text(encoding="utf-8")
    assert 'strncmp(line, "STATUS|", 7)' in ino
    assert 'strncmp(line, "OK|", 3)' in ino
    assert 'strncmp(line, "FAIL|", 5)' in ino


def test_update_web_console_server_does_not_read_serial2():
    """updateWebConsoleServer() 不应直接读 Serial2。

    配网响应（STATUS|/OK|/FAIL|）解析由 MUS4_FW.ino 的 handleSerial2() 统一负责，
    Web 侧再读一遍会造成双消费者竞争 + readStringUntil 阻塞。
    """
    cpp = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp").read_text(encoding="utf-8")
    m = re.search(r"void updateWebConsoleServer\(\)\s*\{(.*?)^\}", cpp, re.DOTALL | re.MULTILINE)
    assert m, "未找到 updateWebConsoleServer 函数体"
    body = m.group(1)
    assert "Serial2.readStringUntil" not in body, "updateWebConsoleServer 不应用 readStringUntil 读 Serial2"
    assert "Serial2.available" not in body, "updateWebConsoleServer 不应直接读 Serial2.available（由 handleSerial2 统一消费）"


def test_drift_assist_config_is_persisted_and_exposed_via_web_console():
    """DriftAssist 参数可在 Drifter Console 调整并持久化到 NVS。

    - DriftConfig 结构体、默认值、limits 在 WifiConsoleTypes.h 中定义。
    - RuntimeState 持有 driftConfig。
    - WifiManager 提供 load/save/reset 持久化接口。
    - WebConsoleServer 暴露 /api/drift-config GET/POST/reset。
    - WebConsoleAssets 提供 /drift 调参页面及主页面入口。
    - DriftAssist 暴露 apply_drift_throttle。
    - WebDataPoint 与 /api/data latest 携带漂移遥测字段。
    """
    source = firmware_source_text()
    wifi_types = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h").read_text(encoding="utf-8")
    runtime_state = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "RuntimeState.h").read_text(encoding="utf-8")
    wifi_manager_h = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.h").read_text(encoding="utf-8")
    wifi_manager_cpp = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.cpp").read_text(encoding="utf-8")
    server_cpp = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp").read_text(encoding="utf-8")
    assets_h = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(encoding="utf-8")
    drift_h = (PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "DriftAssist.h").read_text(encoding="utf-8")
    sketch = MUS4_SKETCH.read_text(encoding="utf-8")

    # Config structure and defaults
    assert "struct DriftConfig" in wifi_types
    assert "steeringGyroSign" in wifi_types
    assert "maxYawRate" in wifi_types
    assert "defaultDriftConfig()" in source
    assert "isValidDriftConfig(" in source

    # Runtime state
    assert "DriftConfig driftConfig" in runtime_state

    # Persistence
    assert "void loadDriftConfigPreference()" in wifi_manager_h
    assert "bool saveDriftConfigPreference(const DriftConfig& config)" in wifi_manager_h
    assert "bool resetDriftConfigPreference()" in wifi_manager_h
    assert "MUS4_PREF_DRIFT_STEERING_GYRO_SIGN_KEY" in wifi_manager_cpp
    assert "MUS4_PREF_DRIFT_PULSE_DUTY_KEY" in wifi_manager_cpp

    # HTTP API
    assert "static void handleWifiWebDriftConfig()" in server_cpp
    assert "static void handleWifiWebDriftConfigSet()" in server_cpp
    assert "static void handleWifiWebDriftConfigReset()" in server_cpp
    assert 'wifiWebServer.on("/api/drift-config", HTTP_GET, handleWifiWebDriftConfig)' in server_cpp
    assert 'wifiWebServer.on("/api/drift-config", HTTP_POST, handleWifiWebDriftConfigSet)' in server_cpp
    assert 'wifiWebServer.on("/api/drift-config/reset", HTTP_POST, handleWifiWebDriftConfigReset)' in server_cpp

    # Web UI
    assert "WIFI_WEB_DRIFT_HTML" in assets_h
    assert 'href="/drift"' in assets_h
    assert "function saveDriftConfig()" in assets_h
    assert "function resetDriftConfigToDefault()" in assets_h
    assert "fetch('/api/drift-config'" in assets_h
    assert 'id="driftConfigPanel"' not in assets_h  # 使用独立 /drift 页面，避免破坏主页面结构测试

    # DriftAssist throttle control
    assert "int apply_drift_throttle(int driver_throttle)" in drift_h
    assert "int apply_drift_throttle(int driver_throttle)" in source

    # Telemetry fields
    assert "float driftYawError;" in wifi_types
    assert "float driftSteeringCorrection;" in wifi_types
    assert "int8_t driftThrottleMode;" in wifi_types
    assert "point.driftYawError = drift_yaw_error;" in sketch
    assert "point.driftSteeringCorrection = drift_steering_correction;" in sketch
    assert "point.driftThrottleMode = drift_throttle_mode;" in sketch
    assert '\\"dye\\":' in server_cpp
    assert '\\"dsc\\":' in server_cpp
    assert '\\"dtm\\":' in server_cpp

    # Boot load
    assert "loadDriftConfigPreference();" in sketch
    assert "load_drift_config(wifiRuntime.driftConfig);" in sketch



def test_wifi_sta_history_nvs_keys_and_size_constant():
    history_source = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiStaHistory.cpp").read_text(encoding="utf-8")
    wifi_types = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h").read_text(encoding="utf-8")

    # 容量常量与 NVS 命名空间集中定义在 WifiConsoleTypes.h
    assert "static const uint8_t WIFI_STA_HISTORY_SIZE = 5;" in wifi_types
    assert 'static const char* MUS4_PREF_NAMESPACE = "mus4";' in wifi_types

    # 槽位键数组：sta_h{0..4}s 存 SSID、sta_h{0..4}p 存密码（NVS 键长 <= 15 字符）
    assert "static const char* const WIFI_STA_HISTORY_SSID_KEYS[WIFI_STA_HISTORY_SIZE]" in history_source
    assert "static const char* const WIFI_STA_HISTORY_PASS_KEYS[WIFI_STA_HISTORY_SIZE]" in history_source
    for slot in range(5):
        assert f'"sta_h{slot}s"' in history_source
        assert f'"sta_h{slot}p"' in history_source

    # 持久化与加载都走共享 mus4Prefs 的 "mus4" 命名空间
    assert "mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)" in history_source
    assert "mus4Prefs.begin(MUS4_PREF_NAMESPACE, true)" in history_source

    # loadWifiStaHistory 含旧单槽 sta_ssid/sta_pass → 历史槽 0 迁移逻辑
    assert "MUS4_PREF_STA_ENABLED_KEY" in history_source
    assert "MUS4_PREF_STA_SSID_KEY" in history_source
    assert "MUS4_PREF_STA_PASSWORD_KEY" in history_source
    assert "旧单槽配置迁移" in history_source


def test_wifi_sta_history_module_is_split_from_sketch():
    history_header = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiStaHistory.h").read_text(encoding="utf-8")
    history_source = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiStaHistory.cpp").read_text(encoding="utf-8")
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")

    for declaration in [
        "uint8_t wifiStaHistoryCount();",
        "bool copyWifiStaHistorySsid(uint8_t index, String& ssidOut);",
        "bool findWifiStaHistoryEntry(const String& ssid, String& passwordOut);",
        "int8_t wifiStaHistoryRankOf(const String& ssid);",
        "bool recordWifiStaHistory(const String& ssid, const String& password);",
        "bool removeWifiStaHistoryEntry(const String& ssid);",
        "void clearWifiStaHistory();",
        "void loadWifiStaHistory();",
    ]:
        assert declaration in history_header

    for definition in [
        "uint8_t wifiStaHistoryCount()",
        "bool copyWifiStaHistorySsid(uint8_t index, String& ssidOut)",
        "bool findWifiStaHistoryEntry(const String& ssid, String& passwordOut)",
        "int8_t wifiStaHistoryRankOf(const String& ssid)",
        "bool recordWifiStaHistory(const String& ssid, const String& password)",
        "bool removeWifiStaHistoryEntry(const String& ssid)",
        "void clearWifiStaHistory()",
        "void loadWifiStaHistory()",
    ]:
        assert definition in history_source

    # 主 Sketch 只 include 头文件并调用 API，不承载任何实现
    assert '#include "WifiStaHistory.h"' in sketch_source
    for symbol in [
        "uint8_t wifiStaHistoryCount()",
        "bool copyWifiStaHistorySsid(uint8_t index, String& ssidOut)",
        "bool findWifiStaHistoryEntry(const String& ssid, String& passwordOut)",
        "int8_t wifiStaHistoryRankOf(const String& ssid)",
        "bool recordWifiStaHistory(const String& ssid, const String& password)",
        "bool removeWifiStaHistoryEntry(const String& ssid)",
        "void clearWifiStaHistory()",
        "void loadWifiStaHistory()",
        "struct WifiStaHistoryEntry",
        "persistWifiStaHistory",
        "WIFI_STA_HISTORY_SSID_KEYS",
        "WIFI_STA_HISTORY_PASS_KEYS",
    ]:
        assert symbol not in sketch_source


def test_wifi_sta_history_runtime_state_fields():
    runtime_state = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "RuntimeState.h").read_text(encoding="utf-8")

    assert "bool staHistRetryActive = false;" in runtime_state
    assert "unsigned long staHistRetryDeadlineMs = 0;" in runtime_state
    assert "uint8_t staHistTriedMask = 0;" in runtime_state


def test_wifi_sta_history_sketch_hooks_and_clear_cascade():
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")
    sta_source = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiStaConfig.cpp").read_text(encoding="utf-8")
    manager_header = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.h").read_text(encoding="utf-8")
    manager_source = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.cpp").read_text(encoding="utf-8")

    # setup()：loadWifiStaPreference() 之后加载历史；loop()：updateWifiSta() 之后跑历史重试
    load_pref_idx = sketch_source.find("loadWifiStaPreference();")
    load_hist_idx = sketch_source.find("loadWifiStaHistory();")
    assert -1 < load_pref_idx < load_hist_idx
    update_sta_idx = sketch_source.find("updateWifiSta();")
    update_hist_idx = sketch_source.find("updateWifiStaHistoryRetry();")
    assert -1 < update_sta_idx < update_hist_idx

    # 重试状态机经 WifiManager.h 导出
    assert "void updateWifiStaHistoryRetry();" in manager_header

    # WIFI_STA_CLEAR 级联：clearWifiStaPreference() 内同步清空连接历史
    clear_body = re.search(
        r"bool clearWifiStaPreference\(\)\s*\{(?P<body>.*?)\n\}",
        sta_source,
        re.DOTALL,
    ).group("body")
    assert "clearWifiStaRuntimeStateWithoutDisconnect();" in clear_body
    assert "clearWifiStaHistory();" in clear_body

    # updateWifiSta() 连接成功分支把当前凭据记入历史
    update_sta_body = re.search(
        r"(?:static )?void updateWifiSta\(\)\s*\{(?P<body>.*?)\n\}",
        manager_source,
        re.DOTALL,
    ).group("body")
    assert "recordWifiStaHistory(wifiStaSsid, wifiStaPassword);" in update_sta_body


def test_wifi_sta_history_retry_state_machine_preserves_ap():
    manager_source = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.cpp").read_text(encoding="utf-8")

    retry_body = re.search(
        r"(?:static )?void updateWifiStaHistoryRetry\(\)\s*\{(?P<body>.*?)\n\}",
        manager_source,
        re.DOTALL,
    ).group("body")

    # 异步扫描 → 按历史优先级挑未试过的可见条目 → 复用 applyWifiStaCredentials 重连
    assert "WiFi.scanNetworks(true, true)" in retry_body
    assert "WiFi.scanComplete()" in retry_body
    assert "WiFi.scanDelete()" in retry_body
    assert "copyWifiStaHistorySsid" in retry_body
    assert "findWifiStaHistoryEntry(histSsid, histPassword)" in retry_body
    assert "wifiRuntime.staHistTriedMask" in retry_body
    assert "applyWifiStaCredentials();" in retry_body
    assert "WIFI_STA_HISTORY_RETRY_INTERVAL_MS" in retry_body

    # 重试只换 STA 凭据：不得拆 AP、不得关 Wi-Fi（全仓库 softAPdisconnect(true)
    # 计数仍由 test_soft_ap_disconnect_is_limited_to_explicit_ap_restart 守护）
    assert "WiFi.softAPdisconnect(true)" not in retry_body
    assert "WiFi.mode(WIFI_OFF)" not in retry_body


def test_wifi_sta_history_auth_failed_self_heal_unlocks_slot():
    manager_source = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.cpp").read_text(encoding="utf-8")

    retry_body = re.search(
        r"(?:static )?void updateWifiStaHistoryRetry\(\)\s*\{(?P<body>.*?)\n\}",
        manager_source,
        re.DOTALL,
    ).group("body")

    # 连接失败自愈：已配置凭据连接失败（如 NVS sta_pass 过期/写错，WPA2 密码错误
    # 在 ESP32 上多表现为 timeout 而非 auth_failed），而历史中同一 SSID 存有不同
    # 密码时，清除该槽位已试标记，让重试状态机用历史（最近成功）密码再试一次——
    # 否则开机扫描会把该槽位标为已试，历史回退永远被锁死。
    assert "wifiStaLastError[0] != 0" in retry_body
    assert "wifiStaHistoryRankOf(String(wifiStaSsid))" in retry_body
    assert "findWifiStaHistoryEntry(String(wifiStaSsid), histPassword)" in retry_body
    assert "histPassword != String(wifiStaPassword)" in retry_body
    assert "wifiRuntime.staHistTriedMask &= (uint8_t)~(1u << rank);" in retry_body

    # 有界性守卫：自愈分支必须在 connected 上升沿清掩码逻辑之后、重试窗口判定之前
    mask_clear_idx = retry_body.find("wifiRuntime.staHistTriedMask = 0;")
    self_heal_idx = retry_body.find("wifiStaLastError[0] != 0")
    window_idx = retry_body.find("bool inRetryWindow")
    assert -1 < mask_clear_idx < self_heal_idx < window_idx

    # NVS 凭据自愈：连接成功边沿把验证成功的密码同步回 sta_pass（仅当连上的
    # SSID 与 NVS sta_ssid 相同且密码不一致；回退到其它网络时不触碰 NVS）
    assert "healWifiStaPreferenceAfterConnect();" in retry_body
    heal_body = re.search(
        r"static void healWifiStaPreferenceAfterConnect\(\)\s*\{(?P<body>.*?)\n\}",
        manager_source,
        re.DOTALL,
    ).group("body")
    assert "MUS4_PREF_STA_ENABLED_KEY" in heal_body
    assert "MUS4_PREF_STA_SSID_KEY" in heal_body
    assert "MUS4_PREF_STA_PASSWORD_KEY" in heal_body
    assert "nvsSsid != String(wifiStaSsid)" in heal_body
    assert "nvsPass == String(wifiStaPassword)" in heal_body
    assert "saveWifiStaPreference(String(wifiStaSsid), String(wifiStaPassword))" in heal_body


def test_wifi_sta_history_boot_scan_falls_back_to_history():
    manager_source = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.cpp").read_text(encoding="utf-8")

    setup_body = re.search(
        r"(?:static )?void setupWifiConsole\(\)\s*\{(?P<body>.*?)\n\}",
        manager_source,
        re.DOTALL,
    ).group("body")

    # 开机扫描块：已配置 SSID 不可见（或未配置）时，按历史优先级（槽 0 最近）
    # 挑最佳可见条目接管本次开机连接
    assert "if (wifiStaHistoryCount() > 0)" in setup_body
    assert "staHistBootPicked" in setup_body
    assert "findWifiStaHistoryEntry(histSsid, histPassword)" in setup_body
    assert "wifiStaHistoryRankOf(String(wifiStaSsid))" in setup_body
    assert "STA boot: history slot %u" in setup_body


def test_web_console_wifi_sta_history_api():
    server_source = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp").read_text(encoding="utf-8")

    assert "static void handleWifiWebStaHistory()" in server_source
    assert "static void handleWifiWebStaHistoryDelete()" in server_source

    # 路由注册在 /api/wifi-sta/clear 之后
    clear_idx = server_source.find('wifiWebServer.on("/api/wifi-sta/clear", HTTP_POST, handleWifiWebStaClear)')
    history_idx = server_source.find('wifiWebServer.on("/api/wifi-sta/history", HTTP_GET, handleWifiWebStaHistory)')
    delete_idx = server_source.find('wifiWebServer.on("/api/wifi-sta/history/delete", HTTP_POST, handleWifiWebStaHistoryDelete)')
    assert -1 < clear_idx < history_idx < delete_idx

    # 公开 GET：只输出 rank/ssid/password_set 标志，绝不含密码明文
    history_body = re.search(
        r"static void handleWifiWebStaHistory\(\)\s*\{(?P<body>.*?)\n\}",
        server_source,
        re.DOTALL,
    ).group("body")
    assert '{\\"rank\\":' in history_body
    assert '\\"password_set\\":' in history_body
    assert "wifiStaHistoryCount()" in history_body
    assert "copyWifiStaHistorySsid(slot, ssid)" in history_body
    assert "appendJsonString(response, ssid.c_str())" in history_body
    assert "password.c_str()" not in history_body

    # 删除需认证或 devMode（控制台密码为空时免认证）；只移除历史记录，不动当前连接凭据
    delete_body = re.search(
        r"static void handleWifiWebStaHistoryDelete\(\)\s*\{(?P<body>.*?)\n\}",
        server_source,
        re.DOTALL,
    ).group("body")
    assert "if (!ws.consoleAuthenticated && !ws.devModeEnabled && !isWirelessConsoleAuthDisabled())" in delete_body
    assert "403" in delete_body
    assert '\\"error\\":\\"auth_required\\"' in delete_body
    assert 'wifiWebServer.arg("ssid")' in delete_body
    assert "removeWifiStaHistoryEntry(ssid)" in delete_body
    assert "404" in delete_body
    assert '\\"error\\":\\"not_found\\"' in delete_body
    assert '{\\"deleted\\":true' in delete_body


def test_web_console_wifi_sta_history_ui():
    assets = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(encoding="utf-8")

    # STA 弹窗双栏布局：左侧表单列、右侧连接历史列
    assert ".staCols{display:grid;grid-template-columns:1.2fr 1fr;gap:14px;align-items:start}" in assets
    assert 'id="wifiHistoryList"' in assets
    assert 'id="wifiHistoryEmpty"' in assets

    # 历史列表拉取走公开 GET，删除走需认证 POST
    assert "async function refreshWifiHistory()" in assets
    assert "async function deleteWifiHistoryEntry(ssid,isCurrent)" in assets
    assert "fetch('/api/wifi-sta/history')" in assets
    assert "fetch('/api/wifi-sta/history/delete'" in assets

    # i18n 补丁式追加：5 个键各有中英文文案
    for key in [
        "wifi.historyTitle",
        "wifi.historyEmpty",
        "wifi.historyDeleteConfirm",
        "wifi.historyKeepCurrentNote",
        "wifi.historyDelete",
    ]:
        assert f"I18N.zh['{key}']" in assets
        assert f"I18N.en['{key}']" in assets


def test_web_console_mute_toggle_button_ui():
    """v1.7.45：Web Console 头部新增静音切换按钮（muteToggle），SVG 双图标
    （icoSound / icoMute）随静音态切换显隐，aria-label 走 i18n（mute.title），
    前端经 /api/mute 拉取/切换静音状态并渲染按钮。"""
    assets = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(encoding="utf-8")

    # 头部按钮：全页面仅一处，aria-label 走 i18n
    assert assets.count('id="muteToggle"') == 1
    assert 'data-i18n-aria="mute.title"' in assets

    # 图标显隐 CSS：静音态显示 icoMute、隐藏 icoSound
    assert ".muteButton{" in assets
    assert ".muteButton.muted .icoMute{display:block}" in assets
    assert ".muteButton.muted .icoSound{display:none}" in assets
    # v1.8.16：静音键形态统一为 32×32 圆形图标按钮，深色样式与旁边主题按钮逐值一致
    assert ".muteButton{display:inline-flex;align-items:center;justify-content:center;margin-left:auto;width:32px;height:32px;min-width:0;padding:0;border-radius:9999px;background:#111820;border:1px solid #344154;box-shadow:inset 0 0 0 1px #2b3441;color:#b9c5d3;cursor:pointer}" in assets
    assert ".muteButton:hover{color:#e8edf2}" in assets
    assert ".muteButton.muted{background:rgba(92,200,255,.1);border-color:#5cc8ff;box-shadow:inset 0 0 0 1px #5cc8ff;color:#5cc8ff}" in assets

    # i18n 文案：中英文各一条
    assert "'mute.title':'静音'" in assets
    assert "'mute.title':'Mute'" in assets

    # 前端逻辑：静音状态、渲染、初始化与切换
    assert "let uiMuted=false;" in assets
    assert "function renderMuteButton()" in assets
    assert "async function initMute()" in assets
    assert "async function toggleMute()" in assets
    # v1.8.26：内嵌时经 postMessage 即时接收 DD 静音切换，不等 5s 轮询/刷新
    assert "dd-console-mute-changed" in assets
    assert "window.addEventListener('message',function(e)" in assets
    assert "uiMuted=!!d.muted;renderMuteButton()" in assets

    # 启动链：initLanguage()（v1.7.46 起取代直接 applyLanguage）后接 initMute()
    assert "initLanguage();initMute();" in assets


def test_web_console_mute_api_persists_nvs_preference():
    """v1.7.45：/api/mute GET/POST 端点读写静音状态，静音偏好经 Preferences
    持久化到 NVS 命名空间 "webui"、键 "muted"（UChar 0/1）。
    v1.7.48：状态与持久化上移到 mus4_core 的 MutePreference（全系统唯一静音
    数据源，蜂鸣器等发声方共享同一闸门），WebConsoleServer 只保留调用核心
    API 的薄处理器。"""
    server = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp").read_text(encoding="utf-8")
    pref = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "MutePreference.cpp").read_text(encoding="utf-8")

    # 路由注册：GET 查询、POST 设置
    assert 'wifiWebServer.on("/api/mute", HTTP_GET, handleWifiWebMuteGet);' in server
    assert 'wifiWebServer.on("/api/mute", HTTP_POST, handleWifiWebMuteSet);' in server

    # 薄处理器：读写都委托 mus4_core 的 MutePreference API，本地不再镜像状态
    assert '#include "MutePreference.h"' in server
    assert "isSystemMuted()" in server
    assert "saveMutePreference(" in server
    assert "webUiMuted" not in server
    assert "loadWebUiMutePreference" not in server

    # NVS 持久化：Preferences 命名空间 "webui"、键 "muted"（UChar 0/1），缺省不静音
    assert "#include <Preferences.h>" in pref
    assert "static bool systemMuted = false;" in pref
    assert 'prefs.begin("webui", true)' in pref
    assert 'prefs.begin("webui", false)' in pref
    assert 'prefs.getUChar("muted", 0)' in pref
    assert 'prefs.putUChar("muted", muted ? 1 : 0)' in pref


def test_web_console_language_api_persists_nvs_preference():
    """v1.7.46：/api/language GET/POST 端点读写界面语言，语言偏好经 Preferences
    持久化到 NVS 命名空间 "webui"、键 "lang"（String），非法值 400 invalid_value。
    v1.7.68：取值扩展为三态 "auto"/"zh"/"en"，缺省 "auto"（跟随浏览器语言，
    由页面端 navigator.language 解析），显式 zh/en 覆盖自动检测并跨重启保持。"""
    server = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp").read_text(encoding="utf-8")

    # 路由注册：GET 查询、POST 设置
    assert 'wifiWebServer.on("/api/language", HTTP_GET, handleWifiWebLanguageGet);' in server
    assert 'wifiWebServer.on("/api/language", HTTP_POST, handleWifiWebLanguageSet);' in server

    # NVS 持久化：Preferences 命名空间 "webui"、键 "lang"（String），缺省 auto
    assert 'static String webUiLang = "auto";' in server
    assert "static void loadWebUiLanguagePreference()" in server
    assert "static bool saveWebUiLanguagePreference(const String& lang)" in server
    assert 'prefs.getString("lang", "auto")' in server
    assert 'prefs.putString("lang", lang)' in server
    assert 'loadWebUiLanguagePreference();' in server

    # 非法值与错误路径与 mute 同款：缺参/非法 400 invalid_value、写失败 500
    assert 'return lang == "zh" || lang == "en" || lang == "auto";' in server
    assert server.count('\\"error\\":\\"invalid_value\\"') >= 4


def test_web_console_language_auto_detects_browser_language():
    """v1.7.68：设备语言偏好缺省 "auto" 时，四个页面启动后经 navigator.language
    自动选择界面语言（zh 开头→中文，其余→英文），检测结果写入 localStorage 作
    离线兜底；用户手动切换语言仍 POST 显式 zh/en 持久化，覆盖自动检测。"""
    assets = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(encoding="utf-8")

    detect = "function detectBrowserLanguage(){try{return String(navigator.language||'').toLowerCase().indexOf('zh')===0?'zh':'en'}catch(e){return 'zh'}}"
    auto_branch = "else if(j&&j.lang==='auto'){lang=detectBrowserLanguage();writeStoredLanguage(lang)}"
    # 主控制台 + JUDGE/DRIFT/UPDATE 四个页面均为自包含 i18n 核心，逐页断言
    assert assets.count(detect) == 4
    assert assets.count(auto_branch) == 4

    # 主控制台：手动切换仍显式持久化 zh/en（覆盖 auto），不受自动检测影响
    console = _page_region(assets, "WIFI_WEB_CONSOLE_HTML")
    assert "fetch('/api/language?lang='+uiLang,{method:'POST'})" in console
    assert "function setLanguage(lang)" in console


def test_firmware_version_bumped_to_v1_7_47_for_help_modal_donkeydrifter_layout():
    """v1.7.47：Web Console 功能说明弹窗完全模仿 DonkeyDrifter（右下角锚定 + 功能分类小标题 + 幽灵关闭按钮）随版本号 v1.7.47 发布。

    发布标记只校验 CHANGELOG 历史条目，不钉死 build_info 当前版本（否则每次发新版都误红）。"""
    changelog = CHANGELOG.read_text(encoding="utf-8")

    assert "## 2026-08-08 v1.7.47" in changelog


def test_buzzer_mute_gate_blocks_all_sounds():
    """v1.7.48：蜂鸣器全局静音闸门——静音时 startMelody() 拒绝启动任何旋律
    （模式切换、Park 锁/解锁、Wi-Fi AP 启动/关闭、STA 连接/断开提示音全覆盖，
    含开机 AP 启动音）；update() 检测到播放中被静音立即停音并复位状态机。"""
    buzzer = (PROJECT_ROOT / "libraries" / "mus4_ui" / "src" / "Buzzer.cpp").read_text(encoding="utf-8")

    assert '#include "MutePreference.h"' in buzzer

    # startMelody 开头：静音即直接返回，一切旋律（含开机音）不得启动
    start = buzzer.index("void Buzzer::startMelody")
    gate = buzzer.index("if (isSystemMuted()) return;", start)
    assert gate - start < 400

    # update()：播放中被静音 → 立即停音并复位播放状态
    update = buzzer.index("void Buzzer::update()")
    update_body = buzzer[update:update + 500]
    assert "if (isSystemMuted())" in update_body
    assert "stopTone();" in update_body
    assert "_playing = false;" in update_body


def test_mute_preference_loaded_before_wifi_setup():
    """v1.7.48：静音偏好必须在 setupWifiConsole() 之前加载——AP 启动提示音
    在 Wi-Fi 初始化期间播放，加载晚了开机音就关不掉。"""
    sketch = (PROJECT_ROOT / "MUS4_FW.ino").read_text(encoding="utf-8")

    assert '#include "MutePreference.h"' in sketch
    assert "loadMutePreference();" in sketch
    assert sketch.index("loadMutePreference();") < sketch.index("setupWifiConsole();")


def test_firmware_version_bumped_to_v1_7_48_for_mute_function():
    """v1.7.48：静音按钮落地实际静音功能（蜂鸣器全局静音闸门 + 开机前加载偏好，
    静音选择 NVS 持久化、关机重启后恢复）随版本号 v1.7.48 发布。

    发布标记只校验 CHANGELOG 历史条目，不钉死 build_info 当前版本（否则每次发新版都误红）。"""
    changelog = CHANGELOG.read_text(encoding="utf-8")

    assert "## 2026-08-08 v1.7.48" in changelog


def _page_region(assets, marker):
    start = assets.index(f"static const char {marker}[] PROGMEM")
    return assets[start:assets.index(')rawliteral";', start)]


def _page_i18n_keys(page, lang):
    keys = set(re.findall(rf"I18N\.{lang}\['([^']+)'\]", page))
    marker = f"{lang}:{{"
    if marker in page:
        block = page.split(marker, 1)[1]
        block = block.split("en:{")[0] if lang == "zh" else block.split("}}")[0]
        keys |= set(re.findall(r"'([a-z]+\.[A-Za-z.]+)':", block))
    return keys


def test_web_console_sub_pages_follow_device_language():
    """JUDGE/DRIFT/UPDATE 三个子页面与主控制台共享语言偏好：各页内嵌自包含
    i18n 核心（localStorage 键 mus4.ui.lang + 启动时 GET /api/language 恢复设备
    语言），zh/en 字典键完全对齐且各页键统一前缀（judge./drift./ota.）。"""
    assets = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(encoding="utf-8")

    for marker, prefix in [
        ("WIFI_WEB_JUDGE_HTML", "judge."),
        ("WIFI_WEB_DRIFT_HTML", "drift."),
        ("WIFI_WEB_UPDATE_HTML", "ota."),
    ]:
        page = _page_region(assets, marker)
        assert "const LANG_STORAGE_KEY='mus4.ui.lang'" in page
        assert "async function initLanguage()" in page
        assert "fetch('/api/language'" in page
        assert "function applyLanguage(lang)" in page
        assert "function detectBrowserLanguage()" in page
        assert "j.lang==='auto'" in page
        assert page.count("initLanguage()") == 2, f"{marker} initLanguage 应恰好 1 定义 + 1 调用"
        assert "setLanguage" not in page, f"{marker} 不应带语言切换 UI（跟随设备偏好）"

        zh_keys = _page_i18n_keys(page, "zh")
        en_keys = _page_i18n_keys(page, "en")
        assert zh_keys, f"{marker} 缺少 zh 字典"
        assert zh_keys == en_keys, f"{marker} zh/en 键不对齐: {zh_keys ^ en_keys}"
        assert all(k.startswith(prefix) for k in zh_keys), f"{marker} 存在非 {prefix} 前缀键"


def test_web_console_language_switch_rerenders_joystick_cal_status():
    """主控制台切换语言时，手柄校准实时读数（方向/油门）等动态文案必须立即重渲染，
    不能停留在旧语言；静态默认文本为中文快照（默认中文界面）。"""
    assets = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(encoding="utf-8")

    apply_body = re.search(
        r"function applyLanguage\(lang\)\{(?P<body>.*?)\}\n",
        assets,
        re.DOTALL,
    ).group("body")
    assert "refreshJoystickCalStatus()" in apply_body
    assert "方向: -- / -- / -- | 油门: -- / -- / --" in assets


def test_web_console_led_blink_color_selector():
    """Issue #107：删除 LED 闪烁颜色 RGB 切换按键，固定为 RGB 全选（mask=7）且不可修改。
    #ledBlinkTabs 控件、renderLedBlinkTabs/initLedBlink/toggleLedBlinkColor 逻辑、
    /api/led-blink 接口与 NVS 持久化一并移除；ControlMixer 仍读取恒为 7 的
    getLedBlinkMask() 驱动空闲（手动 + Park）三色交替闪烁。"""
    assets = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(encoding="utf-8")
    server = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp").read_text(encoding="utf-8")
    pref = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "LedBlinkPreference.cpp").read_text(encoding="utf-8")
    pref_h = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "LedBlinkPreference.h").read_text(encoding="utf-8")
    mixer = (PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "ControlMixer.cpp").read_text(encoding="utf-8")
    led = (PROJECT_ROOT / "libraries" / "mus4_ui" / "src" / "LedStatus.cpp").read_text(encoding="utf-8")
    sketch = (PROJECT_ROOT / "MUS4_FW.ino").read_text(encoding="utf-8")

    # 前端：RGB 切换控件与相关逻辑全部移除
    assert 'id="ledBlinkTabs"' not in assets
    assert "toggleLedBlinkColor" not in assets
    assert "initLedBlink" not in assets
    assert "renderLedBlinkTabs" not in assets
    assert "uiLedBlinkMask" not in assets
    assert "LED_BLINK_TAB_COLORS" not in assets
    for text in ["'led.title'", "'led.red'", "'led.green'", "'led.blue'"]:
        assert text not in assets

    # 前端不再读写 /api/led-blink
    assert "fetch('/api/led-blink'" not in assets

    # 后端：路由与处理器移除
    assert "/api/led-blink" not in server
    assert "handleWifiWebLedBlinkGet" not in server
    assert "handleWifiWebLedBlinkSet" not in server
    assert "saveLedBlinkPreference" not in server
    assert '#include "LedBlinkPreference.h"' not in server

    # 持久化移除：LedBlinkPreference 不再读写 NVS，恒返回 7
    assert "#include <Preferences.h>" not in pref
    assert "saveLedBlinkPreference" not in pref
    assert "loadLedBlinkPreference" not in pref
    assert "prefs." not in pref
    assert "return 7;" in pref
    assert "loadLedBlinkPreference" not in pref_h
    assert "saveLedBlinkPreference" not in pref_h
    assert "getLedBlinkMask" in pref_h

    # 固件启动不再加载偏好
    assert "loadLedBlinkPreference();" not in sketch

    # 空闲闪烁仍由固定掩码驱动：ControlMixer 读 getLedBlinkMask，LedStatus 应用掩码
    assert '#include "LedBlinkPreference.h"' in mixer
    assert "getLedBlinkMask()" in mixer
    assert "applyLedBlinkMask(mask)" in mixer
    assert "void applyLedBlinkMask(uint8_t mask)" in led
    assert "setLEDToggle(colors[0], CRGB::Black)" in led
    assert "setLEDToggle(colors[0], colors[1])" in led
    assert "setLEDToggle(colors[0], colors[1], colors[2])" in led


def test_web_console_theme_toggle():
    """Issue #93（v1.8.3）：DC 深浅切换改为静音式单图标按钮 #themeToggle
    （形态与位置参照静音按钮 #muteToggle）：单击在深色 ↔ 浅色间来回切换，
    图标反映当前生效主题（深色显月亮，浅色显太阳，由 html[data-theme] 驱动）。
    默认跟随浏览器 prefers-color-scheme：用户从未手动点过 = 'auto'，
    浏览器切换深浅时实时跟随；手动单击只改内存态 uiTheme，不写任何
    localStorage / sessionStorage，刷新即重置为跟随系统。
    原 #themeTabs 三态按钮组（auto/dark/light）已移除，auto 态由
    "未手动切换 = 跟随浏览器"等效替代，renderThemeTabs 等死代码一并清理；
    <head> 内防闪烁内联脚本避免首帧闪深色（不读存储、直接跟随系统）。"""
    assets = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(encoding="utf-8")

    # 头部主题单按钮：主控制台页面仅一处（漂移调参页 v1.8.36 曾加同名按钮，v1.8.39 已删除、改为完全跟随控制台主题），位于中英文切换键左边
    console_page = _page_region(assets, "WIFI_WEB_CONSOLE_HTML")
    assert console_page.count('id="themeToggle"') == 1
    assert assets.index('id="themeToggle"') < assets.index('data-i18n-aria="language.title"')

    # 单图标按钮：形态参照 #muteToggle（单图标 + 单击切换），太阳/月亮双图标
    assert '<button type="button" id="themeToggle" class="themeButton" onclick="toggleTheme()"' in assets
    assert 'data-i18n-aria="theme.title"' in assets
    assert 'class="icoMoon"' in assets
    assert 'class="icoSun"' in assets

    # CSS：v1.8.4 起与 DD 主题按钮逐值一致（DD Tailwind 类经 theme-mus4.css /
    # theme-light.css 重映射后的实际渲染值）：32×32 圆形、深色 #111820 背景 +
    # #344154 边框 + #2b3441 内描边、浅色 #f4f6f9/#ccd5df/#d5dce4；
    # 图标色深色 #b9c5d3（hover #e8edf2）、浅色 #3f4f63（hover #1a2330）
    assert ".themeButton{display:inline-flex;align-items:center;justify-content:center;width:32px;height:32px;min-width:0;padding:0;border-radius:9999px;background:#111820;border:1px solid #344154;box-shadow:inset 0 0 0 1px #2b3441;color:#b9c5d3;cursor:pointer}" in assets
    assert ".themeButton:hover{color:#e8edf2}" in assets
    assert ".themeButton .icoSun{display:none}" in assets
    assert 'html[data-theme="light"] .themeButton .icoSun{display:block}' in assets
    assert 'html[data-theme="light"] .themeButton .icoMoon{display:none}' in assets
    assert 'html[data-theme="light"] .themeButton{background:#f4f6f9;border-color:#ccd5df;box-shadow:inset 0 0 0 1px #d5dce4}' in assets
    assert 'html[data-theme="light"] .themeButton{color:#3f4f63}' in assets
    assert 'html[data-theme="light"] .themeButton:hover{color:#1a2330}' in assets
    # 图标换 lucide Moon/Sun（与 DD/D 启动页相同路径数据）
    assert '<g class="icoMoon"><path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/></g>' in assets
    assert '<circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/>' in assets

    # 三态按钮组及其代码彻底移除，无死代码残留
    assert "themeTabs" not in assets
    assert "themeSwitch" not in assets
    assert "renderThemeTabs" not in assets
    assert "theme.auto" not in assets
    assert "theme.light" not in assets
    assert "theme.dark" not in assets

    # i18n 文案：仅保留 aria 标题键，中英文各一条
    for text in ["'theme.title':'主题'", "'theme.title':'Theme'"]:
        assert text in assets

    # 前端逻辑：状态、渲染、初始化与切换，仅内存态、不写任何存储
    assert "const THEME_STORAGE_KEY='mus4.ui.theme'" not in assets
    assert "function readStoredTheme()" not in assets
    assert "function writeStoredTheme(theme)" not in assets
    assert "localStorage.getItem('mus4.ui.theme')" not in assets
    assert "let uiTheme='auto'" in assets
    # 单击切换：在当前生效主题（resolvedTheme）的深/浅反向间来回切换（仅内存、不持久化）
    assert "function toggleTheme(){setTheme(resolvedTheme()==='light'?'dark':'light')}" in assets
    assert "function setTheme(theme){uiTheme=theme;applyTheme()}" in assets
    assert "function initTheme(){uiTheme='auto';applyTheme();try{const mq=window.matchMedia('(prefers-color-scheme: light)');const onThemeChange=()=>{if(uiTheme==='auto')applyTheme()};if(mq.addEventListener)mq.addEventListener('change',onThemeChange);else if(mq.addListener)mq.addListener(onThemeChange)}catch(e){}}" in assets
    assert "initTheme();" in assets

    # 跟随系统：'auto' 经 matchMedia 解析（matchMedia 不可用/异常时回退 dark），显式 light/dark 原样返回
    assert "function systemTheme(){try{return window.matchMedia&&window.matchMedia('(prefers-color-scheme: light)').matches?'light':'dark'}catch(e){return 'dark'}}" in assets
    assert "function resolvedTheme(){return uiTheme==='auto'?systemTheme():(uiTheme==='light'?'light':'dark')}" in assets

    # 系统主题 change 监听：仅 'auto' 时实时重应用；新内核 addEventListener，老内核回退 addListener
    assert "const mq=window.matchMedia('(prefers-color-scheme: light)')" in assets
    assert "if(uiTheme==='auto')applyTheme()" in assets
    assert "mq.addEventListener('change',onThemeChange)" in assets
    assert "mq.addListener(onThemeChange)" in assets

    # 防闪烁：<head> 内第一个 <style> 之前的内联脚本，直接按 matchMedia 预置 data-theme（不读任何存储，刷新即重新跟随系统）
    assert "<script>try{let t=window.matchMedia('(prefers-color-scheme: light)').matches?'light':'dark';document.documentElement.dataset.theme=t}catch(e){}</script>" in assets
    assert assets.index("<title>Drifter Console</title>") < assets.index("window.matchMedia('(prefers-color-scheme: light)')")
    assert assets.index("window.matchMedia('(prefers-color-scheme: light)')") < assets.index("<style>")


def test_ota_glitch_led_effect():
    """OTA 固件传输期间状态灯随机乱闪（故障灯效）：灭/红/绿/蓝随机颜色 +
    30-120ms 随机间隔。HTTP /update 与 ArduinoOTA 两条通道都挂载——上传期间
    主循环（及 scanLEDToggle）阻塞在传输 handler 里，灯效必须由上传回调直接
    驱动；传输结束（成功/失败/中止）清 toggleActive，ControlMixer 下一循环
    自动恢复正常状态指示。"""
    led_h = (PROJECT_ROOT / "libraries" / "mus4_ui" / "src" / "LedStatus.h").read_text(encoding="utf-8")
    led = (PROJECT_ROOT / "libraries" / "mus4_ui" / "src" / "LedStatus.cpp").read_text(encoding="utf-8")
    server = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp").read_text(encoding="utf-8")
    sketch = (PROJECT_ROOT / "MUS4_FW.ino").read_text(encoding="utf-8")

    # API 声明与实现
    for decl in ["void startLedOtaGlitch();", "void scanLedOtaGlitch();", "void stopLedOtaGlitch();"]:
        assert decl in led_h
    # 随机颜色：灭/红/绿/蓝四态；随机间隔 30-120ms
    assert "{CRGB::Black, CRGB::Red, CRGB::Green, CRGB::Blue}" in led
    assert "random(0, 4)" in led
    assert "random(30, 121)" in led
    # 结束时归还状态灯：清 toggle 状态，ControlMixer 下一 loop 重应用
    stop_fn = led[led.index("void stopLedOtaGlitch()"):]
    assert "toggleActive = false;" in stop_fn

    # HTTP /update 通道：开始传输启动灯效、每写一块推进一步、END/ABORTED 停止
    assert '#include "LedStatus.h"' in server
    upload_fn = server[server.index("static void handleWifiWebUpdateUpload()"):]
    assert upload_fn.index("startLedOtaGlitch();") < upload_fn.index("UPLOAD_FILE_WRITE")
    assert upload_fn.index("scanLedOtaGlitch();") > upload_fn.index("UPLOAD_FILE_WRITE")
    assert upload_fn.count("stopLedOtaGlitch();") == 2  # UPLOAD_FILE_END 与 UPLOAD_FILE_ABORTED

    # ArduinoOTA 通道：onStart/onProgress 驱动，onEnd/onError 停止
    assert sketch.index("ArduinoOTA.onStart") < sketch.index("startLedOtaGlitch();") < sketch.index("ArduinoOTA.onEnd")
    onprogress_fn = sketch[sketch.index("ArduinoOTA.onProgress"):]
    assert "scanLedOtaGlitch();" in onprogress_fn

    # 成功后跨重启延续：HTTP restart 前与 ArduinoOTA onEnd 都写 RTC 标记
    assert "RTC_DATA_ATTR" in led
    assert "void markLedOtaGlitchAfterReboot()" in led
    assert "bool takeLedOtaGlitchAfterReboot()" in led
    post_fn = server[server.index("static void handleWifiWebUpdatePost()"):]
    assert post_fn.index("markLedOtaGlitchAfterReboot();") < post_fn.index("ESP.restart();")
    onend_fn = sketch[sketch.index("ArduinoOTA.onEnd"):sketch.index("ArduinoOTA.onProgress")]
    assert "markLedOtaGlitchAfterReboot();" in onend_fn
    assert sketch.count("stopLedOtaGlitch();") == 3  # ArduinoOTA onEnd/onError + loop() 蜂鸣器播完判停

    # 重启后：setup() 自检后取标记、以"等蜂鸣器"模式重启乱闪；loop() 播完判停（800ms 宽限）
    assert "void startLedOtaGlitchUntilBuzzerIdle()" in led
    assert "bool isLedOtaGlitchActive()" in led
    assert "bool ledOtaGlitchWaitsForBuzzer()" in led
    assert sketch.index("takeLedOtaGlitchAfterReboot()") > sketch.index("runLedPowerOnSelfTest();")
    assert "startLedOtaGlitchUntilBuzzerIdle();" in sketch
    loop_fn = sketch[sketch.index("void loop()"):]
    assert "buzzer.isPlaying()" in loop_fn
    assert ">= 800" in loop_fn
    # 乱闪期间正常状态机静默：setLEDColor/setLEDToggle×2/scanLEDToggle 四处 early-return 屏蔽
    assert led.count("if (isLedOtaGlitchActive())") == 4
    assert "stopLedOtaGlitch();" in sketch[sketch.index("ArduinoOTA.onError"):]
    # 上电自检 3 秒期间保持驱动蜂鸣器，避免开机旋律第一个音符被阻塞拖长
    assert "buzzer.update();" in led
    assert "delaySelfTestHold(3000)" in led
    # v1.7.53：自检改为 RGB 三通道齐亮（白色）常亮 3 秒，不再红绿蓝轮流各 1 秒
    assert "setLEDColor(CRGB::White);" in led
    assert "CRGB::Red);\n    delaySelfTestHold" not in led


def test_web_console_header_entry_buttons():
    """v1.7.54 新增"进入 Donkey" / "进入 DonkeyDrifter"两个入口按钮；
    v1.7.56 为两个按钮接上 onclick 跳转；
    v1.7.57 改为动态获取 host_ip（从 /api/status 解析），不再硬编码 IP；
    v1.7.59 "进入 Donkey"按钮 D 大写；enterDonkeyDrifter 改为 GET /launch/drive 新标签页打开。
    v1.7.60 修复 Safari 弹窗拦截：页面加载时预取 host_ip 缓存到 _launcherIp，
    按钮点击时同步调用 window.open，不再在 async 函数内 await 后开新标签。
    v1.7.61 改用 <a target="_blank"> 原生链接替代 window.open，彻底解决 Safari 弹窗拦截。
    v1.7.70 "进入"改名"打开"（en Enter→Open）；DonkeyDrifter 右侧新增"打开 Kimi Code Web"
    占位按钮（功能预留，无 href/onclick）。
    v1.7.74 "打开 Kimi Code Web"接功能：沿用 _launcherIp，POST :8090/api/launch/kimi-code-web，
    AbortController 120s 超时，同步上下文先开 about:blank 句柄，成功后导航、失败关闭。
    v1.7.76 三个入口按键高度由 24px 提至 34px（与 DD 侧"打开"按键对齐），
    通过专属规则 #enterDonkeyBtn,#enterDonkeyDrifterBtn,#openKimiCodeWebBtn{height:34px}
    覆盖，.otaButton 基础规则保持 24px。
    v1.7.79 该规则扩展选择器追加 .headerRow .otaLink .otaButton（头部 OTA 按钮同高 34px），
    并新增 #devModeToggle scoped 规则把 DEV 开关轨道加高至 34px。
    v1.7.80 OTA 按钮与 DEV 开关按原比例加宽（OTA 字号 16px/内边距 14px；开关 62×34px、位移 28px），
    开关旁 "DEV ON/OFF" 文字删除，"DEV" 写到滑珠上。
    v1.8.24 OTA 改为 .otaLink 文字胶囊、DEV 改为 #devModeToggle 文字胶囊（DD 同款），
    滑珠 / .otaButton / devModeCheck 全部移除。
    v1.7.90 三个入口按键显示文案去掉"打开 "/"Open "前缀（zh/en 同步），
    按钮 id、href/onclick 跳转与其它词条均不变。
    v1.8.7 Kimi Code Web 右侧新增"打开 DeepSeek Harness"按钮（#openDshBtn），
    沿用 _launcherIp，POST :8090/api/launch/dsh，交互与 Kimi Code Web 按钮同款。
    v1.8.7 Kimi Code Web 右侧新增"打开 DeepSeek Harness"按钮（#openDshBtn），
    沿用 _launcherIp，POST :8090/api/launch/dsh，交互与 Kimi Code Web 按钮同款。
    v1.8.42 Kimi Code Web 与 DeepSeek Harness 之间新增"ZCode"按钮（#openZCodeBtn），
    沿用 _launcherIp，POST :8090/api/launch/zcode，返回 launcher 网页终端 URL
    并在新标签页打开（终端内运行 ZCode TUI agent），交互与 DeepSeek Harness 按钮同款、超时 15s。"""
    assets = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(
        encoding="utf-8"
    )

    # 位置：主标题 <h1> 之后、GitHub 链接之前，Donkey 在左、DonkeyDrifter 居中、
    # Kimi Code Web 在右、ZCode 紧随其后、DeepSeek Harness 在最右
    h1_pos = assets.index('<h1><a class="titleLink" href="https://www.donkeydrift.com" target="_blank" rel="noopener" data-i18n="app.title">Drifter Console</a></h1>')
    donkey_pos = assets.index('data-i18n="button.enterDonkey"')
    drifter_pos = assets.index('data-i18n="button.enterDonkeyDrifter"')
    kimi_pos = assets.index('data-i18n="button.openKimiCodeWeb"')
    zcode_pos = assets.index('data-i18n="button.openZCode"')
    dsh_pos = assets.index('data-i18n="button.openDsh"')
    gh_pos = assets.index('<a class="ghLink"')
    assert h1_pos < donkey_pos < drifter_pos < kimi_pos < zcode_pos < dsh_pos < gh_pos

    # v1.7.61：改用 <a target="_blank"> 原生链接，不再使用 onclick + window.open
    assert 'id="enterDonkeyBtn"' in assets
    assert 'id="enterDonkeyDrifterBtn"' in assets
    assert 'id="openKimiCodeWebBtn"' in assets
    assert 'id="openZCodeBtn"' in assets
    assert 'id="openDshBtn"' in assets
    assert 'target="_blank"' in assets
    assert 'rel="noopener"' in assets

    # v1.7.60：页面加载时预取 host_ip 缓存到 _launcherIp，动态设置 <a> href
    assert '_launcherIp' in assets
    assert '_fetchLauncherIp' in assets
    assert '/api/status' in assets
    assert 'host_ip=' in assets
    assert '192.168.3.41' in assets  # fallback IP
    assert ':8090/' in assets  # launcher port

    # i18n 中英词条齐全（v1.7.70 "进入"→"打开" / Enter→Open）
    # v1.7.90：入口按键文案去掉"打开 "/"Open "前缀，zh/en 词条值相同，各出现 2 次
    assert assets.count("'button.enterDonkey':'Donkey'") == 2
    assert assets.count("'button.enterDonkeyDrifter':'DonkeyDrifter'") == 2
    assert assets.count("'button.openKimiCodeWeb':'Kimi Code Web'") == 2

    # v1.7.74："打开 Kimi Code Web"接功能：沿用 _launcherIp（/api/status 的 host_ip），
    # 点击 POST http://<host>:8090/api/launch/kimi-code-web（空体 POST，免 CORS 预检），
    # AbortController 120s 超时；点击同步上下文先开 about:blank 句柄，成功后导航、失败关闭；
    # 等待态禁用按钮并切换启动中文案，失败/超时走 toast 提示
    assert 'onclick="openKimiCodeWeb()"' in assets
    assert 'async function openKimiCodeWeb()' in assets
    assert "window.open('about:blank','_blank')" in assets
    assert ':8090/api/launch/kimi-code-web' in assets
    assert 'AbortController' in assets
    assert '120000' in assets
    assert "'button.openKimiCodeWebLaunching':'正在启动 Kimi Code Web...'" in assets
    assert "'button.openKimiCodeWebLaunching':'Launching Kimi Code Web...'" in assets
    assert "'toast.kimiCodeWebFailed':'Kimi Code Web 启动失败'" in assets
    assert "'toast.kimiCodeWebFailed':'Failed to launch Kimi Code Web'" in assets
    assert "'toast.kimiCodeWebTimeout':'Kimi Code Web 启动超时'" in assets
    assert "'toast.kimiCodeWebTimeout':'Kimi Code Web launch timed out'" in assets

    # v1.8.7："打开 DeepSeek Harness"按钮：交互与 Kimi Code Web 按钮同款，
    # POST http://<host>:8090/api/launch/dsh，about:blank 句柄 + 120s 超时，
    # 等待态禁用并切启动中文案，失败/超时走 toast；i18n 中英词条各出现 2 次
    assert 'onclick="openDsh()"' in assets
    assert 'async function openDsh()' in assets
    assert ':8090/api/launch/dsh' in assets
    assert "let dshLaunching=false" in assets
    assert assets.count("'button.openDsh':'DeepSeek Harness'") == 2
    assert "'button.openDshLaunching':'正在启动 DeepSeek Harness...'" in assets
    assert "'button.openDshLaunching':'Launching DeepSeek Harness...'" in assets
    assert "'toast.dshFailed':'DeepSeek Harness 启动失败'" in assets
    assert "'toast.dshFailed':'Failed to launch DeepSeek Harness'" in assets
    assert "'toast.dshTimeout':'DeepSeek Harness 启动超时'" in assets
    assert "'toast.dshTimeout':'DeepSeek Harness launch timed out'" in assets

    # v1.8.42："ZCode"按钮：交互与 DeepSeek Harness 按钮同款，
    # POST http://<host>:8090/api/launch/zcode（毫秒级返回，15s 超时），
    # about:blank 句柄 + 等待态禁用并切启动中文案，失败/超时走 toast；
    # i18n 中英词条各出现 2 次
    assert 'onclick="openZCode()"' in assets
    assert 'async function openZCode()' in assets
    assert ':8090/api/launch/zcode' in assets
    assert "let zCodeLaunching=false" in assets
    assert '15000' in assets
    assert assets.count("'button.openZCode':'ZCode'") == 2
    assert "'button.openZCodeLaunching':'正在启动 ZCode...'" in assets
    assert "'button.openZCodeLaunching':'Launching ZCode...'" in assets
    assert "'toast.zCodeFailed':'ZCode 启动失败'" in assets
    assert "'toast.zCodeFailed':'Failed to launch ZCode'" in assets
    assert "'toast.zCodeTimeout':'ZCode 启动超时'" in assets
    assert "'toast.zCodeTimeout':'ZCode launch timed out'" in assets

    # v1.7.59：enterDonkeyDrifter 指向 /launch/drive
    # v1.7.61：入口按钮改用 <a target="_blank">，不再使用 onclick + window.open
    # v1.7.62：enterDonkeyDrifter 改用 #drive hash（与"打开 Donkey"同路径，避免 Safari 问题）
    # v1.8.8：改回 /launch/drive 直达启动中转页，不再渲染 Donkey 菜单页（Issue #103）；
    # Safari 历史问题经 LAUNCH_DRIVE_HTML 轮询重定向后已消除
    assert assets.count(':8090/launch/drive') == 2  # 静态初值 + _applyLauncherStatus 动态改写
    assert ':8090/#drive' not in assets
    assert 'enterDonkeyLauncher' not in assets
    assert 'enterDonkeyDrifter()' not in assets

    # v1.7.76：三个入口按键 34px 高（对齐 DD 侧"打开"按键），专属规则覆盖；
    # v1.8.7 规则再追加 #openDshBtn；v1.8.24 OTA 改为 .otaLink 文字胶囊（不再有 .otaButton）
    assert '.otaLink{display:inline-flex;align-items:center;justify-content:center;height:32px' in assets
    assert '.otaButton{' not in assets
    # v1.8.16：DC 顶栏标签复刻 DD 两类标签结构——D/DD 为 14px 功能标签(.navTab)，
    # KCW/ZCode/DSH 为 12px 弱化标签(.navTabWeak)并带 lucide 图标；仅 2 个 .navTab
    assert '.navTab{font-family:inherit;color:#8fa1b5;font-size:0.875rem;font-weight:500;text-decoration:none;background:transparent;border:none;padding:0;line-height:1.25rem;white-space:nowrap;display:inline-flex;align-items:center;cursor:pointer;margin-right:12px}' in assets
    assert '.navTab:hover{color:#8bdcff;background:transparent}' in assets
    assert '.navTabWeak{font-family:inherit;color:#6b7d90;font-size:0.75rem;font-weight:500;text-decoration:none;background:transparent;border:none;padding:0;line-height:1rem;white-space:nowrap;display:inline-flex;align-items:center;gap:4px;cursor:pointer;margin-right:12px}' in assets
    assert '.navTabWeak:hover{color:#b9c5d3;background:transparent}' in assets
    assert assets.count('class="navTab"') == 2
    assert assets.count('class="navTabWeak"') == 3
    assert '<a class="navTab" data-i18n="button.enterDonkey"' in assets
    assert '<a class="navTab" data-i18n="button.enterDonkeyDrifter"' in assets
    assert '<button type="button" class="navTabWeak" id="openKimiCodeWebBtn"' in assets
    assert '<button type="button" class="navTabWeak" id="openZCodeBtn"' in assets
    assert '<button type="button" class="navTabWeak" id="openDshBtn"' in assets
    assert '<span data-i18n="button.openKimiCodeWeb">Kimi Code Web</span>' in assets
    assert '<span data-i18n="button.openZCode">ZCode</span>' in assets
    assert '<span data-i18n="button.openDsh">DeepSeek Harness</span>' in assets
    # KCW/ZCode/DSH 带 lucide 图标（Sparkles / Code2(code-xml) / FlaskConical，14px，stroke=currentColor）
    assert 'M9.937 15.5A2 2 0 0 0 8.5 14.063' in assets
    assert '<path d="m18 16 4-4-4-4"></path>' in assets
    assert '<path d="m6 8-4 4 4 4"></path>' in assets
    assert '<path d="m14.5 4-5 16"></path>' in assets
    assert 'M14 2v6a2 2 0 0 0 .245.96' in assets
    # v1.8.16 追加：顶栏字体渲染对齐 DD 导航——font-synthesis:none 阻止 500 字重被合成加粗，
    # text-rendering + font-smoothing 让字形更细更清晰（否则 Donkey/DonkeyDrifter 显得更粗更大）
    # v1.8.20：主 DC 页标题行默认显示，仅在 DD 嵌入（?embedded=1）时经 body.embedded 隐藏（Issue #234）
    assert '.headerRow{display:flex;align-items:center;gap:12px;flex-wrap:wrap;margin:0 0 10px;font-family:system-ui,-apple-system,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif;font-synthesis:none;text-rendering:optimizeLegibility;-webkit-font-smoothing:antialiased;-moz-osx-font-smoothing:grayscale}' in assets
    assert 'body.embedded .headerRow{display:none}' in assets

def test_web_console_light_theme_overrides():
    """浅色主题生效：setTheme/initTheme 通过 applyTheme 把解析结果写到
    document.documentElement.dataset.theme（'auto' 经 matchMedia 跟随系统，
    系统为浅色时解析为 light，否则为 dark），并使网格缓存失效重绘。
    浅色样式全部以新增覆盖规则挂在 html[data-theme="light"] 选择器下
    （第三个 <style> 块），深色原文逐字不动。
    v1.8.3 起 setTheme/initTheme 不再渲染三态按钮组（#themeTabs 已移除，
    见 test_web_console_theme_toggle），仅保留主题解析与应用。
    canvas 图表与 toast 的 JS 颜色改从 CHART_THEMES 双主题色表取。"""
    assets = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(encoding="utf-8")

    # 浅色覆盖块：基底 / 日志终端 / 状态卡片 / 画布
    assert 'html[data-theme="light"] body{background:#eef1f5;color:#1a2330}' in assets
    assert 'html[data-theme="light"] .log{background:#f4f7f5;color:#1a7f37}' in assets
    assert 'html[data-theme="light"] .stateCard{border-color:#ccd5df;background:linear-gradient(135deg,#fff,#edf1f6);box-shadow:0 1px 3px rgba(15,23,42,.08)}' in assets
    assert 'html[data-theme="light"] canvas{background:#fbfcfe;border-color:#d5dce4}' in assets
    # 浅色下脉冲动画换成柔和版（结构与深色 pulse 一致，仅换颜色）
    assert '@keyframes pulseLight{50%{box-shadow:0 0 18px rgba(229,72,77,.3);transform:translateY(-1px)}}' in assets
    assert 'html[data-theme="light"] .parkLocked{animation:pulseLight 1.2s infinite}' in assets
    # 浅色特异性修正：fabToggle 保持青色发光圆点身份（hover/focus/active 加深为 #3aa8dd）
    assert 'html[data-theme="light"] .fabToggle{background:#5cc8ff;border-color:#5cc8ff}' in assets
    assert 'html[data-theme="light"] .fabToggle:hover,html[data-theme="light"] .fabToggle:focus-visible,html[data-theme="light"] .fabToggle:active{background:#3aa8dd;border-color:#3aa8dd}' in assets
    assert 'html[data-theme="light"] .muteButton{background:#f4f6f9;border-color:#ccd5df;box-shadow:inset 0 0 0 1px #d5dce4;color:#3f4f63}' in assets
    assert 'html[data-theme="light"] .muteButton:hover{color:#1a2330}' in assets
    assert 'html[data-theme="light"] .muteButton.muted{background:rgba(92,200,255,.1);border-color:#5cc8ff;box-shadow:inset 0 0 0 1px #5cc8ff;color:#5cc8ff}' in assets
    assert 'html[data-theme="light"] .rcNum{background:transparent}' in assets
    # 浅色特异性修正：胶囊按钮组（语言/主题/LED）未激活段恢复透明，缝隙只露出容器底色，与深色行为一致
    assert 'html[data-theme="light"] .langTabs button{background:transparent;color:#5b6b7d}' in assets
    # v1.8.24：OTA/DEV 改为文字胶囊，浅色 .otaLink / #devModeToggle 规则取代原 .otaButton
    assert 'html[data-theme="light"] .otaLink{background:#f4f6f9;border-color:#ccd5df' in assets
    assert 'html[data-theme="light"] #devModeToggle{background:#f4f6f9;border-color:#ccd5df' in assets
    assert 'html[data-theme="light"] #devModeToggle.devOn{background:rgba(92,200,255,.25)' in assets
    assert 'html[data-theme="light"] .otaButton' not in assets
    # v1.8.14：入口标签 .navTab 浅色复刻 DD 主导航标签（弱化色，hover 主题色，透明无框）
    assert 'html[data-theme="light"] .navTab{color:#5b6b7d;background:transparent;border:none}' in assets
    assert 'html[data-theme="light"] .navTab:hover{color:#0a7eb2;background:transparent}' in assets
    assert 'html[data-theme="light"] .navTabWeak{color:#7c8da0;background:transparent;border:none}' in assets
    assert 'html[data-theme="light"] .navTabWeak:hover{color:#3f4f63;background:transparent}' in assets
    # 浅色下胶囊容器加深底色并强化描边，使激活胶囊与外容器的嵌套轮廓与深色一样清晰
    assert 'html[data-theme="light"] .langTabs{background:#dde3ec;border-color:#ccd5df;box-shadow:inset 0 0 0 1px #d5dce4}' in assets

    # JS：主题解析与应用（auto 经 matchMedia 跟随系统），切换时网格缓存失效并重绘
    assert "function systemTheme(){try{return window.matchMedia&&window.matchMedia('(prefers-color-scheme: light)').matches?'light':'dark'}catch(e){return 'dark'}}" in assets
    assert "function resolvedTheme(){return uiTheme==='auto'?systemTheme():(uiTheme==='light'?'light':'dark')}" in assets
    assert "function applyTheme(){document.documentElement.dataset.theme=resolvedTheme();gridReady=false;draw();const dl=document.getElementById('driftTuneLink');if(dl)dl.href='/drift?theme='+resolvedTheme()}" in assets
    assert "function setTheme(theme){uiTheme=theme;applyTheme()}" in assets
    assert "function initTheme(){uiTheme='auto';applyTheme();try{const mq=window.matchMedia('(prefers-color-scheme: light)');const onThemeChange=()=>{if(uiTheme==='auto')applyTheme()};if(mq.addEventListener)mq.addEventListener('change',onThemeChange);else if(mq.addListener)mq.addListener(onThemeChange)}catch(e){}}" in assets

    # 图表/toast 双主题色表：深浅的 grid 与 str 关键色
    assert "grid:'#233041'" in assets
    assert "grid:'#dbe2ea'" in assets
    assert "str:'#5cc8ff'" in assets
    assert "str:'#0c9bd6'" in assets
    # JS 取色走色表而非硬编码
    assert "gridCtx.strokeStyle=CHART_THEMES[resolvedTheme()].grid" in assets
    assert "drawSeries('thr',ct.thr,-1,1,100)" in assets
    assert "toast.style.borderColor=ok?CHART_THEMES[resolvedTheme()].toastOk:CHART_THEMES[resolvedTheme()].toastErr" in assets

    # 原有主题骨架不回退（仅内存态，不再有 localStorage 读写）
    assert "const THEME_STORAGE_KEY='mus4.ui.theme'" not in assets
    assert "let uiTheme='auto'" in assets
    assert "localStorage.getItem(THEME_STORAGE_KEY)" not in assets
    assert "initTheme();" in assets
    # 深色原文不动：激活胶囊两主题保持 #5cc8ff/#061019
    assert '.langTabs button.active{background:#5cc8ff;color:#061019}' in assets


def test_wifi_sta_history_retry_rescans_after_exhaustion():
    """Issue #88：一轮历史候选试完后不再终局——冷却 WIFI_STA_HISTORY_RESCAN_INTERVAL_MS
    后清掩码重开新一轮，覆盖「小车先开机、历史 Wi-Fi 后出现（或暂时不在覆盖范围）」
    的场景，否则该 Wi-Fi 之后出现时小车永远不会再尝试连接。"""

    manager_source = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.cpp").read_text(encoding="utf-8")
    runtime_state = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "RuntimeState.h").read_text(encoding="utf-8")

    # 冷却常量紧邻既有重试节流常量；运行态字段承载重扫描截止时刻
    assert "static const unsigned long WIFI_STA_HISTORY_RESCAN_INTERVAL_MS = 15000;" in manager_source
    assert "unsigned long staHistRescanDeadlineMs = 0;" in runtime_state

    retry_body = re.search(
        r"(?:static )?void updateWifiStaHistoryRetry\(\)\s*\{(?P<body>.*?)\n\}",
        manager_source,
        re.DOTALL,
    ).group("body")

    # 候选耗尽：记录冷却截止并打 rescan 日志，不再直接终局
    assert "wifiRuntime.staHistRescanDeadlineMs = millis() + WIFI_STA_HISTORY_RESCAN_INTERVAL_MS;" in retry_body
    assert "STA history retry: candidates exhausted, rescan in %lus" in retry_body
    # 冷却期满：未到冷却期直接返回（统一 (long)(millis() - deadline) < 0 回绕比较）
    assert "(long)(millis() - wifiRuntime.staHistRescanDeadlineMs) < 0" in retry_body
    # 冷却期满清掩码重开新一轮，connected 上升沿同步清零冷却截止
    assert "wifiRuntime.staHistTriedMask = 0;" in retry_body
    assert "STA history retry: starting new round" in retry_body
    assert "wifiRuntime.staHistRescanDeadlineMs = 0;" in retry_body


def test_wifi_sta_history_retry_window_accepts_unconfigured_sta():
    """Issue #88：STA 从未配置（NVS sta_en=false 或从未配网）但历史记录非空时
    也要进入重试窗口——否则只能靠开机那一刻的扫描，运行中永不重试。"""

    manager_source = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.cpp").read_text(encoding="utf-8")

    retry_body = re.search(
        r"(?:static )?void updateWifiStaHistoryRetry\(\)\s*\{(?P<body>.*?)\n\}",
        manager_source,
        re.DOTALL,
    ).group("body")

    window = re.search(r"bool inRetryWindow.*?\n.*?;", retry_body, re.DOTALL).group(0)
    assert "wifiStaHistoryCount() > 0" in window
    # 历史为空时仍由函数体内既有兜底分支拦截，不会空转扫描
    assert "if (wifiStaHistoryCount() == 0)" in retry_body
