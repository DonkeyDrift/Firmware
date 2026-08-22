#include "WebConsoleServer.h"

#ifdef ENABLE_WIFI_CONSOLE

#include "JsonUtil.h"
#include "Mus4Log.h"
#include "BuildInfo.h"
#include "MutePreference.h"
#include "LedStatus.h"
#include "SharedTypes.h"
#include "StringPrint.h"
#include "WebConsoleAssets.h"
#include "WebConsoleFavicon.h"
#include "WebLogBuffer.h"
#include "WifiConsoleTypes.h"
#include "WirelessConsole.h"
#include "JoystickCalibration.h"
#include "WifiIdentity.h"
#include "WifiManager.h"
#include "WifiOta.h"
#include "WifiStaConfig.h"
#include "WifiStaHistory.h"
#include "DriftAssist.h"

#include <WebServer.h>
#include <WiFi.h>
#include <Update.h>
#include <Preferences.h>

// Hardware/framework globals defined in MUS4_FW.ino
extern WebServer wifiWebServer;

// Runtime state aggregates defined in MUS4_FW.ino
extern WifiRuntimeState wifiRuntime;
extern OtaRuntimeState otaRuntime;
extern bool& wifiStaApplyFromAp;
extern uint8_t& wifiStaTargetChannel;

// Wi-Fi runtime state aliases (kept in MUS4_FW.ino via reference aliases)
#define ws wifiRuntime
#define os otaRuntime

// Control / sensor globals defined in MUS4_FW.ino (to be migrated later)
extern ControlData car_output;
extern SerialBuf wifiConsoleBuf;
extern int servo_mid_v;
extern int motor_mid_v;

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

// Web UI language preference (runtime mirror of NVS "webui"/"lang";
// "auto"/"zh"/"en", default "auto" so first boot follows the browser language)
static String webUiLang = "auto";

// 上位机配网状态（通过 Serial2 接收来自 Linux 上位机的响应）
String hostWifiStatus = "IDLE";
String hostWifiSsid = "";
String hostWifiIp = "";
String hostWifiError = "";

// 上位机周期上报的自身局域网 IP（Serial2 HOSTIP|<ipv4> 帧，仅运行时保存）
String hostReportedIp = "";
unsigned long hostReportedIpMs = 0;

void printWirelessStatus(Print& out)
{
    out.printf("STATUS mode=%d park=%d throttle=%d steering=%d wifi_frames=%lu wifi_errors=%lu ota_window=%d ota_progress=%u ota_ttl_ms=%lu dev_mode=%d park_guard=%d version=%s build=\"%s %s\" web_port=%u free_heap=%lu min_free_heap=%lu ws_port=%u ws_client=%d ws_dropped=%lu ws_queue_full_skip=%lu ws_heap_skip=%lu ws_frames=%lu ws_max_backlog=%lu ws_connects=%lu ws_disconnects=%lu web_update_dt_max=%lu web_sample_dt_max=%lu web_http_dt_max=%lu web_ws_dt_max=%lu http_status_count=%lu http_log_count=%lu http_data_count=%lu http_cmd_count=%lu http_status_dt_max=%lu http_log_dt_max=%lu http_data_dt_max=%lu http_cmd_dt_max=%lu ap_ssid=\"%s\" ap_ip=%s ap_clients=%u sta_configured=%d sta_connected=%d sta_ssid=\"%s\" sta_ip=%s mdns_host=\"%s\" mdns_url=%s mdns_started=%d host_ip=%s host_ip_age_s=%lu web_log_dropped=%lu\n",
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
        wifiWebSocket.count(),
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
        wifiApIpText().c_str(),
        WiFi.softAPgetStationNum(),
        wifiRuntime.staConfigured ? 1 : 0,
        wifiRuntime.staConnected ? 1 : 0,
        wifiRuntime.staSsid,
        wifiStaIpText().c_str(),
        wifiMdnsHostText().c_str(),
        wifiMdnsUrlText().c_str(),
        wifiRuntime.mdnsStarted ? 1 : 0,
        hostReportedIp.c_str(),
        hostReportedIpMs ? (millis() - hostReportedIpMs) / 1000UL : 0UL,
        (unsigned long)webLogBufferDropped());
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
    wifiWebServer.sendHeader("Cache-Control", "no-store");
    wifiWebServer.send_P(200, "text/html", WIFI_WEB_CONSOLE_HTML);
}

static void handleWifiWebJudge()
{
    wifiWebServer.sendHeader("Cache-Control", "no-store");
    wifiWebServer.send_P(200, "text/html", WIFI_WEB_JUDGE_HTML);
}

static void handleWifiWebDrift()
{
    wifiWebServer.sendHeader("Cache-Control", "no-store");
    wifiWebServer.send_P(200, "text/html", WIFI_WEB_DRIFT_HTML);
}

static void handleWifiWebFavicon()
{
    // 浏览器标签页图标为嵌入的静态资源，内容不变，可长缓存
    wifiWebServer.sendHeader("Cache-Control", "max-age=86400");
    wifiWebServer.send_P(200, "image/png", (PGM_P)WEB_CONSOLE_FAVICON_PNG, WEB_CONSOLE_FAVICON_PNG_LEN);
}

static void handleWifiWebCaptivePortal()
{
    redirectWifiWebCaptivePortalToRoot();
}

static void handleWifiWebCaptivePortalRedirectPage()
{
    // 直接返回根页面，不做跳转，避免触发 Windows 强制门户弹窗。
    wifiWebServer.sendHeader("Cache-Control", "no-store");
    wifiWebServer.send_P(200, "text/html", WIFI_WEB_CONSOLE_HTML);
}

static void handleWifiWebWindowsConnectTest()
{
    wifiWebServer.sendHeader("Cache-Control", "no-store");
    wifiWebServer.send(200, "text/plain", "Microsoft Connect Test");
}

