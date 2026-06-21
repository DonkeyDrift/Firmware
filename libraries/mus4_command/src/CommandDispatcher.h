#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"
#include "SerialBufferTypes.h"

#ifdef ENABLE_WIFI_CONSOLE
#include "RuntimeState.h"
void setCommandDispatcherRuntimeStates(OtaRuntimeState& os, WifiRuntimeState& ws);
#endif

// pilotSilent: when true, suppress ACK/NACK responses for Pilot throttle/steering
// data frames. This avoids confusing host software (e.g. DonkeyCar) that does
// not expect replies to its high-rate control packets.
bool dispatchCommandLine(const String& line, Print& out, SerialBuf& sb, bool pilotSilent = false);
