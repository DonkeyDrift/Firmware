#include "WirelessConsole.h"

#include "SharedTypes.h"

#ifdef ENABLE_WIFI_CONSOLE
extern bool wifiConsoleAuthenticated;
extern bool wifiDevModeEnabled;
extern ControlData car_output;

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

bool isWirelessCommandAllowed(const String& line, WirelessCommandOrigin origin)
{
    bool webDevMode = wifiDevModeEnabled && origin == WIRELESS_ORIGIN_WEB;
    if (line.equalsIgnoreCase("PING") || line.equalsIgnoreCase("STATUS") || line.equalsIgnoreCase("WIFI_STA_STATUS")) return true;
    if (line.startsWith("AUTH:")) return true;
    if (isWirelessOtaOpenCommand(line)) return (webDevMode || wifiConsoleAuthenticated) && car_output.park == PARK_LOCKED;
    if (isWirelessOtaStatusCommand(line) || isWirelessOtaCloseCommand(line)) return webDevMode || wifiConsoleAuthenticated;
    if (!wifiConsoleAuthenticated && !webDevMode) return false;
    if (isParkLockedWirelessCommand(line)) return car_output.park == PARK_LOCKED;
    if (line.equalsIgnoreCase("ANSI") || line.equalsIgnoreCase("NOANSI") || line.equalsIgnoreCase("FILTER_DEBUG") || line.equalsIgnoreCase("LOG_WEB") || line.equalsIgnoreCase("LOG_SERIAL") || isWifiStaConfigCommand(line)) return true;
    return isWirelessControlCommand(line);
}
#endif
