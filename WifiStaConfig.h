#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE
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