static void handleWifiWebWindowsNcsi()
{
    wifiWebServer.sendHeader("Cache-Control", "no-store");
    wifiWebServer.send(200, "text/plain", "Microsoft NCSI");
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
    if (target.equalsIgnoreCase("serial") || target.equalsIgnoreCase("serial1")) {
        // v1.7.29：启用串口转发，支持上位机配网（WIFI|ssid|password 协议）。
        // 将 Web 命令原样转发到 Serial2（ESP32→Linux 上位机），并等待响应。
        appendWebLog("web", String("> [serial2] ") + redactWirelessConsoleLine(line));
        String cmdLine = line + "\n";
        Serial2.print(cmdLine);
        // 如果是配网命令，更新状态并提取 SSID
        if (line.startsWith("WIFI|")) {
            int firstPipe = line.indexOf('|');
            int secondPipe = line.indexOf('|', firstPipe + 1);
            if (firstPipe > 0 && secondPipe > firstPipe) {
                hostWifiSsid = line.substring(firstPipe + 1, secondPipe);
            }
            hostWifiStatus = "connecting";
            hostWifiIp = "";
            hostWifiError = "";
            appendWebLog("serial2", String("HOST-WIFI: provisioning started ssid=") + hostWifiSsid);
        }
        String ack = String("ACK:SERIAL2_SENT");
        out.println(ack);
        appendWebLog("serial2", ack);
    } else {
        appendWebLog("web", String("> ") + redactWirelessConsoleLine(line));
        processWirelessConsoleLine(line, out, WIRELESS_ORIGIN_WEB);
        appendWebLogLines("web", response);
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

static void handleWifiWebJoystickCal()
{
    String response;
    StringPrint out(response);
    processWirelessConsoleLine("JOYSTICK_STATUS", out, WIRELESS_ORIGIN_WEB);
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "text/plain", response);
}

static void handleWifiWebJoystickCalSet()
{
    String action = wifiWebServer.arg("action");
    String cmd;
    if (action.equalsIgnoreCase("start")) cmd = "JOYSTICK_CAL";
    else if (action.equalsIgnoreCase("save")) cmd = "JOYSTICK_SAVE";
    else if (action.equalsIgnoreCase("retry")) cmd = "JOYSTICK_RETRY";
    else if (action.equalsIgnoreCase("abort")) cmd = "JOYSTICK_ABORT";
    else if (action.equalsIgnoreCase("reset")) cmd = "JOYSTICK_RESET";

    if (cmd.length() == 0) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(400, "text/plain", "NACK:UNKNOWN_ACTION");
        return;
    }

    String response;
    StringPrint out(response);
    processWirelessConsoleLine(cmd, out, WIRELESS_ORIGIN_WEB);
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "text/plain", response);
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
    appendJsonString(response, wifiApIpText().c_str());
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
    if (!ws.consoleAuthenticated && !ws.devModeEnabled && !isWirelessConsoleAuthDisabled()) {
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
    appendJsonString(response, wifiApIpText().c_str());
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

static void appendJudgeConfigJson(String& response, const JudgeConfig& config)
{
    response += "{\"collisionThreshold\":";
    response += String(config.collisionThreshold, 2);
    response += ",\"bigTurnThreshold\":";
    response += String(config.bigTurnThreshold, 2);
    response += ",\"windowSize\":";
    response += config.windowSize;
    response += ",\"collisionPenalty\":";
    response += String(config.collisionPenalty, 1);
    response += ",\"turnSmoothnessWeight\":";
    response += String(config.turnSmoothnessWeight, 1);
    response += ",\"rangeMatchWeight\":";
    response += String(config.rangeMatchWeight, 1);
    response += ",\"gyroStabilityWeight\":";
    response += String(config.gyroStabilityWeight, 1);
    response += ",\"bigTurnStabilityWeight\":";
    response += String(config.bigTurnStabilityWeight, 1);
    response += ",\"speedStabilityWeight\":";
    response += String(config.speedStabilityWeight, 1);
    response += ",\"throttleStabilityWeight\":";
    response += String(config.throttleStabilityWeight, 1);
    response += ",\"defaults\":{\"collisionThreshold\":";
    response += String(WIFI_JUDGE_COLLISION_THRESHOLD_DEFAULT, 2);
    response += ",\"bigTurnThreshold\":";
    response += String(WIFI_JUDGE_BIG_TURN_THRESHOLD_DEFAULT, 2);
    response += ",\"windowSize\":";
    response += WIFI_JUDGE_WINDOW_SIZE_DEFAULT;
    response += ",\"collisionPenalty\":";
    response += String(WIFI_JUDGE_COLLISION_PENALTY_DEFAULT, 1);
    response += ",\"turnSmoothnessWeight\":";
    response += String(WIFI_JUDGE_TURN_SMOOTHNESS_WEIGHT_DEFAULT, 1);
    response += ",\"rangeMatchWeight\":";
    response += String(WIFI_JUDGE_RANGE_MATCH_WEIGHT_DEFAULT, 1);
    response += ",\"gyroStabilityWeight\":";
    response += String(WIFI_JUDGE_GYRO_STABILITY_WEIGHT_DEFAULT, 1);
    response += ",\"bigTurnStabilityWeight\":";
    response += String(WIFI_JUDGE_BIG_TURN_STABILITY_WEIGHT_DEFAULT, 1);
    response += ",\"speedStabilityWeight\":";
    response += String(WIFI_JUDGE_SPEED_STABILITY_WEIGHT_DEFAULT, 1);
    response += ",\"throttleStabilityWeight\":";
    response += String(WIFI_JUDGE_THROTTLE_STABILITY_WEIGHT_DEFAULT, 1);
    response += "},\"limits\":{\"collisionThresholdMin\":";
    response += String(WIFI_JUDGE_COLLISION_THRESHOLD_MIN, 2);
    response += ",\"collisionThresholdMax\":";
    response += String(WIFI_JUDGE_COLLISION_THRESHOLD_MAX, 2);
    response += ",\"bigTurnThresholdMin\":";
    response += String(WIFI_JUDGE_BIG_TURN_THRESHOLD_MIN, 2);
    response += ",\"bigTurnThresholdMax\":";
    response += String(WIFI_JUDGE_BIG_TURN_THRESHOLD_MAX, 2);
    response += ",\"windowSizeMin\":";
    response += WIFI_JUDGE_WINDOW_SIZE_MIN;
    response += ",\"windowSizeMax\":";
    response += WIFI_JUDGE_WINDOW_SIZE_MAX;
    response += ",\"collisionPenaltyMin\":";
    response += String(WIFI_JUDGE_COLLISION_PENALTY_MIN, 1);
    response += ",\"collisionPenaltyMax\":";
    response += String(WIFI_JUDGE_COLLISION_PENALTY_MAX, 1);
    response += ",\"turnSmoothnessWeightMin\":";
    response += String(WIFI_JUDGE_TURN_SMOOTHNESS_WEIGHT_MIN, 1);
    response += ",\"turnSmoothnessWeightMax\":";
    response += String(WIFI_JUDGE_TURN_SMOOTHNESS_WEIGHT_MAX, 1);
    response += ",\"rangeMatchWeightMin\":";
    response += String(WIFI_JUDGE_RANGE_MATCH_WEIGHT_MIN, 1);
    response += ",\"rangeMatchWeightMax\":";
    response += String(WIFI_JUDGE_RANGE_MATCH_WEIGHT_MAX, 1);
    response += ",\"gyroStabilityWeightMin\":";
    response += String(WIFI_JUDGE_GYRO_STABILITY_WEIGHT_MIN, 1);
    response += ",\"gyroStabilityWeightMax\":";
    response += String(WIFI_JUDGE_GYRO_STABILITY_WEIGHT_MAX, 1);
    response += ",\"bigTurnStabilityWeightMin\":";
    response += String(WIFI_JUDGE_BIG_TURN_STABILITY_WEIGHT_MIN, 1);
    response += ",\"bigTurnStabilityWeightMax\":";
    response += String(WIFI_JUDGE_BIG_TURN_STABILITY_WEIGHT_MAX, 1);
    response += ",\"speedStabilityWeightMin\":";
    response += String(WIFI_JUDGE_SPEED_STABILITY_WEIGHT_MIN, 1);
    response += ",\"speedStabilityWeightMax\":";
    response += String(WIFI_JUDGE_SPEED_STABILITY_WEIGHT_MAX, 1);
    response += ",\"throttleStabilityWeightMin\":";
    response += String(WIFI_JUDGE_THROTTLE_STABILITY_WEIGHT_MIN, 1);
    response += ",\"throttleStabilityWeightMax\":";
    response += String(WIFI_JUDGE_THROTTLE_STABILITY_WEIGHT_MAX, 1);
    response += "}}";
}

static String wifiJudgeConfigJson()
{
    String response;
    response.reserve(256);
    appendJudgeConfigJson(response, ws.judgeConfig);
    return response;
}

static void handleWifiWebJudgeConfig()
{
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "application/json", wifiJudgeConfigJson());
}

