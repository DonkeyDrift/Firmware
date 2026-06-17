#pragma once
#include <Arduino.h>
#include "SharedTypes.h"
#include "RuntimeState.h"

void setSteeringCalibrationRuntimeState(WifiRuntimeState& ws);

struct SteeringCalibration {
    int16_t min_pwm;
    int16_t mid_pwm;
    int16_t max_pwm;
};

enum SteerCalState {
    STEER_CAL_IDLE,
    STEER_CAL_CENTER,
    STEER_CAL_MINMAX,
    STEER_CAL_DONE
};

extern SteeringCalibration steer_cal;
extern bool steer_cal_enabled;
extern SteerCalState steer_cal_state;
extern unsigned long steer_cal_stage_start_ms;
extern int16_t steer_cal_temp_min;
extern int16_t steer_cal_temp_max;

void loadSteeringCalibration();
bool saveSteeringCalibration();
void resetSteeringCalibration();
int mapSteeringCalibrated(int16_t pwm);
void printCalStatus(Print& out);
bool startSteerCalibration(Print& out);
void updateSteerCalibration();
