#include "WebConsoleServer.h"

#ifdef ENABLE_WIFI_CONSOLE

#include "JsonUtil.h"
#include "Mus4Log.h"
#include "BuildInfo.h"
#include "SharedTypes.h"
#include "StringPrint.h"
#include "WebConsoleAssets.h"
#include "WebLogBuffer.h"
#include "WifiConsoleTypes.h"
#include "WirelessConsole.h"
#include "WifiIdentity.h"
#include "WifiManager.h"
#include "WifiOta.h"
#include "WifiStaConfig.h"

#include <WebServer.h>
#include <WiFi.h>
#include <Update.h>

// Hardware/framework globals defined in MUS4_FW.ino
extern WebServer wifiWebServer;

// Runtime state aggregates defined in MUS4_FW.ino
extern WifiRuntimeState wifiRuntime;
extern OtaRuntimeState otaRuntime;

// Wi-Fi runtime state aliases (kept in MUS4_FW.ino via reference aliases)
#define ws wifiRuntime
#define os otaRuntime

// Control / sensor globals defined in MUS4_FW.ino (to be migrated later)
extern ControlData car_output;
extern SerialBuf wifiConsoleBuf;

// Web telemetry globals defined in MUS4_FW.ino (to be migrated to WebTelemetry)
extern WebDataPoint wifiWebData[WIFI_WEB_DATA_CAPACITY];
extern uint32_t wifiWebDataSeq;
extern uint16_t wifiWebDataHead;
extern uint16_t wifiWebDataCount;
extern unsigned long lastWifiWebDataSampleMs;

#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
#include "WebTelemetry.h"
#endif

// Wi-Fi scan cache globals defined in MUS4_FW.ino (to be migrated with WebConsoleServer later)
extern WifiScanEntry wifiScanCache[16];
extern uint8_t wifiScanCacheCount;

// Wi-Fi runtime helpers still in MUS4_FW.ino (to be migrated to WifiManager in slice 4)
extern void startWifiStaHandoff(const String& targetSsid);
extern void clearWifiStaHandoff();
extern void scheduleWifiStaApply();
extern void scheduleWifiApRestart();
extern bool saveDevModePreference(bool enabled);
extern bool saveWifiApPreference(const String& ssid);

// Web telemetry sampler still in MUS4_FW.ino (to be migrated to WebTelemetry in slice 3)
extern void sampleWifiWebData();

// Web-local state (moved from MUS4_FW.ino)
static String wifiWebUpdateErrorMsg;
static size_t wifiWebUpdateReceived = 0;
static unsigned long lastWifiWebUpdateMs = 0;
static uint32_t wifiWebUpdateMaxDtMs = 0;
static uint32_t wifiWebSampleMaxDtMs = 0;
static uint32_t wifiWebHttpMaxDtMs = 0;
static uint32_t wifiWebStatusRequests = 0;
static uint32_t wifiWebLogRequests = 0;
static uint32_t wifiWebDataRequests = 0;
static uint32_t wifiWebCommandRequests = 0;
static uint32_t wifiWebStatusMaxDtMs = 0;
static uint32_t wifiWebLogMaxDtMs = 0;
static uint32_t wifiWebDataMaxDtMs = 0;
static uint32_t wifiWebCommandMaxDtMs = 0;

