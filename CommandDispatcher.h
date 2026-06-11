#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"
#include "SerialBufferTypes.h"

#ifdef ENABLE_WIFI_CONSOLE
#include "RuntimeState.h"
void setCommandDispatcherRuntimeStates(OtaRuntimeState& os, WifiRuntimeState& ws);
#endif

bool dispatchCommandLine(const String& line, Print& out, SerialBuf& sb);