static void handleWifiWebJudgeConfigSet()
{
    if (!wifiWebServer.hasArg("collisionThreshold") ||
        !wifiWebServer.hasArg("bigTurnThreshold") ||
        !wifiWebServer.hasArg("windowSize") ||
        !wifiWebServer.hasArg("collisionPenalty") ||
        !wifiWebServer.hasArg("turnSmoothnessWeight") ||
        !wifiWebServer.hasArg("rangeMatchWeight") ||
        !wifiWebServer.hasArg("gyroStabilityWeight") ||
        !wifiWebServer.hasArg("bigTurnStabilityWeight") ||
        !wifiWebServer.hasArg("speedStabilityWeight") ||
        !wifiWebServer.hasArg("throttleStabilityWeight")) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(400, "application/json", "{\"error\":\"missing_fields\"}");
        return;
    }

    JudgeConfig config = ws.judgeConfig;
    config.collisionThreshold = wifiWebServer.arg("collisionThreshold").toFloat();
    config.bigTurnThreshold = wifiWebServer.arg("bigTurnThreshold").toFloat();
    int windowSize = wifiWebServer.arg("windowSize").toInt();
    config.collisionPenalty = wifiWebServer.arg("collisionPenalty").toFloat();
    config.turnSmoothnessWeight = wifiWebServer.arg("turnSmoothnessWeight").toFloat();
    config.rangeMatchWeight = wifiWebServer.arg("rangeMatchWeight").toFloat();
    config.gyroStabilityWeight = wifiWebServer.arg("gyroStabilityWeight").toFloat();
    config.bigTurnStabilityWeight = wifiWebServer.arg("bigTurnStabilityWeight").toFloat();
    config.speedStabilityWeight = wifiWebServer.arg("speedStabilityWeight").toFloat();
    config.throttleStabilityWeight = wifiWebServer.arg("throttleStabilityWeight").toFloat();
    if (windowSize < WIFI_JUDGE_WINDOW_SIZE_MIN || windowSize > WIFI_JUDGE_WINDOW_SIZE_MAX) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_window_size\"}");
        return;
    }
    config.windowSize = (uint8_t)windowSize;
    if (!isValidJudgeConfig(config)) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_value\"}");
        return;
    }
    if (!saveJudgeConfigPreference(config)) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(500, "application/json", "{\"saved\":false}");
        return;
    }

    String response;
    response.reserve(280);
    response += "{\"saved\":true,\"config\":";
    appendJudgeConfigJson(response, ws.judgeConfig);
    response += "}";
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "application/json", response);
}

static void handleWifiWebJudgeConfigReset()
{
    if (!resetJudgeConfigPreference()) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(500, "application/json", "{\"reset\":false}");
        return;
    }

    String response;
    response.reserve(280);
    response += "{\"reset\":true,\"config\":";
    appendJudgeConfigJson(response, ws.judgeConfig);
    response += "}";
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "application/json", response);
}

// ---------------------------------------------------------------------------
// Web UI mute preference
//
// State and NVS persistence (namespace "webui", key "muted" UChar 0/1,
// default unmuted) live in mus4_core's MutePreference, so every firmware
// sound producer (the Buzzer) shares one gate; the preference is loaded
// early in setup() before Wi-Fi setup can play the AP start melody. Open
// endpoints like judge-config: the wireless command permission layering
// only applies to console commands.

