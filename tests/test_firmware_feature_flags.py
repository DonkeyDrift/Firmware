import pathlib
import re


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
MUS4_SKETCH = PROJECT_ROOT / "mus4.ino"


def test_websocket_curve_data_feature_is_enabled():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert re.search(r"^#define\s+ENABLE_WIFI_WEBSOCKET_TELEMETRY\b", source, re.MULTILINE)


def test_web_console_keeps_original_ui_and_direct_curve_path():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "Donkey Console" in source
    assert "MUS4 Web Console" not in source
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


def test_web_console_uses_dev_label_for_development_switch():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "DEV <b id=\"devModeSwitchText\">OFF</b>" in source
    assert "DEV MODE <b id=\"devModeSwitchText\">OFF</b>" not in source
    assert "DEBUG MODE <b id=\"devModeSwitchText\">OFF</b>" not in source
    assert "Auto OTA <b id=\"devModeSwitchText\">OFF</b>" not in source


def test_web_console_header_and_state_cards_keep_compact_layout():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

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


def test_web_console_header_ota_button_and_log_area_are_compact():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert source.index('<section class="panel" id="chartPanel">') < source.index('<section class="panel" id="serialPanel">')
    assert '<section class="panel" id="serialPanel">' in source
    assert "#serialPanel{display:flex;flex-direction:column}" in source
    assert "#serialPanel .log{flex:0 1 auto;min-height:calc(5 * 1.35em + 16px);max-height:calc(20 * 1.35em + 16px)}" in source
    assert "@media(min-width:900px){.grid{grid-template-columns:2fr 1fr}.wide{grid-column:1/-1}#serialPanel .log{height:calc(20 * 1.35em + 16px)}}" in source
    assert "canvas{width:100%;height:auto;aspect-ratio:38/13;" in source
    assert "#chartPanel:fullscreen canvas{width:min(100%,calc((100vh - 118px) * 38 / 13));height:auto;max-height:calc(100vh - 118px);aspect-ratio:38/13}" in source
    assert "dataMeta.textContent=transport+' realtime seq='+lastDataSeq+' +'+added" not in source
    assert "dataMeta.textContent=transport+' +'+added" not in source
    assert 'id="dataMeta"' not in source
    assert "data ready" not in source
    assert "dataMeta" not in source
    assert "@media(min-width:900px){.grid{grid-template-columns:2fr 1fr}.wide{grid-column:1/-1}}" not in source
    assert "@media(min-width:900px){.grid{grid-template-columns:1fr 2fr}.wide{grid-column:1/-1}}" not in source
    assert '.chartControls{display:flex;gap:6px;flex-wrap:wrap;align-items:center;margin-top:8px}.chartTools{margin-left:auto;display:flex;gap:6px;flex-wrap:wrap}' in source
    assert '<div class="chartControls"><button onclick="toggleChart()" id="chartBtn">暂停</button><button onclick="clearChart()">清空</button><button onclick="toggleChartFullscreen()" id="chartFullscreenBtn">全屏</button><div class="chartTools"><button onclick="ts()">Tub Start</button><button onclick="te()">Tub Stop</button><button onclick="td()">Tub JSON</button></div></div>' in source
    assert "document.getElementById('chartBtn').textContent=chartPaused?'绘制':'暂停'" in source
    assert "document.getElementById('chartFullscreenBtn').textContent=document.fullscreenElement===chartPanel?'分屏':'全屏'" in source
    assert '<button onclick="clearChart()">清空曲线</button>' not in source
    assert '<button onclick="toggleChart()" id="chartBtn">暂停曲线</button>' not in source
    assert "'暂停曲线'" not in source
    assert "'继续曲线'" not in source
    assert "'退出全屏'" not in source
    assert "'全屏曲线'" not in source
    assert '<a href="/update" target="_blank" class="otaLink"><button class="otaButton">OTA</button></a><label class="toggleSwitch"' in source
    assert '<input id="cmd"><button onclick="sendCmd()">发送</button><button onclick="clearLog()">清空</button><button onclick="togglePause()" id="pauseBtn">暂停</button>' in source
    assert 'placeholder="PING / STATUS / AUTH:mus4-debug / 0:0"' not in source
    assert "input{flex:0 1 180px;min-width:120px;max-width:220px}" in source
    assert "document.getElementById('pauseBtn').textContent=logPaused?'继续':'暂停'" in source
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
    source = MUS4_SKETCH.read_text(encoding="utf-8")

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
    assert "async function openWifiStaModal()" in source
    assert "await refreshWifiSta(true)" in source
    assert "wifiStaModal.classList.add('show')" in source


def test_web_console_sta_settings_support_scan_and_password_visibility():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "注意只能连接2.4G WiFi" in source
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
    assert "staSsid.value=ssid" in source
    assert '<label for="staSsid">SSID</label>' in source
    assert '<label for="staPassword">密码</label>' in source
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
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "static void handleWifiWebStaPassword()" in source
    assert 'wifiWebServer.on("/api/wifi-sta/password", HTTP_GET, handleWifiWebStaPassword)' in source
    assert "if (!wifiConsoleAuthenticated && !wifiDevModeEnabled)" in source
    assert "\\\"password_len\\\":" in source
    assert "appendJsonString(response, wifiStaPassword)" in source

    public_body = re.search(
        r"static String wifiStaJson\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "password_len" in public_body
    assert "appendJsonString(response, wifiStaPassword)" not in public_body
    assert "\"password\":" not in public_body


def test_web_console_sta_scan_api_uses_async_wifi_scan():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "static void handleWifiWebStaScan()" in source
    assert 'wifiWebServer.on("/api/wifi-sta/scan", HTTP_GET, handleWifiWebStaScan)' in source
    assert "WiFi.scanNetworks(true" in source
    assert "WiFi.scanComplete()" in source
    assert "WiFi.scanDelete()" in source
    assert "WiFi.RSSI" in source
    assert "WiFi.channel" in source
    assert "\\\"rssi\\\":" in source
    assert "\\\"channel\\\":" in source


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