void printWirelessStatus(Print& out)
{
    out.printf("STATUS mode=%d park=%d throttle=%d steering=%d wifi_frames=%lu wifi_errors=%lu ota_window=%d ota_progress=%u ota_ttl_ms=%lu dev_mode=%d park_guard=%d version=%s build=\"%s %s\" web_port=%u free_heap=%lu min_free_heap=%lu ws_port=%u ws_client=%d ws_dropped=%lu ws_queue_full_skip=%lu ws_heap_skip=%lu ws_frames=%lu ws_max_backlog=%lu ws_connects=%lu ws_disconnects=%lu web_update_dt_max=%lu web_sample_dt_max=%lu web_http_dt_max=%lu web_ws_dt_max=%lu http_status_count=%lu http_log_count=%lu http_data_count=%lu http_cmd_count=%lu http_status_dt_max=%lu http_log_dt_max=%lu http_data_dt_max=%lu http_cmd_dt_max=%lu ap_ssid=\"%s\" ap_ip=%s ap_clients=%u sta_configured=%d sta_connected=%d sta_ssid=\"%s\" sta_ip=%s mdns_host=\"%s\" mdns_url=%s mdns_started=%d\n",
        car_output.mode,
        car_output.park ? 1 : 0,
        car_output.throttle,
        car_output.steering,
        wifiConsoleBuf.frames,
        wifiConsoleBuf.errors,
        os.windowOpen ? 1 : 0,
        os.lastProgressPct,
        wifiOtaTtlMs(otaRuntime, wifiRuntime),
        wifiRuntime.devModeEnabled ? 1 : 0,
        otaRuntime.parkGuardActive ? 1 : 0,
        MUS4_FIRMWARE_VERSION,
        MUS4_BUILD_DATE,
        MUS4_BUILD_TIME,
        WIFI_WEB_CONSOLE_PORT,
        (unsigned long)ESP.getFreeHeap(),
        (unsigned long)WIFI_WEB_TELEMETRY_MIN_FREE_HEAP,
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
        WIFI_WEB_SOCKET_PORT,
        wifiWebSocketClientConnected ? 1 : 0,
        wifiWebSocketDroppedPoints,
        wifiWebSocketQueueFullSkips,
        wifiWebSocketHeapSkips,
        wifiWebSocketFramesSent,
        wifiWebSocketMaxBacklog,
        wifiWebSocketConnects,
        wifiWebSocketDisconnects,
#else
        0,
        0,
        0UL,
        0UL,
        0UL,
        0UL,
        0UL,
        0UL,
        0UL,
#endif
        wifiWebUpdateMaxDtMs,
        wifiWebSampleMaxDtMs,
        wifiWebHttpMaxDtMs,
        wifiWebSocketMaxDtMs,
        wifiWebStatusRequests,
        wifiWebLogRequests,
        wifiWebDataRequests,
        wifiWebCommandRequests,
        wifiWebStatusMaxDtMs,
        wifiWebLogMaxDtMs,
        wifiWebDataMaxDtMs,
        wifiWebCommandMaxDtMs,
        getActiveWifiApSsid().c_str(),
        WiFi.softAPIP().toString().c_str(),
        WiFi.softAPgetStationNum(),
        wifiRuntime.staConfigured ? 1 : 0,
        wifiRuntime.staConnected ? 1 : 0,
        wifiRuntime.staSsid,
        wifiStaIpText().c_str(),
        wifiMdnsHostText().c_str(),
        wifiMdnsUrlText().c_str(),
        wifiRuntime.mdnsStarted ? 1 : 0);
}

static void redirectWifiWebCaptivePortalToRoot()
{
    String url = String("http://") + WiFi.softAPIP().toString() + "/";
    wifiWebServer.sendHeader("Cache-Control", "no-store");
    wifiWebServer.sendHeader("Location", url);
    wifiWebServer.send(302, "text/plain", "");
}

static void handleWifiWebRoot()
{
    wifiWebServer.send_P(200, "text/html", WIFI_WEB_CONSOLE_HTML);
}

static void handleWifiWebCaptivePortal()
{
    redirectWifiWebCaptivePortalToRoot();
}

static void handleWifiWebCaptivePortalRedirectPage()
{
    String host = wifiWebServer.hostHeader();
    if (host.length() == 0 || host.indexOf(WiFi.softAPIP().toString()) >= 0) {
        host = WiFi.softAPIP().toString();
    }
    String url = String("http://") + host + "/";
    String response = String("<!doctype html><html><head><meta charset=\"utf-8\">") +
        "<meta http-equiv=\"refresh\" content=\"0;url=" + url + "\">" +
        "<script>location.replace('" + url + "');</script></head>" +
        "<body><a href=\"" + url + "\">打开 Drifter Console</a></body></html>";
    wifiWebServer.sendHeader("Cache-Control", "no-store");
    wifiWebServer.send(200, "text/html", response);
}

