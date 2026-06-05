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


def test_web_console_uses_dev_mode_label_for_development_switch():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "DEV MODE <b id=\"devModeSwitchText\">OFF</b>" in source
    assert "DEBUG MODE <b id=\"devModeSwitchText\">OFF</b>" not in source
    assert "Auto OTA <b id=\"devModeSwitchText\">OFF</b>" not in source


def test_web_console_header_and_state_cards_keep_compact_layout():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert ".headerRow{display:flex;align-items:flex-end;" in source
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
    assert ".stateCard{position:relative;overflow:hidden;border:1px solid #344154;border-radius:10px;padding:12px" in source
    assert ".stateValue{font-size:24px;font-weight:800;margin-top:4px;white-space:normal;overflow:visible;text-overflow:clip;word-break:normal;overflow-wrap:normal;line-height:1.08}" in source
    assert ".stateMeta span{font-size:15px;font-weight:700;white-space:normal;overflow:visible;text-overflow:clip;word-break:normal;overflow-wrap:normal;line-height:1.2}" in source
    assert "text-overflow:ellipsis" not in source


def test_web_console_places_voltage_before_combined_network_card():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    drift_index = source.index('id="driftCard"')
    voltage_index = source.index('id="voltageCard"')
    network_index = source.index('id="networkCard"')

    assert 'id="apCard"' not in source
    assert 'id="staCard"' not in source
    assert drift_index < voltage_index < network_index


def test_web_console_network_card_uses_ap_sta_tabs_with_ssid_and_ip():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert 'id="networkApTab"' in source
    assert 'id="networkStaTab"' in source
    assert 'id="networkSsidValue"' in source
    assert 'id="networkIpValue"' not in source
    assert 'id="networkSub"' not in source
    assert '<b>SSID</b><span id="networkSsidValue">--</span>' in source
    assert '<b>REMAIN</b><span id="voltageSub">battery</span>' in source
    assert 'openWifiStaModal()' in source
    assert 'ap_ssid=\\"%s\\"' in source
    assert 'sta_ssid=\\"%s\\"' in source
    assert "networkTabPinned" in source
    assert "staConnected?'sta':'ap'" in source
    assert ".netTabs{position:absolute;right:28px;top:8px;" in source
    assert "networkSub.textContent" not in source
    assert "networkIpValue.textContent" not in source
    assert "v.toFixed(1)+'V'" in source
    assert "if(!isNaN(v)&&v>=5)" in source
    assert "voltageValue.textContent='未连接'" in source
    assert "if(!isNaN(v)&&v>0)" not in source
    assert "v.toFixed(2)+'V'" not in source


def test_web_console_network_ip_click_copies_with_non_blocking_toast():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert 'id="networkValue" onclick="copyNetworkIp()"' in source
    assert 'title="点击复制 IP"' not in source
    assert 'id="toast" class="toast"' in source
    assert ".copyValue{cursor:pointer;position:relative}" in source
    assert ".copyValue:hover:after" in source
    assert "content:'点击复制 IP'" in source
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
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert 'id="rcFold" class="fold"' in source
    assert 'id="statusFold" class="fold"' in source
    assert '<span class="foldIcon">▸</span>RC Channels' in source
    assert '<span class="foldIcon">▸</span>STATUS Details' in source
    assert 'aria-expanded="false"><span class="foldIcon">▸</span>RC Channels' in source
    assert '.fold:not(.open) .foldBody{display:none}' in source
    assert "function toggleFold(id)" in source
    assert "function renderStatus(t)" in source
    assert "function parseStatusPairs(t)" in source
    assert "statusBox.textContent=t;updateNetworkCard" not in source


def test_web_console_status_parser_preserves_quoted_values_with_spaces():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "t.trim().split(/\\s+/)" not in source
    assert "while(i<n&&t[i]!==q)" in source
    assert "parseStatusPairs(t).forEach" in source


def test_web_console_status_details_use_responsive_columns():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert ".statusTable{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));" in source
    assert "@media(max-width:900px){.statusTable{grid-template-columns:repeat(2,minmax(0,1fr))}}" in source
    assert "@media(max-width:560px){.statusTable{grid-template-columns:1fr}}" in source


def test_web_console_explains_auth_and_park_rejections():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "function explainCommandError(t)" in source
    assert "请将 CH3/Park 切到锁定状态后重试" in source
    assert "请先 AUTH，或开启 DEV MODE 后重试" in source
    assert "请先 AUTH，或开启 DEBUG MODE 后重试" not in source
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