static void handleWifiWebMuteGet()
{
    String response;
    response.reserve(16);
    response += "{\"muted\":";
    response += (isSystemMuted() ? '1' : '0');
    response += "}";
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "application/json", response);
}

static void handleWifiWebMuteSet()
{
    if (!wifiWebServer.hasArg("muted")) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_value\"}");
        return;
    }
    String mutedArg = wifiWebServer.arg("muted");
    if (mutedArg != "0" && mutedArg != "1") {
        sendWifiWebApiHeaders();
        wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_value\"}");
        return;
    }
    if (!saveMutePreference(mutedArg == "1")) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(500, "application/json", "{\"saved\":false}");
        return;
    }

    String response;
    response.reserve(28);
    response += "{\"saved\":true,\"muted\":";
    response += (isSystemMuted() ? '1' : '0');
    response += "}";
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "application/json", response);
}

// ---------------------------------------------------------------------------
// Web UI language preference
//
// Persisted in NVS namespace "webui", key "lang" (String "auto"/"zh"/"en"),
// default "auto" when the key is absent so a fresh device follows the browser
// language (web pages resolve "auto" via navigator.language). An explicit
// zh/en choice overrides auto-detection and survives reboots. Open endpoints
// like mute/judge-config: the wireless command permission layering only
// applies to console commands.

static bool isValidWebUiLang(const String& lang)
{
    return lang == "zh" || lang == "en" || lang == "auto";
}

static void loadWebUiLanguagePreference()
{
    Preferences prefs;
    if (!prefs.begin("webui", true)) {
        webUiLang = "auto";
        mus4LogLine("web", "lang pref load failed, default auto");
        return;
    }
    String lang = prefs.getString("lang", "auto");
    prefs.end();
    webUiLang = isValidWebUiLang(lang) ? lang : "auto";
}

static bool saveWebUiLanguagePreference(const String& lang)
{
    Preferences prefs;
    if (!prefs.begin("webui", false)) return false;
    size_t written = prefs.putString("lang", lang);
    prefs.end();
    if (written == 0) return false;
    webUiLang = lang;
    return true;
}

static void handleWifiWebLanguageGet()
{
    String response;
    response.reserve(16);
    response += "{\"lang\":\"";
    response += webUiLang;
    response += "\"}";
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "application/json", response);
}

static void handleWifiWebLanguageSet()
{
    if (!wifiWebServer.hasArg("lang")) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_value\"}");
        return;
    }
    String langArg = wifiWebServer.arg("lang");
    if (!isValidWebUiLang(langArg)) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_value\"}");
        return;
    }
    if (!saveWebUiLanguagePreference(langArg)) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(500, "application/json", "{\"saved\":false}");
        return;
    }

    String response;
    response.reserve(30);
    response += "{\"saved\":true,\"lang\":\"";
    response += webUiLang;
    response += "\"}";
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "application/json", response);
}

static void appendDriftConfigJson(String& response, const DriftConfig& config)
{
    response += "{\"steeringGyroSign\":";
    response += config.steeringGyroSign;
    response += ",\"maxYawRate\":";
    response += String(config.maxYawRate, 2);
    response += ",\"kp\":";
    response += String(config.kp, 3);
    response += ",\"kd\":";
    response += String(config.kd, 3);
    response += ",\"maxSteeringCorrection\":";
    response += String(config.maxSteeringCorrection, 2);
    response += ",\"gyroFilterAlpha\":";
    response += String(config.gyroFilterAlpha, 2);
    response += ",\"spinThreshold\":";
    response += String(config.spinThreshold, 2);
    response += ",\"steeringThreshold\":";
    response += String(config.steeringThreshold, 2);
    response += ",\"continuousThrottle\":";
    response += String(config.continuousThrottle, 2);
    response += ",\"pulseThrottle\":";
    response += String(config.pulseThrottle, 2);
    response += ",\"pulseFreqHz\":";
    response += String(config.pulseFreqHz, 2);
    response += ",\"pulseDuty\":";
    response += String(config.pulseDuty, 2);
    response += ",\"defaults\":{";
    response += "\"steeringGyroSign\":";
    response += WIFI_DRIFT_STEERING_GYRO_SIGN_DEFAULT;
    response += ",\"maxYawRate\":";
    response += String(WIFI_DRIFT_MAX_YAW_RATE_DEFAULT, 2);
    response += ",\"kp\":";
    response += String(WIFI_DRIFT_KP_DEFAULT, 3);
    response += ",\"kd\":";
    response += String(WIFI_DRIFT_KD_DEFAULT, 3);
    response += ",\"maxSteeringCorrection\":";
    response += String(WIFI_DRIFT_MAX_STEERING_CORRECTION_DEFAULT, 2);
    response += ",\"gyroFilterAlpha\":";
    response += String(WIFI_DRIFT_GYRO_FILTER_ALPHA_DEFAULT, 2);
    response += ",\"spinThreshold\":";
    response += String(WIFI_DRIFT_SPIN_THRESHOLD_DEFAULT, 2);
    response += ",\"steeringThreshold\":";
    response += String(WIFI_DRIFT_STEERING_THRESHOLD_DEFAULT, 2);
    response += ",\"continuousThrottle\":";
    response += String(WIFI_DRIFT_CONTINUOUS_THROTTLE_DEFAULT, 2);
    response += ",\"pulseThrottle\":";
    response += String(WIFI_DRIFT_PULSE_THROTTLE_DEFAULT, 2);
    response += ",\"pulseFreqHz\":";
    response += String(WIFI_DRIFT_PULSE_FREQ_HZ_DEFAULT, 2);
    response += ",\"pulseDuty\":";
    response += String(WIFI_DRIFT_PULSE_DUTY_DEFAULT, 2);
    response += "},\"limits\":{\"steeringGyroSignMin\":";
    response += WIFI_DRIFT_STEERING_GYRO_SIGN_MIN;
    response += ",\"steeringGyroSignMax\":";
    response += WIFI_DRIFT_STEERING_GYRO_SIGN_MAX;
    response += ",\"maxYawRateMin\":";
    response += String(WIFI_DRIFT_MAX_YAW_RATE_MIN, 2);
    response += ",\"maxYawRateMax\":";
    response += String(WIFI_DRIFT_MAX_YAW_RATE_MAX, 2);
    response += ",\"kpMin\":";
    response += String(WIFI_DRIFT_KP_MIN, 3);
    response += ",\"kpMax\":";
    response += String(WIFI_DRIFT_KP_MAX, 3);
    response += ",\"kdMin\":";
    response += String(WIFI_DRIFT_KD_MIN, 3);
    response += ",\"kdMax\":";
    response += String(WIFI_DRIFT_KD_MAX, 3);
    response += ",\"maxSteeringCorrectionMin\":";
    response += String(WIFI_DRIFT_MAX_STEERING_CORRECTION_MIN, 2);
    response += ",\"maxSteeringCorrectionMax\":";
    response += String(WIFI_DRIFT_MAX_STEERING_CORRECTION_MAX, 2);
    response += ",\"gyroFilterAlphaMin\":";
    response += String(WIFI_DRIFT_GYRO_FILTER_ALPHA_MIN, 2);
    response += ",\"gyroFilterAlphaMax\":";
    response += String(WIFI_DRIFT_GYRO_FILTER_ALPHA_MAX, 2);
    response += ",\"spinThresholdMin\":";
    response += String(WIFI_DRIFT_SPIN_THRESHOLD_MIN, 2);
    response += ",\"spinThresholdMax\":";
    response += String(WIFI_DRIFT_SPIN_THRESHOLD_MAX, 2);
    response += ",\"steeringThresholdMin\":";
    response += String(WIFI_DRIFT_STEERING_THRESHOLD_MIN, 2);
    response += ",\"steeringThresholdMax\":";
    response += String(WIFI_DRIFT_STEERING_THRESHOLD_MAX, 2);
    response += ",\"continuousThrottleMin\":";
    response += String(WIFI_DRIFT_CONTINUOUS_THROTTLE_MIN, 2);
    response += ",\"continuousThrottleMax\":";
    response += String(WIFI_DRIFT_CONTINUOUS_THROTTLE_MAX, 2);
    response += ",\"pulseThrottleMin\":";
    response += String(WIFI_DRIFT_PULSE_THROTTLE_MIN, 2);
    response += ",\"pulseThrottleMax\":";
    response += String(WIFI_DRIFT_PULSE_THROTTLE_MAX, 2);
    response += ",\"pulseFreqHzMin\":";
    response += String(WIFI_DRIFT_PULSE_FREQ_HZ_MIN, 2);
    response += ",\"pulseFreqHzMax\":";
    response += String(WIFI_DRIFT_PULSE_FREQ_HZ_MAX, 2);
    response += ",\"pulseDutyMin\":";
    response += String(WIFI_DRIFT_PULSE_DUTY_MIN, 2);
    response += ",\"pulseDutyMax\":";
    response += String(WIFI_DRIFT_PULSE_DUTY_MAX, 2);
    response += "}}";
}

