#pragma once
#include <Arduino.h>

extern bool drift_assist_enabled;
extern bool drift_assist_active;
extern float drift_compensation;
extern float gyro_z_filtered;
extern float drift_assist_scale;

void update_drift_assist_control(bool driftValid, bool driftScaleValid);
int apply_drift_assist(int driver_steering);