static void handleWifiWebWindowsConnectTest()
{
    handleWifiWebCaptivePortal();
}

static void handleWifiWebWindowsNcsi()
{
    handleWifiWebCaptivePortal();
}

static void handleWifiWebCaptivePortalNotFound()
{
    String uri = wifiWebServer.uri();
    if (uri.startsWith("/api/")) {
        wifiWebServer.sendHeader("Cache-Control", "no-store");
        wifiWebServer.send(404, "application/json", "{\"error\":\"not_found\"}");
        return;
    }
    redirectWifiWebCaptivePortalToRoot();
}

static void recordWifiWebHandlerDt(unsigned long startedMs, uint32_t& maxDtMs)
{
    uint32_t dt = (uint32_t)(millis() - startedMs);
    if (dt > maxDtMs) maxDtMs = dt;
}

static void sendWifiWebApiHeaders()
{
    wifiWebServer.sendHeader("Cache-Control", "no-store");
}

static void handleWifiWebStatus()
{
    unsigned long startedMs = millis();
    wifiWebStatusRequests++;
    String response;
    StringPrint out(response);
    printWirelessStatus(out);
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "text/plain", response);
    recordWifiWebHandlerDt(startedMs, wifiWebStatusMaxDtMs);
}

static void handleWifiWebCommand()
{
    unsigned long startedMs = millis();
    wifiWebCommandRequests++;
    String line = wifiWebServer.arg("plain");
    line.trim();
    if (line.length() == 0) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(400, "text/plain", "NACK:EMPTY\n");
        appendWebLog("web", "> <empty>");
        appendWebLog("cmd", "NACK:EMPTY");
        recordWifiWebHandlerDt(startedMs, wifiWebCommandMaxDtMs);
        return;
    }
    String target = wifiWebServer.arg("target");
    target.trim();
    String response;
    StringPrint out(response);
    if (target.equalsIgnoreCase("serial")) {
        appendWebLog("web", String("> [serial] ") + redactWirelessConsoleLine(line));
        Serial.println(line);
        out.println("ACK:SERIAL");
    } else if (target.equalsIgnoreCase("serial1")) {
        appendWebLog("web", String("> [serial1] ") + redactWirelessConsoleLine(line));
        Serial1.println(line);
        out.println("ACK:SERIAL1");
    } else {
        appendWebLog("web", String("> ") + redactWirelessConsoleLine(line));
        processWirelessConsoleLine(line, out, WIRELESS_ORIGIN_WEB);
        appendWebLogLines("cmd", response);
    }
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "text/plain", response);
    recordWifiWebHandlerDt(startedMs, wifiWebCommandMaxDtMs);
}

static void handleWifiWebDevMode()
{
    String response = String("{\"enabled\":") + (ws.devModeEnabled ? "true" : "false") + "}";
    wifiWebServer.send(200, "application/json", response);
}

static void handleWifiWebDevModeSet()
{
    String body = wifiWebServer.arg("plain");
    body.trim();
    body.toLowerCase();
    bool enabled;
    if (body == "1" || body == "true" || body == "on") {
        enabled = true;
    } else if (body == "0" || body == "false" || body == "off") {
        enabled = false;
    } else {
        wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_value\"}");
        return;
    }
    if (!saveDevModePreference(enabled)) {
        wifiWebServer.send(500, "application/json", "{\"saved\":false}");
        return;
    }
    String response = String("{\"enabled\":") + (ws.devModeEnabled ? "true" : "false") + ",\"saved\":true}";
    wifiWebServer.send(200, "application/json", response);
}

static String wifiApJson()
{
    String response;
    response.reserve(128);
    // Return the configured base SSID (<prefix>-ESP) so the config dialog
    // can edit the prefix. The live broadcast SSID may differ in DEV mode.
    response += "{\"ssid\":";
    appendJsonString(response, ws.apSsid);
    response += ",\"ip\":";
    appendJsonString(response, WiFi.softAPIP().toString().c_str());
    response += ",\"clients\":";
    response += WiFi.softAPgetStationNum();
    response += "}";
    return response;
}

static void handleWifiWebAp()
{
    wifiWebServer.send(200, "application/json", wifiApJson());
}

