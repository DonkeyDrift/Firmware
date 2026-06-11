#include "WirelessConsole.h"

#include "SharedTypes.h"

#ifdef ENABLE_WIFI_CONSOLE
#include "BuildInfo.h"
#include "CommandDispatcher.h"
#include "JsonUtil.h"
#include "Mus4Log.h"
#include "WebLogBuffer.h"
#include "WifiIdentity.h"
#include "WifiOta.h"
#include "WifiStaConfig.h"

#include <WiFi.h>
#include <ESPmDNS.h>

extern ControlData car_output;
extern ControlData pilot_data;
extern ControlData rc_data;
extern SensorData ina219Data;
extern SensorData mpu6050Data;
extern SerialBuf wifiConsoleBuf;
extern WifiRuntimeState wifiRuntime;
extern OtaRuntimeState otaRuntime;

extern bool drift_assist_enabled;
extern bool drift_assist_active;
extern float drift_compensation;
extern float gyro_z_filtered;
extern int pwm_filtered[RC_CHANNEL_COUNT];

extern uint32_t wifiWebUpdateMaxDtMs;
extern uint32_t wifiWebSampleMaxDtMs;
extern uint32_t wifiWebHttpMaxDtMs;
extern uint32_t wifiWebSocketMaxDtMs;
extern uint32_t wifiWebStatusRequests;
extern uint32_t wifiWebLogRequests;
extern uint32_t wifiWebDataRequests;
extern uint32_t wifiWebCommandRequests;
extern uint32_t wifiWebStatusMaxDtMs;
extern uint32_t wifiWebLogMaxDtMs;
extern uint32_t wifiWebDataMaxDtMs;
extern uint32_t wifiWebCommandMaxDtMs;

#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
extern bool wifiWebSocketClientConnected;
extern uint32_t wifiWebSocketDroppedPoints;
extern uint32_t wifiWebSocketQueueFullSkips;
extern uint32_t wifiWebSocketHeapSkips;
extern uint32_t wifiWebSocketFramesSent;
extern uint32_t wifiWebSocketMaxBacklog;
extern uint32_t wifiWebSocketConnects;
extern uint32_t wifiWebSocketDisconnects;
#endif

String redactWirelessConsoleLine(const String& line)
{
    if (line.startsWith("AUTH:")) return "AUTH:<redacted>";
    if (line.startsWith("WIFI_STA_PASSWORD:")) return "WIFI_STA_PASSWORD:<redacted>";
    return line;
}

bool isWirelessControlCommand(const String& line)
{
    int firstColon = line.indexOf(':');
    if (firstColon <= 0) return false;
    String throttleText = line.substring(0, firstColon);
    int secondColon = line.indexOf(':', firstColon + 1);
    int star = line.indexOf('*', firstColon + 1);
    int end = line.length();
    if (secondColon > firstColon) end = secondColon;
    if (star > firstColon && star < end) end = star;
    String steeringText = line.substring(firstColon + 1, end);
    throttleText.trim();
    steeringText.trim();
    if (throttleText.length() == 0 || steeringText.length() == 0) return false;
    for (uint16_t i = 0; i < throttleText.length(); i++) {
        char c = throttleText.charAt(i);
        if (!(isDigit(c) || (i == 0 && c == '-'))) return false;
    }
    for (uint16_t i = 0; i < steeringText.length(); i++) {
        char c = steeringText.charAt(i);
        if (!(isDigit(c) || (i == 0 && c == '-'))) return false;
    }
    return true;
}

bool isWirelessOtaOpenCommand(const String& line)
{
    return line.equalsIgnoreCase("ENABLE_OTA");
}

bool isLocalOtaOpenCommand(const String& line)
{
    return line.startsWith("ENABLE_OTA:");
}

bool isWirelessOtaStatusCommand(const String& line)
{
    return line.equalsIgnoreCase("OTA_STATUS");
}

bool isWirelessOtaCloseCommand(const String& line)
{
    return line.equalsIgnoreCase("DISABLE_OTA");
}

bool isWifiStaConfigCommand(const String& line)
{
    return line.startsWith("WIFI_STA_SSID:") ||
        line.startsWith("WIFI_STA_PASSWORD:") ||
        line.equalsIgnoreCase("WIFI_STA_APPLY") ||
        line.equalsIgnoreCase("WIFI_STA_CLEAR");
}

bool isParkLockedWirelessCommand(const String& line)
{
    return isWirelessOtaOpenCommand(line) ||
        line.equalsIgnoreCase("STEER_CAL") ||
        line.equalsIgnoreCase("CAL_SAVE") ||
        line.equalsIgnoreCase("CAL_RETRY") ||
        line.equalsIgnoreCase("CAL_ABORT") ||
        line.equalsIgnoreCase("CAL_RESET") ||
        line.equalsIgnoreCase("CAL_STATUS") ||
        line.equalsIgnoreCase("TEST") ||
        line.equalsIgnoreCase("TEST_TUI") ||
        line.equalsIgnoreCase("BENCH") ||
        line.equalsIgnoreCase("STRESS") ||
        line.equalsIgnoreCase("REGRESS") ||
        line.equalsIgnoreCase("FILTER_TEST");
}

