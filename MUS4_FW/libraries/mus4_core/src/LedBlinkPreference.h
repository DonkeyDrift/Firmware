#pragma once

#include <stdint.h>

// Idle LED blink color selection (v1.7.51).
//
// The idle (MANUAL + Park locked) WS2812B status LED blink is now fixed to
// red+green+blue (mask = 7) and is no longer user-configurable: the Web Console
// RGB toggle and the /api/led-blink endpoints were removed (Issue #107). The
// mask is a compile-time constant and is no longer persisted in NVS.
//
// getLedBlinkMask() is still read by the ControlMixer every loop to drive the
// idle blink; it always returns 7.

uint8_t getLedBlinkMask();
