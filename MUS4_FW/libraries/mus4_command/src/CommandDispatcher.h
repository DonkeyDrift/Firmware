#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"
#include "SerialBufferTypes.h"

#ifdef ENABLE_AUTH_SERVICE
#include "AuthService.h"
#endif

#ifdef ENABLE_WIFI_CONSOLE
#include "RuntimeState.h"
void setCommandDispatcherRuntimeStates(OtaRuntimeState& os, WifiRuntimeState& ws);
#endif

// pilotSilent: when true, suppress ACK/NACK responses for Pilot throttle/steering
// data frames. This avoids confusing host software (e.g. DonkeyCar) that does
// not expect replies to its high-rate control packets.
bool dispatchCommandLine(const String& line, Print& out, SerialBuf& sb, bool pilotSilent = false);

// 上位机 pilot 控制流最后有效 T:S 帧的 millis()（dispatchCommandLine 内盖戳）；
// ControlMixer 在 FULL_AUTO 下用它做失效保护判据
extern unsigned long pilot_last_frame_ms;
