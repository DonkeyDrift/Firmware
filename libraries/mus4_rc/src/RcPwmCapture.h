#pragma once
#include <Arduino.h>
#include "FirmwareConfig.h"

// Raw PWM values updated by ISRs (volatile for interrupt safety)
extern volatile uint16_t pwm_value[RC_CHANNEL_COUNT];
// Timestamp of last valid pulse per channel (microseconds)
extern volatile unsigned long last_valid_time[RC_CHANNEL_COUNT];

// Initialize RC receiver pins, attach interrupts, and optionally set up MCPWM capture.
void setupRcPwmCapture();
