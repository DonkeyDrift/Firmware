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
        line.equalsIgnoreCase("JOYSTICK_CAL") ||
        line.equalsIgnoreCase("JOYSTICK_SAVE") ||
        line.equalsIgnoreCase("JOYSTICK_RETRY") ||
        line.equalsIgnoreCase("JOYSTICK_ABORT") ||
        line.equalsIgnoreCase("JOYSTICK_RESET") ||
        line.equalsIgnoreCase("TEST") ||
        line.equalsIgnoreCase("TEST_TUI") ||
        line.equalsIgnoreCase("BENCH") ||
        line.equalsIgnoreCase("STRESS") ||
        line.equalsIgnoreCase("REGRESS") ||
        line.equalsIgnoreCase("FILTER_TEST");
}

bool isWirelessCommandAllowed(const String& line, WirelessCommandOrigin origin, WifiRuntimeState& ws)
{
    // DEV ON 仅对 Web 来源放权 "OTA + Web 配置 + 显示/日志切换 + WIFI_STA_*"；
    // 控制命令与 Park 锁定诊断命令始终要求认证。详见
    // docs/Plan/DEV模式影响面与运行逻辑映射.md §3。
    bool webDevMode = ws.devModeEnabled && origin == WIRELESS_ORIGIN_WEB;
    if (line.equalsIgnoreCase("PING") || line.equalsIgnoreCase("STATUS") || line.equalsIgnoreCase("WIFI_STA_STATUS")) return true;
    if (line.startsWith("AUTH:")) return true;
    if (isWirelessOtaOpenCommand(line)) return (webDevMode || ws.consoleAuthenticated) && car_output.park == PARK_LOCKED;
    if (isWirelessOtaStatusCommand(line) || isWirelessOtaCloseCommand(line)) return webDevMode || ws.consoleAuthenticated;
    // DEV ON 显式白名单：显示/日志切换、Wi-Fi STA 配置类命令。
    if (line.equalsIgnoreCase("ANSI") || line.equalsIgnoreCase("NOANSI") || line.equalsIgnoreCase("FILTER_DEBUG") || line.equalsIgnoreCase("LOG_WEB") || line.equalsIgnoreCase("LOG_SERIAL") || line.equalsIgnoreCase("JOYSTICK_STATUS") || isWifiStaConfigCommand(line)) return ws.consoleAuthenticated || webDevMode;
    // 其余命令（控制 / 诊断）严格要求认证，不读 webDevMode。
    if (!ws.consoleAuthenticated) return false;
    if (isParkLockedWirelessCommand(line)) return car_output.park == PARK_LOCKED;
    return isWirelessControlCommand(line);
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
        // NACK 错误码分流：只有已认证用户在 Park 未锁定时收到 PARK_REQUIRED；
        // 未认证用户（即使 DEV ON）一律 UNAUTHORIZED，避免暗示"锁 Park 就能用"。
        if (isParkLockedWirelessCommand(line) && car_output.park != PARK_LOCKED && wifiRuntime.consoleAuthenticated) {
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
