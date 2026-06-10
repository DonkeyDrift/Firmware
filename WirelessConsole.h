#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE
enum WirelessCommandOrigin { WIRELESS_ORIGIN_TCP, WIRELESS_ORIGIN_WEB };

bool isWirelessControlCommand(const String& line);
bool isWirelessOtaOpenCommand(const String& line);
bool isLocalOtaOpenCommand(const String& line);
bool isWirelessOtaStatusCommand(const String& line);
bool isWirelessOtaCloseCommand(const String& line);
bool isWifiStaConfigCommand(const String& line);
bool isParkLockedWirelessCommand(const String& line);
bool isWirelessCommandAllowed(const String& line, WirelessCommandOrigin origin);
String redactWirelessConsoleLine(const String& line);
#endif
