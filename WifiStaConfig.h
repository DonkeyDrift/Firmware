#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE
bool copyWifiStaSsid(const String& ssid);
bool copyWifiStaPassword(const String& password);
String wifiStaIpText();
void clearWifiStaLastError();
void setWifiStaLastError(const char* code, const char* message, bool timedOut);
void scheduleWifiStaApply();
bool saveWifiStaSsidPreference(const String& ssid);
bool saveWifiStaPasswordPreference(const String& password);
void printWifiStaStatus(Print& out);
bool processWifiStaConfigCommand(const String& line, Print& out);
#else
inline bool processWifiStaConfigCommand(const String& line, Print& out)
{
    (void)line;
    (void)out;
    return false;
}
#endif