static String wifiDriftConfigJson()
{
    String response;
    response.reserve(512);
    appendDriftConfigJson(response, ws.driftConfig);
    return response;
}

static void handleWifiWebDriftConfig()
{
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "application/json", wifiDriftConfigJson());
}

static void handleWifiWebDriftConfigSet()
{
    if (!wifiWebServer.hasArg("steeringGyroSign") ||
        !wifiWebServer.hasArg("maxYawRate") ||
        !wifiWebServer.hasArg("kp") ||
        !wifiWebServer.hasArg("kd") ||
        !wifiWebServer.hasArg("maxSteeringCorrection") ||
        !wifiWebServer.hasArg("gyroFilterAlpha") ||
        !wifiWebServer.hasArg("spinThreshold") ||
        !wifiWebServer.hasArg("steeringThreshold") ||
        !wifiWebServer.hasArg("continuousThrottle") ||
        !wifiWebServer.hasArg("pulseThrottle") ||
        !wifiWebServer.hasArg("pulseFreqHz") ||
        !wifiWebServer.hasArg("pulseDuty")) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(400, "application/json", "{\"error\":\"missing_fields\"}");
        return;
    }

    DriftConfig config = ws.driftConfig;
    int sign = wifiWebServer.arg("steeringGyroSign").toInt();
    config.steeringGyroSign = (int8_t)sign;
    config.maxYawRate = wifiWebServer.arg("maxYawRate").toFloat();
    config.kp = wifiWebServer.arg("kp").toFloat();
    config.kd = wifiWebServer.arg("kd").toFloat();
    config.maxSteeringCorrection = wifiWebServer.arg("maxSteeringCorrection").toFloat();
    config.gyroFilterAlpha = wifiWebServer.arg("gyroFilterAlpha").toFloat();
    config.spinThreshold = wifiWebServer.arg("spinThreshold").toFloat();
    config.steeringThreshold = wifiWebServer.arg("steeringThreshold").toFloat();
    config.continuousThrottle = wifiWebServer.arg("continuousThrottle").toFloat();
    config.pulseThrottle = wifiWebServer.arg("pulseThrottle").toFloat();
    config.pulseFreqHz = wifiWebServer.arg("pulseFreqHz").toFloat();
    config.pulseDuty = wifiWebServer.arg("pulseDuty").toFloat();
    if (!isValidDriftConfig(config)) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_value\"}");
        return;
    }
    if (!saveDriftConfigPreference(config)) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(500, "application/json", "{\"saved\":false}");
        return;
    }
    // 使新配置立即在控制循环中生效
    load_drift_config(ws.driftConfig);

    String response;
    response.reserve(520);
    response += "{\"saved\":true,\"config\":";
    appendDriftConfigJson(response, ws.driftConfig);
    response += "}";
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "application/json", response);
}

static void handleWifiWebDriftConfigReset()
{
    if (!resetDriftConfigPreference()) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(500, "application/json", "{\"reset\":false}");
        return;
    }
    load_drift_config(ws.driftConfig);

    String response;
    response.reserve(520);
    response += "{\"reset\":true,\"config\":";
    appendDriftConfigJson(response, ws.driftConfig);
    response += "}";
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "application/json", response);
}

