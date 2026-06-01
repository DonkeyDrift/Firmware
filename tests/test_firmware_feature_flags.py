import pathlib
import re


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
MUS4_SKETCH = PROJECT_ROOT / "mus4.ino"


def test_websocket_curve_data_feature_is_enabled():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert re.search(r"^#define\s+ENABLE_WIFI_WEBSOCKET_TELEMETRY\b", source, re.MULTILINE)


def test_web_console_keeps_original_ui_and_direct_curve_path():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "MUS4 Web Console" in source
    assert "MUS4 Compact Console" not in source
    assert "pendingPoints.push" not in source
    assert "const interp={...prev}" not in source
    assert "chartLatencyMs=160" not in source
    assert "ws.send('ping')" not in source
    assert "\"pong\"" not in source


def test_diagnostic_code_is_not_built_by_default():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert re.search(r"^//\s*#define\s+ENABLE_DIAGNOSTIC_COMMANDS\b", source, re.MULTILINE)
    assert re.search(r"^//\s*#define\s+ENABLE_BOOT_STEERING_SELF_TEST\b", source, re.MULTILINE)


def test_web_console_uses_debug_mode_label_for_development_switch():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "DEBUG MODE <b id=\"devModeSwitchText\">OFF</b>" in source
    assert "Auto OTA <b id=\"devModeSwitchText\">OFF</b>" not in source


def test_web_console_explains_auth_and_park_rejections():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "function explainCommandError(t)" in source
    assert "请将 CH3/Park 切到锁定状态后重试" in source
    assert "请先 AUTH，或开启 DEBUG MODE 后重试" in source
    assert "alert(msg)" in source


def test_web_console_tub_recorder_is_browser_side_and_reuses_telemetry_points():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "mus4.web_data_point.tub.v1" in source
    assert "tubRecording" in source
    assert "tubSamples" in source
    assert "function ts()" in source
    assert "function te()" in source
    assert "function td()" in source
    assert "function tp(p)" in source
    assert "handleDataPayload" in source
    assert "tp(latest)" in source
    assert "TUB_MAX_SAMPLES" in source
    assert "Tub Start" in source
    assert "Download Tub JSON" not in source
    assert "LittleFS" not in source
    assert "SPIFFS" not in source


def test_web_console_sta_refresh_does_not_overwrite_open_modal_input():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "function isWifiStaModalOpen()" in source
    assert "async function refreshWifiSta(forceFill=false)" in source
    assert "if(forceFill||(!isWifiStaModalOpen()&&document.activeElement!==staSsid))" in source
    assert "async function openWifiStaModal(){await refreshWifiSta(true);wifiStaModal.classList.add('show')}" in source


def test_web_console_sta_save_defers_wifi_reconnect_until_after_http_response():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    save_body = re.search(
        r"static bool saveWifiStaPreference\(const String& ssid, const String& password\)\s*\{(?P<body>.*?)\n\}",
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
    source = MUS4_SKETCH.read_text(encoding="utf-8")

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
    assert "setTimeout(resolve,1000)" in save_body
    assert "waitWifiStaConnectionResult()" in save_body
    assert save_body.index("setTimeout(resolve,1000)") < save_body.index("waitWifiStaConnectionResult()")
    assert "showCommandError(t)" not in save_body


def test_runtime_sta_disconnect_does_not_reset_soft_ap():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    apply_body = re.search(
        r"static void applyWifiStaCredentials\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    clear_body = re.search(
        r"static bool clearWifiStaPreference\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    setup_body = re.search(
        r"static void setupWifiConsole\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")

    assert "static void disconnectWifiStaOnly()" in source
    assert "esp_wifi_disconnect()" in source
    assert "disconnectWifiStaOnly()" in apply_body
    assert "disconnectWifiStaOnly()" in clear_body
    assert "WiFi.disconnect(false, false)" not in apply_body
    assert "WiFi.disconnect(false, false)" not in clear_body
    assert "WiFi.disconnect(true, true)" in setup_body


def test_web_console_handles_windows_connectivity_probe_locally():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "#include <DNSServer.h>" in source
    assert "DNSServer wifiCaptiveDnsServer" in source
    assert "wifiCaptiveDnsServer.start" in source
    assert "wifiCaptiveDnsServer.processNextRequest()" in source
    assert "handleWifiWebWindowsConnectTest" in source
    assert "Microsoft Connect Test" in source
    assert "Microsoft NCSI" in source
    assert 'wifiWebServer.on("/connecttest.txt", HTTP_GET, handleWifiWebWindowsConnectTest)' in source
    assert 'wifiWebServer.on("/ncsi.txt", HTTP_GET, handleWifiWebWindowsNcsi)' in source
