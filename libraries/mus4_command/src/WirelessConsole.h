#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE
#include "RuntimeState.h"

enum WirelessCommandOrigin { WIRELESS_ORIGIN_TCP, WIRELESS_ORIGIN_WEB };

void processWirelessConsoleLine(const String& line, Print& out, WirelessCommandOrigin origin);
void printWirelessStatus(Print& out);

bool isWirelessControlCommand(const String& line);
bool isWirelessOtaOpenCommand(const String& line);
bool isLocalOtaOpenCommand(const String& line);
bool isWirelessOtaStatusCommand(const String& line);
bool isWirelessOtaCloseCommand(const String& line);
bool isWifiStaConfigCommand(const String& line);
bool isParkLockedWirelessCommand(const String& line);
bool isWirelessCommandAllowed(const String& line, WirelessCommandOrigin origin, WifiRuntimeState& ws);
String redactWirelessConsoleLine(const String& line);
#endif