static void handleWifiWebStaPassword()
{
    if (!ws.consoleAuthenticated && !ws.devModeEnabled && !isWirelessConsoleAuthDisabled()) {
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

static bool isWifiWebRequestFromAp(const String& sourceArg)
{
    if (sourceArg == "ap") return true;
    IPAddress apIp = WiFi.softAPIP();
    if (apIp == IPAddress(0, 0, 0, 0)) return false;
    if (WiFi.softAPgetStationNum() > 0) return true;
    if (!ws.staConnected) return true;
    WiFiClient client = wifiWebServer.client();
    if (client && client.localIP() == apIp) return true;
    String host = wifiWebServer.hostHeader();
    return host.indexOf(apIp.toString()) >= 0;
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
    if (!ws.consoleAuthenticated && !ws.devModeEnabled && !isWirelessConsoleAuthDisabled()) {
        wifiWebServer.send(403, "application/json", "{\"error\":\"auth_required\"}");
        return;
    }
    String ssid = wifiWebServer.arg("ssid");
    String password = wifiWebServer.arg("password");
    String sourceArg = wifiWebServer.arg("source");
    bool requestFromAp = isWifiWebRequestFromAp(sourceArg);
    int targetChannel = wifiWebServer.arg("channel").toInt();
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
    wifiStaApplyFromAp = requestFromAp;
    wifiStaTargetChannel = (requestFromAp && targetChannel >= 1 && targetChannel <= 14) ? (uint8_t)targetChannel : 0;
    appendWebLog("web", String("wifi sta saved ssid=") + ws.staSsid + " password=<redacted>");
    wifiWebServer.send(200, "application/json", String("{\"saved\":true,\"applied\":true,\"state\":") + wifiStaJson() + "}");
    scheduleWifiStaApply();
}

static void handleWifiWebStaClear()
{
    if (!ws.consoleAuthenticated && !ws.devModeEnabled && !isWirelessConsoleAuthDisabled()) {
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

// STA connection history is read-only and public (same visibility as the
// scan list); passwords are never emitted, only a "password_set" flag.
static void handleWifiWebStaHistory()
{
    String response;
    response.reserve(256);
    response += "{\"count\":";
    response += wifiStaHistoryCount();
    response += ",\"entries\":[";
    bool firstEntry = true;
    for (uint8_t slot = 0; slot < WIFI_STA_HISTORY_SIZE; slot++) {
        String ssid;
        if (!copyWifiStaHistorySsid(slot, ssid)) continue;
        String password;
        findWifiStaHistoryEntry(ssid, password);
        if (!firstEntry) response += ',';
        firstEntry = false;
        response += "{\"rank\":";
        response += slot + 1;
        response += ",\"ssid\":";
        appendJsonString(response, ssid.c_str());
        response += ",\"password_set\":";
        response += password.length() > 0 ? "true" : "false";
        response += '}';
    }
    response += "]}";
    wifiWebServer.send(200, "application/json", response);
}

static void handleWifiWebStaHistoryDelete()
{
    if (!ws.consoleAuthenticated && !ws.devModeEnabled && !isWirelessConsoleAuthDisabled()) {
        wifiWebServer.send(403, "application/json", "{\"error\":\"auth_required\"}");
        return;
    }
    String ssid = wifiWebServer.arg("ssid");
    ssid.trim();
    // Only the history record is removed: the configured credentials and the
    // live STA connection (ws.staSsid/staPassword) are intentionally kept,
    // so deleting the currently connected WiFi does not drop the link.
    if (ssid.length() == 0 || !removeWifiStaHistoryEntry(ssid)) {
        wifiWebServer.send(404, "application/json", "{\"error\":\"not_found\"}");
        return;
    }
    mus4Logf("web", "wifi sta history deleted ssid=%s", ssid.c_str());
    String response;
    response.reserve(48);
    response += "{\"deleted\":true,\"count\":";
    response += wifiStaHistoryCount();
    response += "}";
    wifiWebServer.send(200, "application/json", response);
}

// 上位机配网状态查询（通过 Serial2 发送 WIFI|ssid|password 给 Linux 上位机后，
// Linux 上位机会回复 STATUS|CONNECTING / OK|<ip> / FAIL|<reason>）。
static void handleWifiWebHostWifiStatus()
{
    sendWifiWebApiHeaders();
    String json;
    json.reserve(224);
    json += "{\"status\":";
    appendJsonString(json, hostWifiStatus.c_str());
    json += ",\"ssid\":";
    appendJsonString(json, hostWifiSsid.c_str());
    json += ",\"ip\":";
    appendJsonString(json, hostWifiIp.c_str());
    json += ",\"error\":";
    appendJsonString(json, hostWifiError.c_str());
    json += ",\"host_ip\":";
    appendJsonString(json, hostReportedIp.c_str());
    json += ",\"host_ip_age_s\":";
    json += String(hostReportedIpMs ? (millis() - hostReportedIpMs) / 1000UL : 0UL);
    json += "}";
    wifiWebServer.send(200, "application/json", json);
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
    response += ",\"gx\":";
    response += String(point.gyroX, 3);
    response += ",\"gy\":";
    response += String(point.gyroY, 3);
    response += ",\"ax\":";
    response += String(point.accelX, 3);
    response += ",\"ay\":";
    response += String(point.accelY, 3);
    response += ",\"az\":";
    response += String(point.accelZ, 3);
    response += ",\"de\":";
    response += point.driftEnabled ? 1 : 0;
    response += ",\"da\":";
    response += point.driftActive ? 1 : 0;
    response += ",\"dc\":";
    response += String(point.driftCompensation, 2);
    response += ",\"gzf\":";
    response += String(point.gyroZFiltered, 3);
    response += ",\"dye\":";
    response += String(point.driftYawError, 3);
    response += ",\"dsc\":";
    response += String(point.driftSteeringCorrection, 2);
    response += ",\"dtm\":";
    response += point.driftThrottleMode;
    response += ",\"pseudoSpeed\":";
    response += String(point.pseudoSpeed, 1);
    response += ",\"sd\":";
    response += point.actuatorSteeringDuty;
    response += ",\"ed\":";
    response += point.actuatorThrottleDuty;
    response += ",\"sm\":";
    response += servo_mid_v;
    response += ",\"mm\":";
    response += motor_mid_v;
    response += ",\"tl\":";
    response += joystick_cal.throttle_min_duty;
    response += ",\"tu\":";
    response += joystick_cal.throttle_max_duty;
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
    wifiWebServer.sendHeader("Cache-Control", "no-store");
    wifiWebServer.send_P(200, "text/html", WIFI_WEB_UPDATE_HTML);
}

static bool isWifiWebUpdateAuthOk()
{
    if (isWirelessConsoleAuthDisabled()) return true;
    if (ws.devModeEnabled) return true;
    if (ws.consoleAuthenticated) return true;
    // Allow one-shot auth via query parameter for scripted uploads
    if (wifiWebServer.hasArg("auth") && wifiWebServer.arg("auth").equals(WIFI_CONSOLE_AP_PASSWORD)) {
        return true;
    }
    return false;
}

static void resetOtaAfterFailedUpload()
{
    // 确保 Update 对象不会卡在 running 状态，否则后续 Update.begin() 会报
    // "already running" 而彻底拒绝新的 OTA 请求。
    if (Update.isRunning()) {
        Update.abort();
    }
    os.inProgress = false;
    os.parkGuardActive = false;
    os.closeWsPending = false;
    os.lastProgressPct = 0;
    if (ws.devModeEnabled) {
        // DEV 模式下保持窗口 open，方便继续调试；重设 TTL 避免立即超时。
        os.windowOpen = true;
        os.deadlineMs = millis() + WIFI_OTA_WINDOW_MS;
    } else {
        os.windowOpen = false;
        os.deadlineMs = 0;
    }
}

static void handleWifiWebUpdateUpload()
{
    HTTPUpload& upload = wifiWebServer.upload();
    if (upload.status == UPLOAD_FILE_START) {
        wifiWebUpdateErrorMsg = "";
        wifiWebUpdateReceived = 0;
        // 防御：如果上次更新异常退出导致 Update 对象仍处 running 状态，
        // 先 abort 再 begin，避免 "already running" 导致新上传无法开始。
        if (Update.isRunning()) {
            Update.abort();
        }
        if (!isWifiWebUpdateAuthOk()) {
            wifiWebUpdateErrorMsg = "NACK:AUTH_REQUIRED";
            mus4LogLine("ota", "http update rejected: auth required");
            return;
        }
        // v1.7.34：HTTP OTA 上传开始时若 Park 未锁定，自动强制锁定而非拒绝。
        // OTA 传输期间本就会通过 forceWifiOtaParkLocked() 强制 Park Locked，
        // 前置检查反而导致开发模式下仍需手动按遥控器锁定，与 "OTA 传输期间
        // 自动 Park Locked" 的文档描述不一致。
        os.parkGuardActive = true;
        forceWifiOtaParkLocked();
        os.inProgress = true;
        os.windowOpen = true;
        os.closeWsPending = true;
        os.lastProgressPct = 0;
        startLedOtaGlitch(); // 传输期间状态灯随机乱闪（故障灯效）
        // v1.7.27：同步 WebServer 默认 read timeout 仅 5000ms，OTA 期间 Flash
        // erase/write 可能让 TCP 接收窗口长时间为 0，触发 read timeout 导致
        // "http update aborted"。上传开始时把客户端 timeout 提高到 30s，给
        // Flash 写入和 TCP 流控恢复留出足够余量。
        wifiWebServer.client().setTimeout(30000);
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
            scanLedOtaGlitch(); // 每写一块推进一步故障灯效（内部按随机间隔门控）
            // v1.7.26：每收到一块 OTA 数据后让出 CPU，避免长时间连续写 Flash
            // 阻塞 Wi-Fi/AsyncTCP task 触发 Task WDT，导致上传中途断连。
            // 使用 yield() 而非 delay(n)，既能让其他任务（包括 idle/WDT）获得时间片，
            // 又不会人为拖慢 HTTP 上传节奏。
            yield();
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        stopLedOtaGlitch(); // 传输结束（无论成败）归还状态灯给正常状态机
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
        // abort 路径由 handleWifiWebUpdatePost 统一清理 OTA 状态；
        // 这里只标记错误并打日志，避免状态重置分散在两处。
        stopLedOtaGlitch();
        wifiWebUpdateErrorMsg = "NACK:ABORTED";
        mus4LogLine("ota", "http update aborted");
    }
}

static void handleWifiWebUpdatePost()
{
    unsigned long startedMs = millis();
    sendWifiWebApiHeaders();
    if (wifiWebUpdateErrorMsg.length() > 0) {
        // v1.7.27：上传失败后必须完整清理 OTA 状态，否则 os.inProgress /
        // parkGuardActive / windowOpen 会长期卡住，导致后续 OTA（包括
        // Reset 前的多次尝试）行为异常；Update 对象若仍处 running 状态，
        // 下一次 Update.begin() 会直接失败。
        resetOtaAfterFailedUpload();
        wifiWebServer.send(500, "text/plain", wifiWebUpdateErrorMsg + "\n");
        recordWifiWebHandlerDt(startedMs, wifiWebHttpMaxDtMs);
        return;
    }
    wifiWebServer.send(200, "text/plain", "ACK:UPDATE_OK\n");
    recordWifiWebHandlerDt(startedMs, wifiWebHttpMaxDtMs);
    // 故障灯效延续到重启后：setup() 取标记重新启动乱闪，直到开机蜂鸣器播完
    markLedOtaGlitchAfterReboot();
    delay(100);
    ESP.restart();
}

void setupWebConsoleServer()
{
    // v1.7.28：OTA 上传期间，浏览器轮询 /api/status、/api/log、/api/data 等会
    // 占用同步 WebServer 的单客户端处理能力和 LWIP TCP socket。通过 middleware
    // 在 inProgress 期间对非 /update 请求快速返回 503，让浏览器立即释放连接，
    // 避免 OTA 大文件传输被并发 HTTP 请求挤占资源导致连接被 reset。
    wifiWebServer.addMiddleware([](WebServer &server, Middleware::Callback next) -> bool {
        if (otaRuntime.inProgress && server.uri() != "/update") {
            server.send(503, "text/plain", "OTA in progress\n");
            return false;
        }
        return next();
    });
    wifiWebServer.on("/", HTTP_GET, handleWifiWebRoot);
    wifiWebServer.on("/judge", HTTP_GET, handleWifiWebJudge);
    wifiWebServer.on("/drift", HTTP_GET, handleWifiWebDrift);
    wifiWebServer.on("/favicon.png", HTTP_GET, handleWifiWebFavicon);
    wifiWebServer.on("/favicon.ico", HTTP_GET, handleWifiWebFavicon);
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
    wifiWebServer.on("/api/joystick-cal", HTTP_GET, handleWifiWebJoystickCal);
    wifiWebServer.on("/api/joystick-cal", HTTP_POST, handleWifiWebJoystickCalSet);
    wifiWebServer.on("/api/wifi-ap", HTTP_GET, handleWifiWebAp);
    wifiWebServer.on("/api/wifi-ap", HTTP_POST, handleWifiWebApSet);
    wifiWebServer.on("/api/wifi-sta", HTTP_GET, handleWifiWebSta);
    wifiWebServer.on("/api/wifi-sta", HTTP_POST, handleWifiWebStaSet);
    wifiWebServer.on("/api/wifi-sta/password", HTTP_GET, handleWifiWebStaPassword);
    wifiWebServer.on("/api/wifi-sta/scan", HTTP_GET, handleWifiWebStaScan);
    wifiWebServer.on("/api/wifi-sta/clear", HTTP_POST, handleWifiWebStaClear);
    wifiWebServer.on("/api/wifi-sta/history", HTTP_GET, handleWifiWebStaHistory);
    wifiWebServer.on("/api/wifi-sta/history/delete", HTTP_POST, handleWifiWebStaHistoryDelete);
    wifiWebServer.on("/api/host-wifi-status", HTTP_GET, handleWifiWebHostWifiStatus);
    wifiWebServer.on("/api/judge-config", HTTP_GET, handleWifiWebJudgeConfig);
    wifiWebServer.on("/api/judge-config", HTTP_POST, handleWifiWebJudgeConfigSet);
    wifiWebServer.on("/api/judge-config/reset", HTTP_POST, handleWifiWebJudgeConfigReset);
    wifiWebServer.on("/api/mute", HTTP_GET, handleWifiWebMuteGet);
    wifiWebServer.on("/api/mute", HTTP_POST, handleWifiWebMuteSet);
    wifiWebServer.on("/api/language", HTTP_GET, handleWifiWebLanguageGet);
    wifiWebServer.on("/api/language", HTTP_POST, handleWifiWebLanguageSet);
    wifiWebServer.on("/api/drift-config", HTTP_GET, handleWifiWebDriftConfig);
    wifiWebServer.on("/api/drift-config", HTTP_POST, handleWifiWebDriftConfigSet);
    wifiWebServer.on("/api/drift-config/reset", HTTP_POST, handleWifiWebDriftConfigReset);
    wifiWebServer.on("/api/log", HTTP_GET, handleWifiWebLog);
    wifiWebServer.on("/api/data", HTTP_GET, handleWifiWebData);
    wifiWebServer.on("/update", HTTP_GET, handleWifiWebUpdateGet);
    wifiWebServer.on("/update", HTTP_POST, handleWifiWebUpdatePost, handleWifiWebUpdateUpload);
    wifiWebServer.onNotFound(handleWifiWebCaptivePortalNotFound);
    // Load the persisted Web UI language preference once at server init (default: auto)
    loadWebUiLanguagePreference();
    wifiWebServer.begin();
}

void updateWebConsoleServer()
{
    // v1.7.18 起 AP/STA 互斥切换：STA-only 状态下 wifiConsoleStarted 会被
    // stopWifiApForStaOnly() 主动置 false（含义聚焦到「AP 服务是否就绪」），
    // 但 wifiWebServer 仍在 STA 接口监听，HTTP 必须继续被驱动；否则浏览器
    // 通过 STA IP 访问会被 TCP RST。Web Console 的驱动门槛改为「AP 或 STA
    // 有一个就绪」，与 AP 入口生命周期解耦。
    if (!ws.consoleStarted && !ws.staConnected) return;
    unsigned long now = millis();
    if (lastWifiWebUpdateMs != 0) {
        uint32_t dt = (uint32_t)(now - lastWifiWebUpdateMs);
        if (dt > wifiWebUpdateMaxDtMs) wifiWebUpdateMaxDtMs = dt;
    }
    lastWifiWebUpdateMs = now;
    // OTA 上传期间主循环会被同步 WebServer 的 /update handler 长时间占用，
    // 继续采样遥测数据没有意义，还会增加堆分配和主循环开销；跳过采样，
    // 把 CPU 时间片尽量留给 handleClient 处理 TCP ACK 与 Flash 写入间隙。
    unsigned long stageStart = millis();
    uint32_t stageDt = 0;
    if (!otaRuntime.inProgress) {
        stageStart = millis();
        sampleWifiWebData();
        stageDt = (uint32_t)(millis() - stageStart);
        if (stageDt > wifiWebSampleMaxDtMs) wifiWebSampleMaxDtMs = stageDt;
    }
    // 配网响应（STATUS|/OK|/FAIL|）由 MUS4_FW.ino 的 handleSerial2() 统一解析并
    // 更新 hostWifiStatus/hostWifiIp/hostWifiError。此处不再直接读 Serial2，
    // 避免与 handleSerial2() 的逐字节状态机形成双消费者竞争，也避免
    // readStringUntil 在半行数据上阻塞拖慢 Web 主循环。
    stageStart = millis();
    wifiWebServer.handleClient();
    stageDt = (uint32_t)(millis() - stageStart);
    if (stageDt > wifiWebHttpMaxDtMs) wifiWebHttpMaxDtMs = stageDt;
}

#endif