static void handleWifiWebApSet()
{
    if (!ws.consoleAuthenticated && !ws.devModeEnabled) {
        wifiWebServer.send(403, "application/json", "{\"error\":\"auth_required\"}");
        return;
    }
    String prefix = wifiWebServer.arg("ssid");
    prefix.trim();
    if (!isValidApSsidPrefix(prefix)) {
        wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_ssid\"}");
        return;
    }
    String fullSsid = prefix + WIFI_AP_SSID_SUFFIX;
    if (!saveWifiApPreference(fullSsid)) {
        wifiWebServer.send(500, "application/json", "{\"saved\":false}");
        return;
    }
    appendWebLog("web", String("wifi ap saved ssid=") + ws.apSsid);
    wifiWebServer.send(200, "application/json", String("{\"saved\":true,\"restart_pending\":true,\"state\":") + wifiApJson() + "}");
    scheduleWifiApRestart();
}

static String wifiStaJson()
{
    String response;
    response.reserve(320);
    response += "{\"configured\":";
    response += ws.staConfigured ? "true" : "false";
    response += ",\"connected\":";
    response += ws.staConnected ? "true" : "false";
    response += ",\"timed_out\":";
    response += ws.staTimedOut ? "true" : "false";
    response += ",\"connecting\":";
    response += ws.staConnecting ? "true" : "false";
    response += ",\"last_error\":";
    appendJsonString(response, ws.staConnected ? "" : ws.staLastError);
    response += ",\"last_error_message\":";
    appendJsonString(response, ws.staConnected ? "" : ws.staLastErrorMessage);
    response += ",\"ssid\":";
    appendJsonString(response, ws.staSsid);
    response += ",\"password_set\":";
    response += ws.staPasswordSet ? "true" : "false";
    response += ",\"password_len\":";
    response += ws.staPasswordSet ? strlen(ws.staPassword) : 0;
    response += ",\"ap_ip\":";
    appendJsonString(response, WiFi.softAPIP().toString().c_str());
    response += ",\"sta_ip\":";
    appendJsonString(response, wifiStaIpText().c_str());
    response += ",\"mdns_host\":";
    appendJsonString(response, wifiMdnsHostText().c_str());
    response += ",\"mdns_url\":";
    appendJsonString(response, wifiMdnsUrlText().c_str());
    response += ",\"mdns_started\":";
    response += ws.mdnsStarted ? "true" : "false";
    response += ",\"handoff_active\":";
    response += ws.staHandoffActive ? "true" : "false";
    response += ",\"handoff_target_ssid\":";
    appendJsonString(response, ws.staHandoffTargetSsid);
    response += ",\"handoff_sta_ip\":";
    appendJsonString(response, ws.staHandoffStaIp[0] ? ws.staHandoffStaIp : wifiStaIpText().c_str());
    response += ",\"handoff_ap_ssid\":";
    appendJsonString(response, ws.staHandoffApSsid[0] ? ws.staHandoffApSsid : getActiveWifiApSsid().c_str());
    response += ",\"handoff_ap_url\":";
    appendJsonString(response, "http://192.168.4.1/");
    response += ",\"handoff_mdns_url\":";
    appendJsonString(response, wifiMdnsUrlText().c_str());
    response += "}";
    return response;
}

static void handleWifiWebSta()
{
    wifiWebServer.send(200, "application/json", wifiStaJson());
}

static void handleWifiWebStaPassword()
{
    if (!ws.consoleAuthenticated && !ws.devModeEnabled) {
        wifiWebServer.send(403, "application/json", "{\"error\":\"auth_required\"}");
        return;
    }
    String response;
    response.reserve(128);
    response += "{\"password_set\":";
    response += ws.staPasswordSet ? "true" : "false";
    response += ",\"password_len\":";
    response += ws.staPasswordSet ? strlen(ws.staPassword) : 0;
    response += ",\"password\":";
    if (ws.staPasswordSet) appendJsonString(response, ws.staPassword);
    else appendJsonString(response, "");
    response += '}';
    wifiWebServer.send(200, "application/json", response);
}

