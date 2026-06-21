#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"
#include "RuntimeState.h"
#include "SerialBufferTypes.h"
#include "WirelessConsole.h"

#ifdef ENABLE_WIFI_CONSOLE
unsigned long wifiOtaTtlMs(OtaRuntimeState& os, WifiRuntimeState& ws);
void printWifiOtaStatus(Print& out, OtaRuntimeState& os, WifiRuntimeState& ws);
void closeWifiOtaWindow(const char* reason, OtaRuntimeState& os);
void forceWifiOtaParkLocked();
void keepDevModeOtaWindowActive(OtaRuntimeState& os, WifiRuntimeState& ws);
bool shouldEmitSerial1Telemetry(OtaRuntimeState& os);
void openLocalWifiOtaWindow(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os);
bool processLocalOtaMaintenanceCommand(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os, WifiRuntimeState& ws);
void openWifiOtaWindow(Print& out, WirelessCommandOrigin origin, OtaRuntimeState& os, WifiRuntimeState& ws);
void updateWifiOta(OtaRuntimeState& os, WifiRuntimeState& ws);
#else
inline unsigned long wifiOtaTtlMs(OtaRuntimeState& os, WifiRuntimeState& ws) { (void)os; (void)ws; return 0; }
inline void printWifiOtaStatus(Print& out, OtaRuntimeState& os, WifiRuntimeState& ws) { (void)out; (void)os; (void)ws; }
inline void closeWifiOtaWindow(const char* reason, OtaRuntimeState& os) { (void)reason; (void)os; }
inline void forceWifiOtaParkLocked() {}
inline void keepDevModeOtaWindowActive(OtaRuntimeState& os, WifiRuntimeState& ws) { (void)os; (void)ws; }
inline bool shouldEmitSerial1Telemetry(OtaRuntimeState& os) { (void)os; return true; }
inline void openLocalWifiOtaWindow(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os) { (void)line; (void)out; (void)sb; (void)os; }
inline bool processLocalOtaMaintenanceCommand(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os, WifiRuntimeState& ws) { (void)line; (void)out; (void)sb; (void)os; (void)ws; return false; }
inline void openWifiOtaWindow(Print& out, WirelessCommandOrigin origin, OtaRuntimeState& os, WifiRuntimeState& ws) { (void)out; (void)origin; (void)os; (void)ws; }
inline void updateWifiOta(OtaRuntimeState& os, WifiRuntimeState& ws) { (void)os; (void)ws; }
#endif