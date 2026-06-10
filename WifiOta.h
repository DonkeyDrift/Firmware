#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE
unsigned long wifiOtaTtlMs();
void printWifiOtaStatus(Print& out);
#else
inline unsigned long wifiOtaTtlMs() { return 0; }
inline void printWifiOtaStatus(Print& out) { (void)out; }
#endif