static void startWifiStaScan()
{
    if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return;
    WiFi.scanNetworks(true, true);
}

static void cacheWifiStaScanResults(int count)
{
    wifiScanCacheCount = 0;
    for (int i = 0; i < count && wifiScanCacheCount < 16; i++) {
        String ssid = WiFi.SSID(i);
        ssid.trim();
        int32_t channel = WiFi.channel(i);
        if (ssid.length() == 0 || channel < 1 || channel > 14) continue;
        int32_t rssi = WiFi.RSSI(i);
        int existing = -1;
        for (uint8_t j = 0; j < wifiScanCacheCount; j++) {
            if (ssid.equals(wifiScanCache[j].ssid)) {
                existing = j;
                break;
            }
        }
        if (existing >= 0 && rssi <= wifiScanCache[existing].rssi) continue;
        WifiScanEntry& entry = existing >= 0 ? wifiScanCache[existing] : wifiScanCache[wifiScanCacheCount++];
        ssid.toCharArray(entry.ssid, sizeof(entry.ssid));
        entry.rssi = rssi;
        entry.channel = channel;
        entry.secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }
    for (uint8_t i = 0; i < wifiScanCacheCount; i++) {
        for (uint8_t j = i + 1; j < wifiScanCacheCount; j++) {
            if (wifiScanCache[j].rssi > wifiScanCache[i].rssi) {
                WifiScanEntry tmp = wifiScanCache[i];
                wifiScanCache[i] = wifiScanCache[j];
                wifiScanCache[j] = tmp;
            }
        }
    }
}

static void handleWifiWebStaScan()
{
    int result = WiFi.scanComplete();
    bool scanning = result == WIFI_SCAN_RUNNING;
    if (result >= 0) {
        cacheWifiStaScanResults(result);
        WiFi.scanDelete();
        startWifiStaScan();
        scanning = false;
    } else if (result != WIFI_SCAN_RUNNING) {
        startWifiStaScan();
        scanning = true;
    }
    String response;
    response.reserve(640);
    response += "{\"scanning\":";
    response += scanning ? "true" : "false";
    response += ",\"networks\":[";
    for (uint8_t i = 0; i < wifiScanCacheCount; i++) {
        if (i > 0) response += ',';
        response += "{\"ssid\":";
        appendJsonString(response, wifiScanCache[i].ssid);
        response += ",\"rssi\":";
        response += wifiScanCache[i].rssi;
        response += ",\"channel\":";
        response += wifiScanCache[i].channel;
        response += ",\"secure\":";
        response += wifiScanCache[i].secure ? "true" : "false";
        response += '}';
    }
    response += "]}";
    wifiWebServer.send(200, "application/json", response);
}

static void handleWifiWebStaSet()
{
    if (!ws.consoleAuthenticated && !ws.devModeEnabled) {
        wifiWebServer.send(403, "application/json", "{\"error\":\"auth_required\"}");
        return;
    }
    String ssid = wifiWebServer.arg("ssid");
    String password = wifiWebServer.arg("password");
    String sourceArg = wifiWebServer.arg("source");
    bool keepPassword = wifiWebServer.arg("keep_password") == "1";
    ssid.trim();
    if (ssid.length() == 0 || ssid.length() > WIFI_STA_SSID_MAX_LEN) {
        wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_ssid\"}");
        return;
    }
    bool staHandoffRequested = ws.staConnected && sourceArg == "sta" && !ssid.equals(WiFi.SSID());
    if (keepPassword) {
        if (!ws.staPasswordSet) {
            wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_password\"}");
            return;
        }
        if (!saveWifiStaSsidPreference(ssid)) {
            wifiWebServer.send(500, "application/json", "{\"saved\":false}");
            return;
        }
    } else {
        if (password.length() > 0 && (password.length() < WIFI_STA_PASSWORD_MIN_LEN || password.length() > WIFI_STA_PASSWORD_MAX_LEN)) {
            wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_password\"}");
            return;
        }
        if (!saveWifiStaPreference(ssid, password)) {
            wifiWebServer.send(500, "application/json", "{\"saved\":false}");
            return;
        }
    }
    if (staHandoffRequested) {
        startWifiStaHandoff(ssid);
    } else {
        clearWifiStaHandoff();
    }
    appendWebLog("web", String("wifi sta saved ssid=") + ws.staSsid + " password=<redacted>");
    wifiWebServer.send(200, "application/json", String("{\"saved\":true,\"applied\":true,\"state\":") + wifiStaJson() + "}");
    scheduleWifiStaApply();
}

