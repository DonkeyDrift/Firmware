#include "ControlMixer.h"
#include "FirmwareConfig.h"
#include "SharedTypes.h"
#include "RcFilter.h"
#include "RcPwmCapture.h"
#include "JoystickCalibration.h"
#include "DriftAssist.h"
#include "LedStatus.h"
#include "LedBlinkPreference.h"
#include "Mus4Log.h"
#include "Buzzer.h"

#ifdef ENABLE_GAMEPAD_MODE
#include "GamepadMode.h"
#endif

extern ControlData rc_data;
extern ControlData pilot_data;
extern ControlData car_output;
extern uint16_t pwm_filtered[];
extern JoystickCalibrationData joystick_cal;
extern bool toggleActive;
extern Buzzer buzzer;

static int lastCarMode = -1;
static int hostMode = -1;    // 上位机命令模式覆盖；-1 = 无覆盖
static int lastRcMode = -1;  // 上次生效的 RC 派生模式；-1 = 未初始化

static void applyMode(int mode)
{
    car_output.mode = mode;
    if (car_output.mode != lastCarMode)
    {
        buzzer.playModeSound(car_output.mode);
        lastCarMode = car_output.mode;
    }
}

bool setCarModeCommand(int mode)
{
    if (mode < CAR_MODE_MANUAL || mode > CAR_MODE_FULL_AUTO)
    {
        return false;
    }
    hostMode = mode;
    applyMode(mode); // 立即生效
    return true;
}

void mode_change(bool modeValid)
{
    if (!modeValid) {
        // RC 信号无效：保持当前模式（含上位机覆盖）
        return;
    }

    rc_data.mode = pwm_filtered[CH_MODE];
    int rcMode;
    if (rc_data.mode <= MODE_PWM_MANUAL_MAX)
    {
        rcMode = CAR_MODE_MANUAL; // 0: RC manual mode
    }
    else if (rc_data.mode >= MODE_PWM_FULL_AUTO_MIN)
    {
        rcMode = CAR_MODE_FULL_AUTO; // 2: autonomous driving mode
    }
    else
    {
        rcMode = CAR_MODE_SEMI_AUTO; // 1: Pilot steering with manual throttle
    }

    if (rcMode != lastRcMode)
    {
        // 遥控器模式开关位置发生变化 → 遥控器覆盖上位机设置
        hostMode = -1;
        lastRcMode = rcMode;
        applyMode(rcMode);
    }
    else if (hostMode >= 0)
    {
        // RC 未变且存在上位机覆盖 → 维持上位机设置
        applyMode(hostMode);
    }
    else
    {
        applyMode(rcMode);
        lastRcMode = rcMode;
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
        car_output.steering = apply_drift_assist(pilot_data.steering);

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
            car_output.throttle = mapJoystickAxis(rc_data.throttle,
                                                    joystick_cal.throttle,
                                                    joystick_cal.throttle_enabled,
                                                    RC_THROTTLE_MIN,
                                                    RC_THROTTLE_MID,
                                                    RC_THROTTLE_MAX);
        }
        car_output.steering = apply_drift_assist(pilot_data.steering);
    }
    else
    {
        // Controlled by RC Controller (car_output.mode = CAR_MODE_MANUAL)
        if (car_output.park == 1)
        {
            static uint8_t appliedBlinkMask = 0xFF; // 0xFF is invalid, forces the first apply
            uint8_t mask = getLedBlinkMask();
            if (carOutputModeLast != CAR_MODE_MANUAL || mask != appliedBlinkMask || !toggleActive)
            {
                applyLedBlinkMask(mask); // idle blink colors from Web Console selection
                appliedBlinkMask = mask;
                carOutputModeLast = CAR_MODE_MANUAL;
            }
        }
        else
        {
            setLEDColor(CRGB::Green); // set LED to blue

            // RC => CAR
            car_output.throttle = mapJoystickAxis(rc_data.throttle,
                                                    joystick_cal.throttle,
                                                    joystick_cal.throttle_enabled,
                                                    RC_THROTTLE_MIN,
                                                    RC_THROTTLE_MID,
                                                    RC_THROTTLE_MAX);
        }
        car_output.steering = mapJoystickAxis(rc_data.steering,
                                                joystick_cal.steering,
                                                joystick_cal.steering_enabled,
                                                RC_STEERING_MIN,
                                                RC_STEERING_MID,
                                                RC_STEERING_MAX);
        // Drift Assist: add counter-steer compensation when enabled
        car_output.steering = apply_drift_assist(car_output.steering);
    }

    // Drift Assist: modulate throttle when enabled and not parked
    if (car_output.park != PARK_LOCKED) {
        car_output.throttle = apply_drift_throttle(car_output.throttle);
    }
}
