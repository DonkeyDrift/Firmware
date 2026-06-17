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
    PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "SteeringCalibration.h",
    PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "SteeringCalibration.cpp",
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


def test_firmware_version_is_v1_7_6_and_changelog_is_current():
    build_info = BUILD_INFO.read_text(encoding="utf-8")
    changelog = CHANGELOG.read_text(encoding="utf-8")

    assert '#define MUS4_FIRMWARE_VERSION "v1.7.6"' in build_info
    assert "## 2026-06-12 v1.7.6" in changelog
    assert changelog.index("## 2026-06-12 v1.7.6") < changelog.index("## 2026-06-11 v1.7.4")


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
    assert '<option value="serial1">Serial1</option>' in source
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
    assert "Serial Log：查看设备日志和命令反馈" in source
    assert "Tub JSON：记录并下载遥测样本" in source
    assert "OTA / DEV：固件更新与开发模式开关" in source
    assert "Status Cards: view mode, Park, OTA, and connection status" in source


def test_web_console_has_collapsed_glow_fab_with_radial_actions():
    source = firmware_source_text()

    assert 'id="fabToggle"' in source
    assert 'class="fabToggle"' in source
    assert 'id="fabActions"' in source
    assert 'class="fabActions"' in source
    assert 'id="langFab"' in source
    assert 'class="langFab"' in source
    assert 'id="langMenu"' in source
    assert 'class="langMenu"' in source
    assert 'id="helpFab"' in source
    assert "🌐" in source
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
    assert ".fabActions.show .langFab" in source
    assert ".fabActions.show .helpFab" in source
    assert source.index('id="fabToggle"') < source.index('id="fabActions"') < source.index('id="langFab"') < source.index('id="helpFab"')


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

    assert "bool dispatchCommandLine(const String& line, Print& out, SerialBuf& sb)" in dispatcher_header
    assert "bool dispatchCommandLine(const String& line, Print& out, SerialBuf& sb)" in dispatcher_source
    assert "extern ControlData pilot_data;" in dispatcher_source
    assert "struct struct_message" not in dispatcher_source
    assert "ControlData esp_now_data" in sketch_source
    assert "ControlData rc_data" in sketch_source
    assert "ControlData pilot_data" in sketch_source
    assert "ControlData car_output" in sketch_source
    assert "struct struct_message" not in sketch_source
    assert "dispatchCommandLine(line, out, sb);" in source
    assert "dispatchCommandLine(String(sb.buf), ser, sb);" not in source
    assert "#define PROCESS_COMMAND_LINE" not in sketch_source
    assert "PROCESS_COMMAND_LINE" not in sketch_source

    for symbol in [
        "ACK:LOG_WEB",
        "ACK:LOG_SERIAL",
        "ACK:CAL_SAVED",
        "NACK:CAL_SAVE_FAILED",
        "NACK:CAL_INVALID_RANGE",
        "NACK:CAL_NOT_DONE",
        "ACK:CAL_RETRY",
        "ACK:CAL_ABORTED",
        "ACK:CAL_RESET",
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
    assert "dispatchCommandLine(line, out, sb);" in reader_source
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

    # Serial1 telemetry must always be logged, even when the OTA window keeps
    # the hardware line silent.
    assert "appendWebLog(\"serial1\", telem)" in sketch_source
    assert "if (shouldEmitSerial1Telemetry(otaRuntime)) {\n            Serial1.println(telem);" in sketch_source

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


def test_wifi_console_types_are_split_from_sketch():
    source = firmware_source_text()
    wifi_types = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h").read_text(encoding="utf-8")
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")

    for symbol in [
        "const char* WIFI_CONSOLE_AP_DEFAULT_SSID = \"MUS4-ESP\";",
        "const char* WIFI_CONSOLE_AP_PASSWORD = \"mus4-debug\";",
        "const uint16_t WIFI_CONSOLE_PORT = 2323;",
        "const uint16_t WIFI_WEB_CONSOLE_PORT = 80;",
        "const uint32_t WIFI_WEB_TELEMETRY_MIN_FREE_HEAP = 60000;",
        "const uint8_t WIFI_CONSOLE_CHANNEL = 6;",
        "const uint8_t WIFI_CONSOLE_MAX_CLIENTS = 1;",
        "const unsigned long WIFI_CONSOLE_RETRY_INTERVAL_MS = 5000;",
        "const unsigned long WIFI_STA_CONNECT_TIMEOUT_MS = 15000;",
        "const unsigned long WIFI_STA_APPLY_DELAY_MS = 800;",
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
    ]:
        assert symbol in wifi_types

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
        "car_output.mode != CAR_MODE_MANUAL || !drift_assist_enabled",
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



def test_steering_calibration_remains_available_after_module_split():
    source = firmware_source_text()

    assert "struct SteeringCalibration" in source
    assert "enum SteerCalState" in source
    assert "void loadSteeringCalibration()" in source
    assert "bool saveSteeringCalibration()" in source
    assert "void resetSteeringCalibration()" in source
    assert "int mapSteeringCalibrated(int16_t pwm)" in source
    assert "bool startSteerCalibration(Print& out)" in source
    assert "void updateSteerCalibration()" in source
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
        "void scanLEDToggle()",
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
    assert "extern const int SERVO_MID_V = 7372" in actuator_source
    assert "extern const int SERVO_RANGE_V = 2458" in actuator_source


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
    assert "return !os.windowOpen && !os.inProgress" in ota_source
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
    assert "if (!webDevMode && !ws.consoleAuthenticated)" in ota_source
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
    assert "return !os.windowOpen && !os.inProgress" in source
    assert "forceWifiOtaParkLocked" in source
    assert "AUTH:<redacted>" in source
    assert "WIFI_STA_PASSWORD:<redacted>" in source


def test_web_console_uses_dev_label_for_development_switch():
    source = firmware_source_text()

    assert "DEV <b id=\"devModeSwitchText\">OFF</b>" in source
    assert "DEV MODE <b id=\"devModeSwitchText\">OFF</b>" not in source
    assert "DEBUG MODE <b id=\"devModeSwitchText\">OFF</b>" not in source
    assert "Auto OTA <b id=\"devModeSwitchText\">OFF</b>" not in source


def test_web_console_header_and_state_cards_keep_compact_layout():
    source = firmware_source_text()

    assert ".headerRow{display:flex;align-items:flex-end;" in source
    assert ".toggleSwitch{position:relative;display:inline-flex;align-items:center;gap:8px;cursor:pointer}" in source
    assert ".otaLink{margin-left:auto;text-decoration:none}" in source
    assert ".otaButton{background:#5cc8ff;color:#061019;border-color:#5cc8ff;font-weight:800;font-size:11px;padding:0 10px;min-width:0;height:24px;border-radius:999px;line-height:1}" in source
    assert ".devHint{position:relative}" in source
    assert ".devHint:hover:after" in source
    assert "content:'开发模式会持久化；Web Console 免 AUTH，但仍保留 Park Locked 安全限制。OTA 传输期间会默认 Park Locked。'" in source
    assert ".version{color:#8fa1b5;font-size:12px;text-transform:uppercase;letter-spacing:.08em;display:inline-block;transform:translateY(-1px)}" in source
    assert ".stateGrid{display:grid;gap:10px;align-items:stretch;grid-template-columns:" in source
    assert "#modeCard{grid-area:mode}" in source
    assert "#parkCard{grid-area:park}" in source
    assert "#driftCard{grid-area:drift}" in source
    assert "#voltageCard{grid-area:voltage}" in source
    assert "#networkCard{grid-area:network}" in source
    assert 'grid-template-areas:"mode park drift voltage network"' in source
    assert 'grid-template-areas:"mode park drift" "voltage network network"' in source
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
    assert '<b data-i18n="state.remain">REMAIN</b><span id="voltageSub">battery</span>' in source
    assert 'onclick="event.stopPropagation();openNetworkSettings()"' in source
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
    assert "voltageValue.textContent=t('voltage.disconnected')" in source
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


def test_wifi_ap_ssid_prefix_is_limited_to_six_chars_with_dev_mode_suffix():
    wifi_types = (PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h").read_text(encoding="utf-8")
    manager_header = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.h").read_text(encoding="utf-8")
    manager_source = (PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.cpp").read_text(encoding="utf-8")
    assets_source = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(encoding="utf-8")
    source = firmware_source_text()

    assert 'const uint8_t WIFI_AP_SSID_PREFIX_MAX_LEN = 6;' in wifi_types
    assert 'const char* WIFI_AP_SSID_SUFFIX = "-ESP";' in wifi_types
    assert "String getActiveWifiApSsid()" in manager_header
    assert "String getActiveWifiApSsid()" in manager_source
    assert "buildWifiDevApSsid" in manager_source
    assert "wifiStaSsidShortUpper" in manager_source
    assert "wifiStaIpTailText" in manager_source
    assert "!wifiDevModeEnabled || !wifiStaConnected" in manager_source
    assert 'maxlength="6"' in assets_source
    assert '/^([A-Za-z0-9]{1,6})$/' in assets_source
    assert "长度为 1-6 位" in assets_source
    assert "前 3 位大写" in source
    assert "last two IP octets" in source


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
    source = firmware_source_text()

    assert "WIFI_STA_HANDOFF_AP_KEEP_MS" not in source
    assert "WIFI_AP_STOP_AFTER_STA_CONNECTED_DELAY_MS" not in source
    assert "bool& wifiStaHandoffActive" in source
    assert "char* const wifiStaHandoffTargetSsid" in source
    assert "void startWifiStaHandoff" in source
    assert "void finishWifiStaHandoff" in source
    assert "void clearWifiStaHandoff" in source
    handoff_body = re.search(
        r"void startWifiStaHandoff\(const String& targetSsid\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "ensureWifiApAvailable()" in handoff_body
    assert "restartWifiAp()" not in handoff_body
    assert "body.set('source',location.hostname==='192.168.4.1'?'ap':'sta')" in source
    assert "wifiWebServer.arg(\"source\")" in source
    assert "startWifiStaHandoff(ssid)" in source


def test_wifi_sta_handoff_status_api_and_web_prompt_are_present():
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
    assert "请将电脑/手机切换到 Wi-Fi" in source
    assert "然后打开" in source
    assert "http://192.168.4.1/" in source
    assert "连接设备 AP" in source
    assert "打开新地址" in source
    assert "复制 IP" in source


def test_web_console_header_ota_button_and_log_area_are_compact():
    source = firmware_source_text()
    assets_source = (PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleAssets.h").read_text(encoding="utf-8")
    sketch_source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "static const char WIFI_WEB_UPDATE_HTML[] PROGMEM" in assets_source
    assert "MUS4 HTTP OTA" in assets_source
    assert "static const char WIFI_WEB_UPDATE_HTML[] PROGMEM" not in sketch_source
    assert source.index('<section class="panel" id="chartPanel">') < source.index('<section class="panel" id="serialPanel">')
    assert '<section class="panel" id="serialPanel">' in source
    assert "#serialPanel{display:flex;flex-direction:column;gap:8px;padding-bottom:6px}" in source
    assert "#serialPanel .log{flex:1 1 auto;min-height:calc(5 * 1.35em + 16px);max-height:calc(20 * 1.35em + 16px)}" in source
    assert "@media(min-width:900px){.grid{grid-template-columns:2fr 1fr}.wide{grid-column:1/-1}#diagnosticsPanel{grid-column:1/-1}}" in source
    assert "canvas{width:100%;height:auto;aspect-ratio:38/13;" in source
    assert "#chartPanel:fullscreen canvas{width:min(100%,calc((100vh - 118px) * 38 / 13));height:auto;max-height:calc(100vh - 118px);aspect-ratio:38/13}" in source
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
    assert '<span class="recMeta">录制量<b id="tubMeta">0</b></span>' in source
    assert 'function clearChart(){pointHead=0;pointCount=0;points.fill(null);scrollOffset=0;smoothedDt=16;gridReady=false;tubSamples=[];tubStartedMs=0;tubStoppedMs=0;tubLastSeq=0;tubRecording=false;updateTubMeta();draw()}' in source
    assert "c.innerHTML=chartPaused?ICON_PLAY:ICON_PAUSE" in source
    assert "f.innerHTML=document.fullscreenElement===chartPanel?ICON_FULLSCREEN_EXIT:ICON_FULLSCREEN" in source
    assert '<button onclick="clearChart()">清空曲线</button>' not in source
    assert '<button onclick="toggleChart()" id="chartBtn">暂停曲线</button>' not in source
    assert "'暂停曲线'" not in source
    assert "'继续曲线'" not in source
    assert "'退出全屏'" not in source
    assert "'全屏曲线'" not in source
    assert '<a href="/update" target="_blank" class="otaLink"><button class="otaButton" data-i18n="button.ota">OTA</button></a><label class="toggleSwitch"' in source
    assert '<button class="iconButton" onclick="togglePause()" id="pauseBtn" title="暂停"><svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="4" width="4" height="16" rx="1"/><rect x="14" y="4" width="4" height="16" rx="1"/></svg></button><button class="iconButton" onclick="clearLog()" title="清空"><svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/></svg></button><button class="iconButton" onclick="sendCmd()" id="sendBtn" title="发送"><svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M2 21l21-9L2 3v7l15 2-15 2v7z"/></svg></button><input id="cmd"><select id="cmdTarget">' in source
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
    assert "versionLabel.textContent=s.version.replace(/^V/,'v')" in source
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
    assert '.fold:not(.open) .foldBody{display:none}' in source
    assert diagnostics_panel in source
    assert source.index(state_panel) < source.index(chart_panel)
    assert source.index(chart_panel) < source.index(serial_panel)
    assert source.index(serial_panel) < source.index(diagnostics_panel)
    assert source.index(diagnostics_panel) < source.index('id="rcFold" class="fold"')
    assert source.index('id="rcFold" class="fold"') < source.index('id="statusFold" class="fold"')
    assert '@media(min-width:900px){.grid{grid-template-columns:2fr 1fr}.wide{grid-column:1/-1}#diagnosticsPanel{grid-column:1/-1}}' in source
    assert "function toggleFold(id)" in source
    assert "function renderStatus(t)" in source
    assert "function parseStatusPairs(t)" in source
    assert "statusBox.textContent=t;updateNetworkCard" not in source


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

    assert "mus4.web_data_point.tub.v1" in source
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


def test_web_console_sta_settings_support_scan_and_password_visibility():
    source = firmware_source_text()

    assert 'id="staNotice"' in source
    assert "注意只能连接2.4G WiFi" in source
    assert "staNotice.textContent='正在连接'" in source
    assert "staNotice.textContent='STA 已连接，IP：'+j.sta_ip+'，AP 保持开启，可继续通过 AP 配置'" in source
    assert "staNotice.textContent='连接失败'" in source
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
        r"function selectWifiSsid\(ssid\)\{(?P<body>.*?)\}\n",
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
    assert "if (!ws.consoleAuthenticated && !ws.devModeEnabled)" in source
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


def test_web_console_keeps_ap_running_after_successful_wifi_sta_connection():
    source = firmware_source_text()

    assert "WIFI_AP_STOP_AFTER_STA_CONNECTED_DELAY_MS" not in source
    assert "wifiApStopPending" not in source
    assert "scheduleWifiApStopAfterStaConnected" not in source
    assert "stopWifiApAfterStaConnected" not in source
    assert "AP stopped after STA connected" not in source
    restart_body = re.search(
        r"(?:static )?bool restartWifiAp\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "wifiCaptiveDnsServer.stop()" in restart_body
    assert "WiFi.softAPdisconnect(true)" in restart_body
    assert "WiFi.mode(WIFI_STA)" not in source

    update_sta_body = re.search(
        r"(?:static )?void updateWifiSta\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    connected_branch = update_sta_body.split("if (status == WL_CONNECTED)", 1)[1].split("if (wifiStaConnected)", 1)[0]
    assert "finishWifiStaHandoff()" in connected_branch
    assert "scheduleWifiApStopAfterStaConnected" not in connected_branch
    assert "WiFi.softAP(" not in connected_branch
    assert "restartWifiAp()" not in connected_branch
    # Dev mode may schedule an AP restart to reflect STA info in the AP SSID.
    if "scheduleWifiApRestart()" in connected_branch:
        assert "wifiDevModeEnabled" in connected_branch
        assert "getActiveWifiApSsid()" in connected_branch


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
    assert "setTimeout(()=>{location.href=url},100)" in source
    assert "redirectToStaConsole(j.sta_ip)" in source
    assert "j.sta_ip&&j.sta_ip!=='0.0.0.0'" in source
    assert "const url='http://'+ip+'/'" in source
    assert "STA 已连接，IP：'+ip+'，正在跳转到 '+url" in source


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
    assert "setTimeout(resolve,1000)" in save_body
    assert "waitWifiStaConnectionResult()" in save_body
    assert save_body.index("setTimeout(resolve,1000)") < save_body.index("waitWifiStaConnectionResult()")
    assert "showCommandError(t)" not in save_body
    assert "await refreshStatus();cmd.value=''" in wait_body
    assert "STA 已连接，IP：'+j.sta_ip+'，AP 保持开启，可继续通过 AP 配置" in wait_body
    assert "AP 可能已关闭，STA 可能已连接" not in wait_body
    assert "showWifiStaFailureModal({ssid:staSsid.value.trim(),last_error_message:'AP 可能已关闭" not in wait_body
    assert "Date.now()+17000" not in wait_body
    assert "Date.now()+22000" in wait_body
    assert wait_body.index("staNotice.textContent='STA 已连接，IP：'+j.sta_ip+'，AP 保持开启，可继续通过 AP 配置'") < wait_body.index("await refreshStatus();cmd.value=''")


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
    assert "stopWifiMdnsIfNeeded()" in disconnected_branch
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
    assert setup_body.index("setupWifiWebConsole()") < setup_body.index("startWifiApServices(\"AP started\")")
    assert console_services_body.index("wifiCaptiveDnsServer.start(53, \"*\", WiFi.softAPIP())") < console_services_body.index("wifiConsoleServer.begin()")
    assert console_services_body.index("wifiConsoleServer.begin()") < console_services_body.index("wifiWebServer.begin()")
    assert console_services_body.index("wifiWebServer.begin()") < console_services_body.index("wifiConsoleStarted = true")
    assert setup_body.index("startWifiApServices(\"AP started\")") < setup_body.index("applyWifiStaCredentials()")


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
        r"(?:static )?bool startWifiApServices\(const char\* logPrefix\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "configureWifiSoftApNetwork()" in start_services_body
    assert start_services_body.index("configureWifiSoftApNetwork()") < start_services_body.index("WiFi.softAP(")
    assert "startWifiApServices(\"AP restarted\")" in restart_body
    assert "startWifiApServices(\"AP started\")" in setup_body


def test_sta_disconnect_keeps_soft_ap_clients_connected_and_services_available():
    source = firmware_source_text()

    ensure_body = re.search(
        r"(?:static )?bool ensureWifiApAvailable\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    update_sta_body = re.search(
        r"(?:static )?void updateWifiSta\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    disconnected_branch = update_sta_body.split("if (wifiStaConnected)", 1)[1].split("if (!wifiStaConnecting)", 1)[0]

    assert "ensureWifiApAvailable()" in disconnected_branch
    assert "restartWifiAp()" not in disconnected_branch
    assert "WiFi.softAPdisconnect(true)" not in disconnected_branch
    start_services_body = re.search(
        r"(?:static )?bool startWifiApServices\(const char\* logPrefix\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")

    assert "wifiApStopPending" not in source
    console_services_body = re.search(
        r"(?:static )?bool startWifiConsoleServices\(const char\* logPrefix\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")

    assert "startWifiApServices(\"AP ensured\")" in ensure_body
    assert "startWifiConsoleServices(\"AP ensured\")" in ensure_body
    assert "WiFi.softAPIP() == IPAddress(0, 0, 0, 0)" in ensure_body
    assert "WiFi.softAPdisconnect(true)" not in ensure_body
    assert "WiFi.mode(WIFI_OFF)" not in ensure_body
    assert "WiFi.softAP(" not in ensure_body
    assert "WiFi.softAP(" in start_services_body
    assert "wifiCaptiveDnsServer.start(53, \"*\", WiFi.softAPIP())" in console_services_body
    assert "wifiConsoleServer.begin()" in console_services_body
    assert "wifiConsoleServer.setNoDelay(true)" in console_services_body
    assert "wifiWebServer.begin()" in console_services_body
    assert "wifiConsoleStarted = true" in console_services_body


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
    assert "WiFi.disconnect(true, true)" in setup_body


def test_soft_ap_disconnect_is_limited_to_explicit_ap_restart():
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

    assert source.count("WiFi.softAPdisconnect(true)") == 1
    assert "WiFi.softAPdisconnect(true)" in restart_body
    assert "WiFi.softAPdisconnect(true)" not in setup_body
    assert "WiFi.softAPdisconnect(true)" not in apply_body
    assert "WiFi.softAPdisconnect(true)" not in update_sta_body
    assert "WiFi.mode(WIFI_OFF)" not in restart_body
    assert "WiFi.mode(WIFI_OFF)" not in apply_body
    assert "WiFi.mode(WIFI_OFF)" not in update_sta_body
    assert "WiFi.mode(WIFI_STA)" not in source


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
    assert "Microsoft Connect Test" not in source
    assert "Microsoft NCSI" not in source
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
    assert "location.replace" in redirect_body
    assert "http-equiv=\\\"refresh\\\"" in redirect_body
    assert "打开 Drifter Console" in redirect_body
    assert "打开 DonkeyDrift Console" not in redirect_body
    assert "WiFi.softAPIP().toString()" in redirect_body

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
    assert "#define ENABLE_WIFI_NETBIOS_DISCOVERY" in source
    assert "#define ENABLE_WIFI_LLMNR_DISCOVERY" in source
    assert "#ifdef ENABLE_WIFI_CONSOLE" in source
    assert source.index("#define ENABLE_WIFI_NETBIOS_DISCOVERY") > source.index("#ifdef ENABLE_WIFI_CONSOLE")


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
    assert "stopWifiNetbiosIfNeeded()" in disconnected_branch


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
    assert "stopWifiLlmnrIfNeeded()" in disconnected_branch
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