static void handleWifiWebStaClear()
{
    if (!ws.consoleAuthenticated && !ws.devModeEnabled) {
        wifiWebServer.send(403, "application/json", "{\"error\":\"auth_required\"}");
        return;
    }
    if (!clearWifiStaPreference()) {
        wifiWebServer.send(500, "application/json", "{\"cleared\":false}");
        return;
    }
    appendWebLog("web", "wifi sta cleared");
    wifiWebServer.send(200, "application/json", "{\"cleared\":true}");
}

static void handleWifiWebLog()
{
    unsigned long startedMs = millis();
    wifiWebLogRequests++;
    uint32_t since = wifiWebServer.hasArg("since") ? (uint32_t)wifiWebServer.arg("since").toInt() : 0;
    String response;
    response.reserve(512);
    writeWebLogsJson(response, since);
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "application/json", response);
    recordWifiWebHandlerDt(startedMs, wifiWebLogMaxDtMs);
}

static void appendWifiWebPlotPointJson(String& response, WebDataPoint& point)
{
    response += "{\"seq\":";
    response += point.seq;
    response += ",\"t\":";
    response += point.t;
    response += ",\"dt\":";
    response += point.dtMs;
    response += ",\"thr\":";
    response += point.throttle;
    response += ",\"str\":";
    response += point.steering;
    response += ",\"gz\":";
    response += String(point.gyroZ, 3);
    response += '}';
}

static void appendWifiWebStateJson(String& response, WebDataPoint& point)
{
    response += "{\"seq\":";
    response += point.seq;
    response += ",\"t\":";
    response += point.t;
    response += ",\"dt\":";
    response += point.dtMs;
    response += ",\"thr\":";
    response += point.throttle;
    response += ",\"str\":";
    response += point.steering;
    response += ",\"mode\":";
    response += point.mode;
    response += ",\"park\":";
    response += point.park ? 1 : 0;
    response += ",\"rct\":";
    response += point.rcThrottle;
    response += ",\"rcs\":";
    response += point.rcSteering;
    response += ",\"ch1\":";
    response += point.rcChannels[CH_STEERING];
    response += ",\"ch2\":";
    response += point.rcChannels[CH_THROTTLE];
    response += ",\"ch3\":";
    response += point.rcChannels[CH_PARK];
    response += ",\"ch4\":";
    response += point.rcChannels[CH_MODE];
    response += ",\"ch5\":";
    response += point.rcChannels[CH_DRIFT];
    response += ",\"ch6\":";
    response += point.rcChannels[CH_DRIFT_SCALE];
    response += ",\"pt\":";
    response += point.pilotThrottle;
    response += ",\"ps\":";
    response += point.pilotSteering;
    response += ",\"cur\":";
    response += String(point.currentMa, 2);
    response += ",\"vol\":";
    response += String(point.voltage, 2);
    response += ",\"gz\":";
    response += String(point.gyroZ, 3);
    response += ",\"de\":";
    response += point.driftEnabled ? 1 : 0;
    response += ",\"da\":";
    response += point.driftActive ? 1 : 0;
    response += ",\"dc\":";
    response += String(point.driftCompensation, 2);
    response += ",\"gzf\":";
    response += String(point.gyroZFiltered, 3);
    response += '}';
}

