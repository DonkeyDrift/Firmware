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
    // WIFI|<ssid>|<password>（Serial2 上位机配网协议）：保留 ssid，脱敏密码段；
    // 前缀大小写敏感，与 isWirelessModeCommand 等现有 startsWith 判断一致。
    if (line.startsWith("WIFI|")) {
        int secondPipe = line.indexOf('|', 5);
        if (secondPipe > 5) return line.substring(0, secondPipe + 1) + "<redacted>";
        return "WIFI|<redacted>";
    }
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

bool isCalibrationCommand(const String& line)
{
    return line.equalsIgnoreCase("STEER_CAL") ||
        line.equalsIgnoreCase("CAL_SAVE") ||
        line.equalsIgnoreCase("CAL_RETRY") ||
        line.equalsIgnoreCase("CAL_ABORT") ||
        line.equalsIgnoreCase("CAL_RESET") ||
        line.equalsIgnoreCase("CAL_STATUS") ||
        line.equalsIgnoreCase("JOYSTICK_CAL") ||
        line.equalsIgnoreCase("JOYSTICK_SAVE") ||
        line.equalsIgnoreCase("JOYSTICK_RETRY") ||
        line.equalsIgnoreCase("JOYSTICK_ABORT") ||
        line.equalsIgnoreCase("JOYSTICK_RESET");
}

bool isWirelessModeCommand(const String& line)
{
    if (!line.startsWith("MODE ") && !line.startsWith("MODE:")) return false;
    String arg = line.substring(5);
    arg.trim();
    return arg.length() == 1 && arg.charAt(0) >= '0' && arg.charAt(0) <= '2';
}

bool isWirelessCommandAllowed(const String& line, WirelessCommandOrigin origin, WifiRuntimeState& ws)
{
    // DEV ON 仅对 Web 来源放权 "OTA + Web 配置 + 显示/日志切换 + WIFI_STA_* + 校准命令"；
    // 校准命令免认证但仍需 Park 锁定；控制命令与其他诊断命令（TEST/BENCH 等）
    // 始终要求认证。详见 docs/Plan/DEV模式影响面与运行逻辑映射.md §2.8、§3。
    bool webDevMode = ws.devModeEnabled && origin == WIRELESS_ORIGIN_WEB;
    // 控制台密码为空时视为已认证（isWirelessConsoleAuthDisabled）。
    const bool authed = ws.consoleAuthenticated || isWirelessConsoleAuthDisabled();
    if (line.equalsIgnoreCase("PING") || line.equalsIgnoreCase("STATUS") || line.equalsIgnoreCase("WIFI_STA_STATUS")) return true;
    if (line.startsWith("AUTH:")) return true;
    if (isWirelessOtaOpenCommand(line)) return (webDevMode || authed) && car_output.park == PARK_LOCKED;
    if (isWirelessOtaStatusCommand(line) || isWirelessOtaCloseCommand(line)) return webDevMode || authed;
    // DEV ON 显式白名单：显示/日志切换、Wi-Fi STA 配置类命令。
    if (line.equalsIgnoreCase("ANSI") || line.equalsIgnoreCase("NOANSI") || line.equalsIgnoreCase("FILTER_DEBUG") || line.equalsIgnoreCase("LOG_WEB") || line.equalsIgnoreCase("LOG_SERIAL") || line.equalsIgnoreCase("JOYSTICK_STATUS") || line.startsWith("SERVO_MID") || line.startsWith("MOTOR_MID") || line.startsWith("THROTTLE_MIN") || line.startsWith("THROTTLE_MAX") || isWifiStaConfigCommand(line)) return authed || webDevMode;
    // DEV ON 允许校准命令免认证（但仍需 Park 锁定）。
    if (webDevMode && isCalibrationCommand(line)) return car_output.park == PARK_LOCKED;
    // 其余命令（控制 / 诊断）严格要求认证，不读 webDevMode。
    if (!authed) return false;
    if (isWirelessModeCommand(line)) return true; // 模式命令需认证，Park 锁定下也允许（油门仍钳 0）
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
        // NACK 错误码分流：已认证用户或 DEV ON Web 来源在 Park 未锁定时收到
        // PARK_REQUIRED；纯未认证用户收到 UNAUTHORIZED。
        if (isParkLockedWirelessCommand(line) && car_output.park != PARK_LOCKED && (wifiRuntime.consoleAuthenticated || isWirelessConsoleAuthDisabled() || (wifiRuntime.devModeEnabled && origin == WIRELESS_ORIGIN_WEB))) {
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