bool isWirelessCommandAllowed(const String& line, WirelessCommandOrigin origin, WifiRuntimeState& ws)
{
    bool webDevMode = ws.devModeEnabled && origin == WIRELESS_ORIGIN_WEB;
    if (line.equalsIgnoreCase("PING") || line.equalsIgnoreCase("STATUS") || line.equalsIgnoreCase("WIFI_STA_STATUS")) return true;
    if (line.startsWith("AUTH:")) return true;
    if (isWirelessOtaOpenCommand(line)) return (webDevMode || ws.consoleAuthenticated) && car_output.park == PARK_LOCKED;
    if (isWirelessOtaStatusCommand(line) || isWirelessOtaCloseCommand(line)) return webDevMode || ws.consoleAuthenticated;
    if (!ws.consoleAuthenticated && !webDevMode) return false;
    if (isParkLockedWirelessCommand(line)) return car_output.park == PARK_LOCKED;
    if (line.equalsIgnoreCase("ANSI") || line.equalsIgnoreCase("NOANSI") || line.equalsIgnoreCase("FILTER_DEBUG") || line.equalsIgnoreCase("LOG_WEB") || line.equalsIgnoreCase("LOG_SERIAL") || isWifiStaConfigCommand(line)) return true;
    return isWirelessControlCommand(line);
}

void printWirelessStatus(Print& out)
{
    out.printf("STATUS mode=%d park=%d throttle=%d steering=%d wifi_frames=%lu wifi_errors=%lu ota_window=%d ota_progress=%u ota_ttl_ms=%lu dev_mode=%d park_guard=%d version=%s build=\"%s %s\" web_port=%u free_heap=%lu min_free_heap=%lu ws_port=%u ws_client=%d ws_dropped=%lu ws_queue_full_skip=%lu ws_heap_skip=%lu ws_frames=%lu ws_max_backlog=%lu ws_connects=%lu ws_disconnects=%lu web_update_dt_max=%lu web_sample_dt_max=%lu web_http_dt_max=%lu web_ws_dt_max=%lu http_status_count=%lu http_log_count=%lu http_data_count=%lu http_cmd_count=%lu http_status_dt_max=%lu http_log_dt_max=%lu http_data_dt_max=%lu http_cmd_dt_max=%lu ap_ssid=\"%s\" ap_ip=%s ap_clients=%u sta_configured=%d sta_connected=%d sta_ssid=\"%s\" sta_ip=%s mdns_host=\"%s\" mdns_url=%s mdns_started=%d\n",
        car_output.mode,
        car_output.park ? 1 : 0,
        car_output.throttle,
        car_output.steering,
        wifiConsoleBuf.frames,
        wifiConsoleBuf.errors,
        otaRuntime.windowOpen ? 1 : 0,
        otaRuntime.lastProgressPct,
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
        wifiRuntime.apSsid,
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

void processWirelessConsoleLine(const String& line, Print& out, WirelessCommandOrigin origin)
{
    if (line.equalsIgnoreCase("PING")) {
        out.println("PONG");
        return;
    }
    if (line.equalsIgnoreCase("STATUS")) {
        printWirelessStatus(out);
        return;
    }
    if (line.startsWith("AUTH:")) {
        wifiRuntime.consoleAuthenticated = line.substring(5).equals(WIFI_CONSOLE_AP_PASSWORD);
        out.println(wifiRuntime.consoleAuthenticated ? "AUTH_OK" : "AUTH_FAIL");
        return;
    }
    if (!isWirelessCommandAllowed(line, origin, wifiRuntime)) {
        bool webDevMode = wifiRuntime.devModeEnabled && origin == WIRELESS_ORIGIN_WEB;
        if (isParkLockedWirelessCommand(line) && car_output.park != PARK_LOCKED && (wifiRuntime.consoleAuthenticated || webDevMode)) {
            out.println("NACK:PARK_REQUIRED");
        } else {
            out.println("NACK:UNAUTHORIZED");
        }
        wifiConsoleBuf.errors++;
        return;
    }
    if (isWirelessOtaOpenCommand(line)) {
        openWifiOtaWindow(out, origin, otaRuntime, wifiRuntime);
        return;
    }
    if (isWirelessOtaStatusCommand(line)) {
        printWifiOtaStatus(out, otaRuntime, wifiRuntime);
        return;
    }
    if (isWirelessOtaCloseCommand(line)) {
        closeWifiOtaWindow("USER", otaRuntime);
        out.println("OTA_CLOSED");
        return;
    }
    if (processWifiStaConfigCommand(line, out, wifiRuntime)) {
        return;
    }
    dispatchCommandLine(line, out, wifiConsoleBuf);
}
#endif