static void handleWifiWebData()
{
    unsigned long startedMs = millis();
    wifiWebDataRequests++;
    uint32_t since = wifiWebServer.hasArg("since") ? (uint32_t)wifiWebServer.arg("since").toInt() : 0;
    String response;
    response.reserve(768);
    response += "{\"points\":[";
    bool first = true;
    for (uint16_t i = 0; i < wifiWebDataCount; i++) {
        uint16_t index = (wifiWebDataHead + WIFI_WEB_DATA_CAPACITY - wifiWebDataCount + i) % WIFI_WEB_DATA_CAPACITY;
        WebDataPoint& point = wifiWebData[index];
        if (point.seq <= since) continue;
        if (!first) response += ',';
        first = false;
        appendWifiWebPlotPointJson(response, point);
    }
    response += "],\"latest\":";
    if (wifiWebDataCount > 0) {
        uint16_t latestIndex = (wifiWebDataHead + WIFI_WEB_DATA_CAPACITY - 1) % WIFI_WEB_DATA_CAPACITY;
        appendWifiWebStateJson(response, wifiWebData[latestIndex]);
    } else {
        response += "null";
    }
    response += '}';
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "application/json", response);
    recordWifiWebHandlerDt(startedMs, wifiWebDataMaxDtMs);
}

static void handleWifiWebUpdateGet()
{
    wifiWebServer.send_P(200, "text/html", WIFI_WEB_UPDATE_HTML);
}

static bool isWifiWebUpdateAuthOk()
{
    if (ws.devModeEnabled) return true;
    if (ws.consoleAuthenticated) return true;
    // Allow one-shot auth via query parameter for scripted uploads
    if (wifiWebServer.hasArg("auth") && wifiWebServer.arg("auth").equals(WIFI_CONSOLE_AP_PASSWORD)) {
        return true;
    }
    return false;
}

