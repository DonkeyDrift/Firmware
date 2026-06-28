#pragma once
#include <Arduino.h>
#include "FirmwareConfig.h"

// Servo midpoint duty (300Hz/14bit ledc), runtime-configurable via NVS.
// Default 7372 = 1500µs. Use SERVO_MID command to adjust.
extern int servo_mid_v;

// Motor/ESC midpoint duty (300Hz/14bit ledc), runtime-configurable via NVS.
// Default 7372 = 1500µs. Use MOTOR_MID command to adjust.
extern int motor_mid_v;

// Servo range (±500µs @ 300Hz/14bit), extern for Diagnostics.cpp linkage.
extern const int SERVO_RANGE_V;
extern const int MOTOR_RANGE_V;

// Last written PWM duty values (300Hz/14bit ledc), for TUI display
extern int actuator_steering_duty;
extern int actuator_throttle_duty;

// Initialize PWM output channels (ledcAttachChannel)
void setupActuatorOutput();

// Write current car_output values to servo/ESC PWM channels
void updateActuatorOutput();
