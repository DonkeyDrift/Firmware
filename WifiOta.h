#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE
unsigned long wifiOtaTtlMs();
void printWifiOtaStatus(Print& out);
void closeWifiOtaWindow(const char* reason);
void forceWifiOtaParkLocked();
void keepDevModeOtaWindowActive();
#else
inline unsigned long wifiOtaTtlMs() { return 0; }
inline void printWifiOtaStatus(Print& out) { (void)out; }
inline void closeWifiOtaWindow(const char* reason) { (void)reason; }
inline void forceWifiOtaParkLocked() {}
inline void keepDevModeOtaWindowActive() {}
#endif