static void handleWifiWebUpdateUpload()
{
    HTTPUpload& upload = wifiWebServer.upload();
    if (upload.status == UPLOAD_FILE_START) {
        wifiWebUpdateErrorMsg = "";
        wifiWebUpdateReceived = 0;
        if (!isWifiWebUpdateAuthOk()) {
            wifiWebUpdateErrorMsg = "NACK:AUTH_REQUIRED";
            mus4LogLine("ota", "http update rejected: auth required");
            return;
        }
        if (car_output.park != PARK_LOCKED) {
            wifiWebUpdateErrorMsg = "NACK:PARK_REQUIRED";
            mus4LogLine("ota", "http update rejected: park required");
            return;
        }
        os.parkGuardActive = true;
        forceWifiOtaParkLocked();
        os.inProgress = true;
        os.windowOpen = true;
        os.lastProgressPct = 0;
        if (!Update.begin(upload.totalSize > 0 ? upload.totalSize : UPDATE_SIZE_UNKNOWN)) {
            wifiWebUpdateErrorMsg = "NACK:BEGIN_FAILED:" + String(Update.errorString());
            mus4Logf("ota", "http update begin failed: %s", Update.errorString());
        } else {
            mus4LogLine("ota", "http update begin");
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (wifiWebUpdateErrorMsg.length() > 0) return;
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            wifiWebUpdateErrorMsg = "NACK:WRITE_FAILED";
            mus4Logf("ota", "http update write failed at %u", wifiWebUpdateReceived);
        } else {
            wifiWebUpdateReceived += upload.currentSize;
            if (upload.totalSize > 0) {
                os.lastProgressPct = (uint8_t)((wifiWebUpdateReceived * 100U) / upload.totalSize);
            }
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (wifiWebUpdateErrorMsg.length() > 0) {
            Update.end();
            return;
        }
        if (!Update.end(true)) {
            wifiWebUpdateErrorMsg = "NACK:END_FAILED:" + String(Update.errorString());
            mus4Logf("ota", "http update end failed: %s", Update.errorString());
        } else {
            os.lastProgressPct = 100;
            mus4LogLine("ota", "http update success");
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.end();
        wifiWebUpdateErrorMsg = "NACK:ABORTED";
        os.inProgress = false;
        mus4LogLine("ota", "http update aborted");
    }
}

static void handleWifiWebUpdatePost()
{
    unsigned long startedMs = millis();
    sendWifiWebApiHeaders();
    if (wifiWebUpdateErrorMsg.length() > 0) {
        wifiWebServer.send(500, "text/plain", wifiWebUpdateErrorMsg + "\n");
        recordWifiWebHandlerDt(startedMs, wifiWebHttpMaxDtMs);
        return;
    }
    wifiWebServer.send(200, "text/plain", "ACK:UPDATE_OK\n");
    recordWifiWebHandlerDt(startedMs, wifiWebHttpMaxDtMs);
    delay(100);
    ESP.restart();
}

void setupWebConsoleServer()
{
    wifiWebServer.on("/", HTTP_GET, handleWifiWebRoot);
    wifiWebServer.on("/connecttest.txt", HTTP_GET, handleWifiWebWindowsConnectTest);
    wifiWebServer.on("/ncsi.txt", HTTP_GET, handleWifiWebWindowsNcsi);
    wifiWebServer.on("/redirect", HTTP_GET, handleWifiWebCaptivePortalRedirectPage);
    wifiWebServer.on("/hotspot-detect.html", HTTP_GET, handleWifiWebCaptivePortal);
    wifiWebServer.on("/library/test/success.html", HTTP_GET, handleWifiWebCaptivePortal);
    wifiWebServer.on("/success.txt", HTTP_GET, handleWifiWebCaptivePortal);
    wifiWebServer.on("/generate_204", HTTP_GET, handleWifiWebCaptivePortal);
    wifiWebServer.on("/gen_204", HTTP_GET, handleWifiWebCaptivePortal);
    wifiWebServer.on("/mobile/status.php", HTTP_GET, handleWifiWebCaptivePortal);
    wifiWebServer.on("/connectivity-check.html", HTTP_GET, handleWifiWebCaptivePortal);
    wifiWebServer.on("/api/status", HTTP_GET, handleWifiWebStatus);
    wifiWebServer.on("/api/cmd", HTTP_POST, handleWifiWebCommand);
    wifiWebServer.on("/api/devmode", HTTP_GET, handleWifiWebDevMode);
    wifiWebServer.on("/api/devmode", HTTP_POST, handleWifiWebDevModeSet);
    wifiWebServer.on("/api/wifi-ap", HTTP_GET, handleWifiWebAp);
    wifiWebServer.on("/api/wifi-ap", HTTP_POST, handleWifiWebApSet);
    wifiWebServer.on("/api/wifi-sta", HTTP_GET, handleWifiWebSta);
    wifiWebServer.on("/api/wifi-sta", HTTP_POST, handleWifiWebStaSet);
    wifiWebServer.on("/api/wifi-sta/password", HTTP_GET, handleWifiWebStaPassword);
    wifiWebServer.on("/api/wifi-sta/scan", HTTP_GET, handleWifiWebStaScan);
    wifiWebServer.on("/api/wifi-sta/clear", HTTP_POST, handleWifiWebStaClear);
    wifiWebServer.on("/api/log", HTTP_GET, handleWifiWebLog);
    wifiWebServer.on("/api/data", HTTP_GET, handleWifiWebData);
    wifiWebServer.on("/update", HTTP_GET, handleWifiWebUpdateGet);
    wifiWebServer.on("/update", HTTP_POST, handleWifiWebUpdatePost, handleWifiWebUpdateUpload);
    wifiWebServer.onNotFound(handleWifiWebCaptivePortalNotFound);
    wifiWebServer.begin();
}

void updateWebConsoleServer()
{
    if (!ws.consoleStarted) return;
    unsigned long now = millis();
    if (lastWifiWebUpdateMs != 0) {
        uint32_t dt = (uint32_t)(now - lastWifiWebUpdateMs);
        if (dt > wifiWebUpdateMaxDtMs) wifiWebUpdateMaxDtMs = dt;
    }
    lastWifiWebUpdateMs = now;
    unsigned long stageStart = millis();
    sampleWifiWebData();
    uint32_t stageDt = (uint32_t)(millis() - stageStart);
    if (stageDt > wifiWebSampleMaxDtMs) wifiWebSampleMaxDtMs = stageDt;
    stageStart = millis();
    wifiWebServer.handleClient();
    stageDt = (uint32_t)(millis() - stageStart);
    if (stageDt > wifiWebHttpMaxDtMs) wifiWebHttpMaxDtMs = stageDt;
}

#endif
