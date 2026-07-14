#pragma once
#include <Arduino.h>
#include "WifiConsoleTypes.h"

extern bool drift_assist_enabled;
extern bool drift_assist_active;
extern float drift_compensation;
extern float gyro_z_filtered;
extern float drift_assist_scale;
extern float drift_yaw_error;
extern float drift_steering_correction;
extern int8_t drift_throttle_mode;

void update_drift_assist_control(bool driftValid, bool driftScaleValid);
void load_drift_config(const DriftConfig& config);
int apply_drift_assist(int driver_steering);
int apply_drift_throttle(int driver_throttle);
