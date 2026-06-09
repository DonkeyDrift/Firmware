import pathlib
import re


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
MUS4_SKETCH = PROJECT_ROOT / "MUS4_FW.ino"
ARDUINO_WSL_SCRIPT = PROJECT_ROOT / "arduino-cli-wsl.ps1"
CONFIG_YAML = PROJECT_ROOT / "config.yaml"
WSLBUILD_YAML = PROJECT_ROOT / "wslbuild.yaml"
SMART_PROVISIONING_SKETCH = PROJECT_ROOT / "examples" / "smart_provisioning" / "smart_provisioning.ino"
SMART_PROVISIONING_WEB_UI = PROJECT_ROOT / "examples" / "smart_provisioning" / "web_ui.h"


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
    assert 'id="networkMdnsValue"' in source
    assert 'id="networkIpValue"' not in source
    assert 'id="networkSub"' not in source
    assert '<b>SSID</b><span id="networkSsidValue">--</span><b>LAN</b><span id="networkMdnsValue" onclick="openNetworkLanUrl()">--</span>' in source
    assert '<b>REMAIN</b><span id="voltageSub">battery</span>' in source
    assert 'onclick="event.stopPropagation();openNetworkSettings()"' in source
    assert '<button class="gear" onclick="event.stopPropagation();openWifiStaModal()">' not in source
    assert 'ap_ssid=\\"%s\\"' in source
    assert 'sta_ssid=\\"%s\\"' in source
    assert "networkTabPinned" in source
    assert "staConnected?'sta':'ap'" in source
    assert ".netTabs{position:absolute;right:28px;top:8px;" in source
    assert "networkSub.textContent" not in source
    assert "networkIpValue.textContent" not in source
    assert "networkMdnsValue.textContent" in source
    assert "openNetworkLanUrl" in source
    assert "mdns_url" in source
    assert ".local 打不开时请使用 STA IP" in source
    assert "LAN 名称不可用，请使用 STA IP" in source
    assert "v.toFixed(1)+'V'" in source
    assert "if(!isNaN(v)&&v>=5)" in source
    assert "voltageValue.textContent='未连接'" in source
    assert "if(!isNaN(v)&&v>0)" not in source
    assert "v.toFixed(2)+'V'" not in source


def test_web_console_ap_ssid_modal_and_api_are_present():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert 'id="wifiApModal"' in source
    assert 'AP SSID 配置' in source
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
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "static bool isMdnsSafeHostnameChar(char c)" in source
    assert "static bool isMdnsSafeHostname(const String& value)" in source
    assert "if (!isMdnsSafeHostname(ssid)) return false;" in source
    assert "c >= 'A' && c <= 'Z'" in source
    assert "c >= 'a' && c <= 'z'" in source
    assert "c >= '0' && c <= '9'" in source
    assert "c == '-'" in source
    assert "value[0] == '-'" in source
    assert "value[value.length() - 1] == '-'" in source
    assert "SSID 只能使用字母、数字和短横线" in source
    assert "invalid_ssid" in source


def test_web_console_exposes_ap_name_mdns_lan_console_entry():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "#include <ESPmDNS.h>" in source
    assert "bool wifiMdnsStarted" in source
    assert "static void startWifiMdnsIfNeeded()" in source
    assert "static void stopWifiMdnsIfNeeded()" in source
    assert "static String wifiMdnsHostText()" in source
    assert "static String wifiMdnsUrlText()" in source
    assert "MDNS.begin(wifiMdnsHostText().c_str())" in source
    assert 'MDNS.addService("http", "tcp", WIFI_WEB_CONSOLE_PORT)' in source
    assert "ESP.getEfuseMac" not in source


