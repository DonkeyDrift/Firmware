#pragma once

#include <stdint.h>

// Idle LED blink color selection (v1.7.51).
//
// Bitmask selecting which colors the WS2812B status LED cycles through while
// the car is idle (MANUAL mode + Park locked): bit0 = red, bit1 = green,
// bit2 = blue. One bit set -> that color blinks on/off (equal on/off time);
// two/three bits -> alternating flash; 0 -> LED off while idle. Persisted in NVS namespace "webui", key
// "ledblink" (UChar 0-7); default 7 (red+green+blue) when the key is absent,
// and the choice is restored after power off / reboot.
//
// loadLedBlinkPreference() runs early in setup(); the ControlMixer reads
// getLedBlinkMask() every loop and re-applies on change, so a
// POST /api/led-blink takes effect immediately, without a restart.

void loadLedBlinkPreference();
bool saveLedBlinkPreference(uint8_t mask);
uint8_t getLedBlinkMask();
