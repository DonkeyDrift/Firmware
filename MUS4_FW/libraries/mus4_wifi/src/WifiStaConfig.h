#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE
#include "RuntimeState.h"

void setWifiRuntimeState(WifiRuntimeState& ws);

bool copyWifiStaSsid(const String& ssid);
bool copyWifiStaPassword(const String& password);
String wifiStaIpText();
String wifiApIpText();
void clearWifiStaLastError();
void setWifiStaLastError(const char* code, const char* message, bool timedOut);
void scheduleWifiStaApply();
bool saveWifiStaPreference(const String& ssid, const String& password);
bool saveWifiStaSsidPreference(const String& ssid);
bool saveWifiStaPasswordPreference(const String& password);
void clearWifiStaRuntimeStateWithoutDisconnect();
bool clearWifiStaPreference();
void loadWifiStaPreference();
void printWifiStaStatus(Print& out);
bool processWifiStaConfigCommand(const String& line, Print& out, WifiRuntimeState& ws);
#else
inline bool processWifiStaConfigCommand(const String& line, Print& out)
{
    (void)line;
    (void)out;
    return false;
}
#endif
