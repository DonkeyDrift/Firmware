#pragma once
#include <Arduino.h>
#include "SharedTypes.h"
#include "RuntimeState.h"

void setJoystickCalibrationRuntimeState(WifiRuntimeState& ws);

struct AxisCalibration {
    int16_t min_pwm = 0;
    int16_t mid_pwm = 0;
    int16_t max_pwm = 0;
};

struct JoystickCalibrationData {
    AxisCalibration steering;
    AxisCalibration throttle;
    bool steering_enabled = false;
    bool throttle_enabled = false;
};

enum class JoystickCalState {
    IDLE,
    CENTERING,
    MINMAX,
    DONE
};

extern JoystickCalibrationData joystick_cal;
extern JoystickCalState joystick_cal_state;
extern unsigned long joystick_cal_stage_start_ms;
extern int16_t joystick_cal_temp_min[2];
extern int16_t joystick_cal_temp_max[2];

void loadJoystickCalibration();
bool saveJoystickCalibration();
void resetJoystickCalibration();

int mapJoystickAxis(int16_t pwm,
                    const AxisCalibration& cal,
                    bool enabled,
                    int16_t default_min,
                    int16_t default_mid,
                    int16_t default_max);

void printJoystickCalStatus(Print& out);
bool startJoystickCalibration(Print& out);
void updateJoystickCalibration();
void abortJoystickCalibration();
bool validateJoystickCalibration(const AxisCalibration& axis);
