#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"
#include "SerialBufferTypes.h"

#ifdef ENABLE_WIFI_CONSOLE
unsigned long wifiOtaTtlMs();
void printWifiOtaStatus(Print& out);
void closeWifiOtaWindow(const char* reason);
void forceWifiOtaParkLocked();
void keepDevModeOtaWindowActive();
bool shouldEmitSerial1Telemetry();
void openLocalWifiOtaWindow(const String& line, Print& out, SerialBuf& sb);
#else
inline unsigned long wifiOtaTtlMs() { return 0; }
inline void printWifiOtaStatus(Print& out) { (void)out; }
inline void closeWifiOtaWindow(const char* reason) { (void)reason; }
inline void forceWifiOtaParkLocked() {}
inline void keepDevModeOtaWindowActive() {}
inline bool shouldEmitSerial1Telemetry() { return true; }
inline void openLocalWifiOtaWindow(const String& line, Print& out, SerialBuf& sb) { (void)line; (void)out; (void)sb; }
#endif