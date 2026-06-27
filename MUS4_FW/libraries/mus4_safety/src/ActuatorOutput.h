#pragma once
#include <Arduino.h>
#include "FirmwareConfig.h"

// Servo midpoint and range (extern for Diagnostics.cpp linkage)
extern const int SERVO_MID_V;
extern const int SERVO_RANGE_V;

// Last written PWM duty values (300Hz/14bit ledc), for TUI display
extern int actuator_steering_duty;
extern int actuator_throttle_duty;

// Initialize PWM output channels (ledcAttachChannel)
void setupActuatorOutput();

// Write current car_output values to servo/ESC PWM channels
void updateActuatorOutput();
