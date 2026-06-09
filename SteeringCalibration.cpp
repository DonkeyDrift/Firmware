#include "SteeringCalibration.h"

#include <Preferences.h>

#include "FirmwareConfig.h"
#include "Mus4Log.h"
#include "SharedTypes.h"
#include "TUI.h"

extern Preferences mus4Prefs;
extern TUI tui;
extern ControlData car_output;
extern uint16_t pwm_filtered[];
extern const char* MUS4_PREF_NAMESPACE;
extern const char* MUS4_PREF_STEER_MIN_KEY;
extern const char* MUS4_PREF_STEER_MID_KEY;
extern const char* MUS4_PREF_STEER_MAX_KEY;
extern const char* MUS4_PREF_STEER_CAL_EN_KEY;

SteeringCalibration steer_cal;
bool steer_cal_enabled = false;
SteerCalState steer_cal_state = STEER_CAL_IDLE;
unsigned long steer_cal_stage_start_ms = 0;
int16_t steer_cal_temp_min = 0;
int16_t steer_cal_temp_max = 0;

void loadSteeringCalibration()
{
    steer_cal.min_pwm = RC_STEERING_MIN;
    steer_cal.mid_pwm = RC_STEERING_MID;
    steer_cal.max_pwm = RC_STEERING_MAX;
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, true)) {
        steer_cal_enabled = false;
        return;
    }
    steer_cal_enabled = mus4Prefs.getBool(MUS4_PREF_STEER_CAL_EN_KEY, false);
    if (steer_cal_enabled) {
        steer_cal.min_pwm = (int16_t)mus4Prefs.getShort(MUS4_PREF_STEER_MIN_KEY, RC_STEERING_MIN);
        steer_cal.mid_pwm = (int16_t)mus4Prefs.getShort(MUS4_PREF_STEER_MID_KEY, RC_STEERING_MID);
        steer_cal.max_pwm = (int16_t)mus4Prefs.getShort(MUS4_PREF_STEER_MAX_KEY, RC_STEERING_MAX);
    }
    mus4Prefs.end();
    mus4Logf("cal", "steer_cal enabled=%d min=%d mid=%d max=%d",
             steer_cal_enabled ? 1 : 0, steer_cal.min_pwm, steer_cal.mid_pwm, steer_cal.max_pwm);
}

bool saveSteeringCalibration()
{
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    mus4Prefs.putShort(MUS4_PREF_STEER_MIN_KEY, steer_cal.min_pwm);
    mus4Prefs.putShort(MUS4_PREF_STEER_MID_KEY, steer_cal.mid_pwm);
    mus4Prefs.putShort(MUS4_PREF_STEER_MAX_KEY, steer_cal.max_pwm);
    mus4Prefs.putBool(MUS4_PREF_STEER_CAL_EN_KEY, true);
    mus4Prefs.end();
    steer_cal_enabled = true;
    mus4Logf("cal", "saved min=%d mid=%d max=%d", steer_cal.min_pwm, steer_cal.mid_pwm, steer_cal.max_pwm);
    return true;
}

void resetSteeringCalibration()
{
    steer_cal.min_pwm = RC_STEERING_MIN;
    steer_cal.mid_pwm = RC_STEERING_MID;
    steer_cal.max_pwm = RC_STEERING_MAX;
    steer_cal_enabled = false;
    if (mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) {
        mus4Prefs.remove(MUS4_PREF_STEER_MIN_KEY);
        mus4Prefs.remove(MUS4_PREF_STEER_MID_KEY);
        mus4Prefs.remove(MUS4_PREF_STEER_MAX_KEY);
        mus4Prefs.remove(MUS4_PREF_STEER_CAL_EN_KEY);
        mus4Prefs.end();
    }
    mus4LogLine("cal", "reset to defaults");
}

int mapSteeringCalibrated(int16_t pwm)
{
    if (pwm < steer_cal.mid_pwm) {
        return map(pwm, steer_cal.min_pwm, steer_cal.mid_pwm, -100, 0);
    } else {
        return map(pwm, steer_cal.mid_pwm, steer_cal.max_pwm, 0, 100);
    }
}

void printCalStatus(Print& out)
{
    out.printf("CAL_STATUS enabled=%d min=%d mid=%d max=%d state=%d\n",
               steer_cal_enabled ? 1 : 0,
               steer_cal.min_pwm, steer_cal.mid_pwm, steer_cal.max_pwm,
               (int)steer_cal_state);
}

bool startSteerCalibration(Print& out)
{
    if (car_output.park != PARK_LOCKED) {
        out.println("NACK:PARK_REQUIRED");
        return false;
    }
    steer_cal_state = STEER_CAL_CENTER;
    steer_cal_stage_start_ms = millis();
    steer_cal_temp_min = 32767;
    steer_cal_temp_max = -32768;
    tui.log("[CAL] Keep steering centered, auto-capture in 3s...");
    mus4LogLine("cal", "center stage started");
    return true;
}

void updateSteerCalibration()
{
    if (steer_cal_state == STEER_CAL_IDLE) return;

    unsigned long now = millis();
    unsigned long elapsed = now - steer_cal_stage_start_ms;

    if (steer_cal_state == STEER_CAL_CENTER) {
        if (elapsed < 3000) return;
        steer_cal.mid_pwm = (int16_t)pwm_filtered[CH_STEERING];
        char buf[64];
        snprintf(buf, sizeof(buf), "[CAL] Center captured: %d", steer_cal.mid_pwm);
        tui.log(buf);
        mus4Logf("cal", "center=%d", steer_cal.mid_pwm);
        steer_cal_state = STEER_CAL_MINMAX;
        steer_cal_stage_start_ms = now;
        steer_cal_temp_min = 32767;
        steer_cal_temp_max = -32768;
        tui.log("[CAL] Swing stick full left/right within 5s...");
    } else if (steer_cal_state == STEER_CAL_MINMAX) {
        int16_t current = (int16_t)pwm_filtered[CH_STEERING];
        if (current < steer_cal_temp_min) steer_cal_temp_min = current;
        if (current > steer_cal_temp_max) steer_cal_temp_max = current;
        if (elapsed < 5000) return;
        steer_cal.min_pwm = steer_cal_temp_min;
        steer_cal.max_pwm = steer_cal_temp_max;
        char buf[96];
        snprintf(buf, sizeof(buf), "[CAL] Range captured: min=%d max=%d", steer_cal.min_pwm, steer_cal.max_pwm);
        tui.log(buf);
        mus4Logf("cal", "range min=%d max=%d", steer_cal.min_pwm, steer_cal.max_pwm);
        steer_cal_state = STEER_CAL_DONE;
        snprintf(buf, sizeof(buf), "[CAL] Result: mid=%d min=%d max=%d", steer_cal.mid_pwm, steer_cal.min_pwm, steer_cal.max_pwm);
        tui.log(buf);
        tui.log("[CAL] Send CAL_SAVE / CAL_RETRY / CAL_ABORT");
    }
}
