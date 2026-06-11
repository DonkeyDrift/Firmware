#include "ControlMixer.h"
#include "FirmwareConfig.h"
#include "SharedTypes.h"
#include "RcFilter.h"
#include "RcPwmCapture.h"
#include "SteeringCalibration.h"
#include "DriftAssist.h"
#include "LedStatus.h"
#include "Mus4Log.h"
#include "Buzzer.h"

#ifdef ENABLE_GAMEPAD_MODE
#include "GamepadMode.h"
#endif

extern ControlData rc_data;
extern ControlData pilot_data;
extern ControlData car_output;
extern uint16_t pwm_filtered[];
extern bool steer_cal_enabled;
extern bool toggleActive;
extern Buzzer buzzer;

static int lastCarMode = -1;

void mode_change(bool modeValid)
{
    if (!modeValid) {
        return;
    }

    rc_data.mode = pwm_filtered[CH_MODE];
    if (rc_data.mode <= MODE_PWM_MANUAL_MAX)
    {
        car_output.mode = CAR_MODE_MANUAL; // 0: RC manual mode
    }
    else if (rc_data.mode >= MODE_PWM_FULL_AUTO_MIN)
    {
        car_output.mode = CAR_MODE_FULL_AUTO; // 2: autonomous driving mode
    }
    else
    {
        car_output.mode = CAR_MODE_SEMI_AUTO; // 1: Pilot steering with manual throttle
    }

    if (car_output.mode != lastCarMode)
    {
        buzzer.playModeSound(car_output.mode);
        lastCarMode = car_output.mode;
    }
}

static int carOutputModeLast = -1;

void updateControlOutput()
{
    if (car_output.mode == CAR_MODE_FULL_AUTO)
    {
        // Controlled by Pilot
        if (car_output.park == 1)
        {
            if (carOutputModeLast != CAR_MODE_FULL_AUTO || !toggleActive)
            {
                setLEDToggle(CRGB::Blue, CRGB::Red);
                carOutputModeLast = CAR_MODE_FULL_AUTO;
            }
            if (!toggleActive)
            {
                setLEDToggle(CRGB::Blue, CRGB::Red);
            }
        }
        else
        {
            setLEDColor(CRGB::Blue); // set LED to Red
            car_output.throttle = pilot_data.throttle;
        }
        car_output.steering = pilot_data.steering;

        #ifdef ENABLE_GAMEPAD_MODE
            sendGamepadPacket();
        #endif
    }
    else if (car_output.mode == CAR_MODE_SEMI_AUTO)
    {
        // Controlled by both RC and Pilot
        if (car_output.park == 1)
        {
            if (carOutputModeLast != CAR_MODE_SEMI_AUTO || !toggleActive)
            {
                setLEDToggle(CRGB::Yellow, CRGB::Red);
                carOutputModeLast = CAR_MODE_SEMI_AUTO;
            }
        }
        else
        {
            setLEDColor(CRGB::Yellow); // set LED to blue
            car_output.throttle = map(rc_data.throttle, RC_THROTTLE_MIN, RC_THROTTLE_MAX, -100, 100);
        }
        car_output.steering = pilot_data.steering;
    }
    else
    {
        // Controlled by RC Controller (car_output.mode = CAR_MODE_MANUAL)
        if (car_output.park == 1)
        {
            if (carOutputModeLast != CAR_MODE_MANUAL || !toggleActive)
            {
                setLEDToggle(CRGB::Green, CRGB::Red);
                carOutputModeLast = CAR_MODE_MANUAL;
            }
        }
        else
        {
            setLEDColor(CRGB::Green); // set LED to blue

            // RC => CAR
            car_output.throttle = map(rc_data.throttle, RC_THROTTLE_MIN, RC_THROTTLE_MAX, -100, 100);
        }
        if (steer_cal_enabled) {
            car_output.steering = mapSteeringCalibrated(rc_data.steering);
        } else {
            car_output.steering = map(rc_data.steering, RC_STEERING_MIN, RC_STEERING_MAX, -100, 100);
        }
        // Drift Assist: add counter-steer compensation only in manual mode
        car_output.steering = apply_drift_assist(car_output.steering);
    }
}