def test_web_status_and_sta_api_include_ap_name_mdns_console_url():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    status_body = re.search(
        r"static void printWirelessStatus\(Print& out\)\s*\{(?P<body>.*?)\n\}",
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
    assert "wifiMdnsHostText().c_str()" in status_body
    assert "wifiMdnsUrlText().c_str()" in status_body
    assert "wifiMdnsStarted ? 1 : 0" in status_body
    assert "\\\"mdns_host\\\"" in sta_json_body
    assert "\\\"mdns_url\\\"" in sta_json_body
    assert "\\\"mdns_started\\\"" in sta_json_body
    assert "wifiMdnsHostText().c_str()" in sta_json_body
    assert "wifiMdnsUrlText().c_str()" in sta_json_body
    assert "host.toLowerCase()" in source
    assert "String(\"http://\") + wifiMdnsHostText() + \".local/\"" in source


def test_wifi_sta_to_sta_handoff_keeps_ap_as_transition_page():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "WIFI_STA_HANDOFF_AP_KEEP_MS" not in source
    assert "WIFI_AP_STOP_AFTER_STA_CONNECTED_DELAY_MS" not in source
    assert "bool wifiStaHandoffActive" in source
    assert "char wifiStaHandoffTargetSsid" in source
    assert "static void startWifiStaHandoff" in source
    assert "static void finishWifiStaHandoff" in source
    assert "static void clearWifiStaHandoff" in source
    assert "restartWifiAp()" in re.search(
        r"static void startWifiStaHandoff.*?\n\}",
        source,
        re.DOTALL,
    ).group(0)
    assert "body.set('source',location.hostname==='192.168.4.1'?'ap':'sta')" in source
    assert "wifiWebServer.arg(\"source\")" in source
    assert "startWifiStaHandoff(ssid)" in source


def test_wifi_sta_handoff_status_api_and_web_prompt_are_present():
    source = MUS4_SKETCH.read_text(encoding="utf-8")
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


def test_web_console_keeps_ap_running_after_successful_wifi_sta_connection():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "WIFI_AP_STOP_AFTER_STA_CONNECTED_DELAY_MS" not in source
    assert "wifiApStopPending" not in source
    assert "scheduleWifiApStopAfterStaConnected" not in source
    assert "stopWifiApAfterStaConnected" not in source
    assert "AP stopped after STA connected" not in source
    assert "wifiCaptiveDnsServer.stop()" in source
    assert "WiFi.softAPdisconnect(true)" in source
    assert "WiFi.mode(WIFI_STA)" not in source

    update_sta_body = re.search(
        r"static void updateWifiSta\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    connected_branch = update_sta_body.split("if (status == WL_CONNECTED)", 1)[1].split("if (wifiStaConnected)", 1)[0]
    assert "finishWifiStaHandoff()" in connected_branch
    assert "scheduleWifiApStopAfterStaConnected" not in connected_branch
    assert "WiFi.softAP(" not in connected_branch
    assert "restartWifiAp()" not in connected_branch
    assert "scheduleWifiApRestart()" not in connected_branch
    assert "wifiApSsid" not in connected_branch


def test_web_console_keeps_ap_available_when_wifi_sta_connection_fails():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    failure_body = re.search(
        r"static void setWifiStaLastError\(const char\* code, const char\* message, bool timedOut\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert "wifiApStopPending" not in source
    assert "保留本轮连接的首个失败原因" in failure_body
    assert "if (wifiStaLastError[0] != 0) return" in failure_body
    assert "wifiStaConnecting = false" in failure_body
    assert "wifiStaConnected = false" in failure_body


def test_web_console_redirects_to_sta_ip_after_successful_wifi_sta_connection():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

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
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    apply_body = re.search(
        r"static void applyWifiStaCredentials\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    update_sta_body = re.search(
        r"static void updateWifiSta\(\)\s*\{(?P<body>.*?)\n\}",
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
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    setup_body = re.search(
        r"static void setupWifiConsole\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert setup_body.index("wifiCaptiveDnsServer.start(53, \"*\", WiFi.softAPIP())") < setup_body.index("wifiConsoleServer.begin()")
    assert setup_body.index("wifiConsoleServer.begin()") < setup_body.index("setupWifiWebConsole()")
    assert setup_body.index("setupWifiWebConsole()") < setup_body.index("wifiConsoleStarted = true")
    assert setup_body.index("wifiConsoleStarted = true") < setup_body.index("applyWifiStaCredentials()")


def test_wifi_softap_uses_explicit_ipv4_gateway_configuration():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    restart_body = re.search(
        r"static bool restartWifiAp\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    setup_body = re.search(
        r"static void setupWifiConsole\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")

    assert "static bool configureWifiSoftApNetwork()" in source
    assert "IPAddress apIp(192, 168, 4, 1)" in source
    assert "IPAddress subnet(255, 255, 255, 0)" in source
    assert "WiFi.softAPConfig(apIp, apIp, subnet)" in source
    assert "configureWifiSoftApNetwork()" in restart_body
    assert restart_body.index("configureWifiSoftApNetwork()") < restart_body.index("WiFi.softAP(")
    assert "configureWifiSoftApNetwork()" in setup_body
    assert setup_body.index("configureWifiSoftApNetwork()") < setup_body.index("WiFi.softAP(")


def test_restart_wifi_ap_restores_web_console_servers_after_sta_disconnect():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    restart_body = re.search(
        r"static bool restartWifiAp\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    update_sta_body = re.search(
        r"static void updateWifiSta\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    disconnected_branch = update_sta_body.split("if (wifiStaConnected)", 1)[1].split("if (!wifiStaConnecting)", 1)[0]

    assert "restartWifiAp()" in disconnected_branch
    assert "wifiApStopPending" not in source
    assert "WiFi.softAP(" in restart_body
    assert "wifiCaptiveDnsServer.start(53, \"*\", WiFi.softAPIP())" in restart_body
    assert "wifiConsoleServer.begin()" in restart_body
    assert "wifiConsoleServer.setNoDelay(true)" in restart_body
    assert "wifiWebServer.begin()" in restart_body
    assert "wifiConsoleStarted = true" in restart_body


def test_runtime_sta_disconnect_does_not_reset_soft_ap():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    disconnect_body = re.search(
        r"static void disconnectWifiStaOnly\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    apply_body = re.search(
        r"static void applyWifiStaCredentials\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    runtime_clear_body = re.search(
        r"static void clearWifiStaRuntimeStateWithoutDisconnect\(\)\s*\{(?P<body>.*?)\n\}",
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
    assert "wifiStaConfigured = false" in runtime_clear_body
    assert "wifiStaConnected = false" in runtime_clear_body
    assert "wifiStaConnecting = false" in runtime_clear_body
    assert "wifiStaApplyPending = false" in runtime_clear_body
    assert "clearWifiStaLastError()" in runtime_clear_body
    assert "WiFi.disconnect(true, true)" in setup_body


def test_web_console_handles_common_captive_portal_probes_locally():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

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
    assert "打开 Donkey Console" in redirect_body
    assert "WiFi.softAPIP().toString()" in redirect_body

    not_found_body = re.search(
        r"static void handleWifiWebCaptivePortalNotFound\(\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    ).group("body")
    assert 'uri.startsWith("/api/")' in not_found_body
    assert 'wifiWebServer.send(404, "application/json", "{\\"error\\":\\"not_found\\"}")' in not_found_body
    assert "redirectWifiWebCaptivePortalToRoot()" in not_found_body
