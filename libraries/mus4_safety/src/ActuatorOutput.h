#pragma once
#include <Arduino.h>
#include "FirmwareConfig.h"

// Servo midpoint and range (extern for Diagnostics.cpp linkage)
extern const int SERVO_MID_V;
extern const int SERVO_RANGE_V;

// Initialize PWM output channels (ledcAttachChannel)
void setupActuatorOutput();

// Write current car_output values to servo/ESC PWM channels
void updateActuatorOutput();
