#include "JoystickCalibration.h"

#include <Preferences.h>

#include "FirmwareConfig.h"
#include "Mus4Log.h"
#include "SharedTypes.h"
#include "TUI.h"
#include "WifiConsoleTypes.h"

extern TUI tui;
extern ControlData car_output;
extern uint16_t pwm_filtered[];

static WifiRuntimeState* g_ws = nullptr;

void setJoystickCalibrationRuntimeState(WifiRuntimeState& ws)
{
    g_ws = &ws;
}

static inline Preferences& prefs()
{
    static Preferences dummy;
    if (!g_ws || !g_ws->prefs) return dummy;
    return *g_ws->prefs;
}

JoystickCalibrationData joystick_cal;
JoystickCalState joystick_cal_state = JoystickCalState::IDLE;
unsigned long joystick_cal_stage_start_ms = 0;
int16_t joystick_cal_temp_min[2] = {0, 0};
int16_t joystick_cal_temp_max[2] = {0, 0};

static constexpr int16_t CAL_CAPTURE_INIT_MIN = 32767;
static constexpr int16_t CAL_CAPTURE_INIT_MAX = -32768;

static constexpr size_t CENTER_WINDOW_SIZE = 20;
static constexpr int16_t CENTER_STABLE_THRESHOLD_US = 6;
static constexpr int16_t CENTER_STABLE_COUNT_REQUIRED = 10;

static int16_t center_samples[2][CENTER_WINDOW_SIZE];
static size_t center_sample_index[2] = {0, 0};
static size_t center_sample_count[2] = {0, 0};
static int16_t center_stable_count[2] = {0, 0};

static int16_t computeWindowMedian(int16_t samples[], size_t count)
{
    if (count == 0) {
        return 0;
    }

    int16_t sorted[CENTER_WINDOW_SIZE];
    for (size_t i = 0; i < count; ++i) {
        sorted[i] = samples[i];
    }

    for (size_t i = 1; i < count; ++i) {
        int16_t key = sorted[i];
        int j = (int)i - 1;
        while (j >= 0 && sorted[j] > key) {
            sorted[j + 1] = sorted[j];
            --j;
        }
        sorted[j + 1] = key;
    }

    if (count % 2 == 1) {
        return sorted[count / 2];
    }
    return (sorted[count / 2 - 1] + sorted[count / 2]) / 2;
}

static void resetCenteringWindow()
{
    center_sample_index[CH_STEERING] = 0;
    center_sample_index[CH_THROTTLE] = 0;
    center_sample_count[CH_STEERING] = 0;
    center_sample_count[CH_THROTTLE] = 0;
    center_stable_count[CH_STEERING] = 0;
    center_stable_count[CH_THROTTLE] = 0;
}

static void captureCenterAndAdvance()
{
    joystick_cal.steering.mid_pwm = (int16_t)pwm_filtered[CH_STEERING];
    joystick_cal.throttle.mid_pwm = (int16_t)pwm_filtered[CH_THROTTLE];

    char buf[96];
    snprintf(buf, sizeof(buf), "[CAL] Center captured: steer=%d throt=%d",
             joystick_cal.steering.mid_pwm, joystick_cal.throttle.mid_pwm);
    tui.log(buf);
    mus4Logf("cal", "center captured: steer=%d throt=%d",
             joystick_cal.steering.mid_pwm, joystick_cal.throttle.mid_pwm);

    joystick_cal_state = JoystickCalState::MINMAX;
    joystick_cal_stage_start_ms = millis();
    joystick_cal_temp_min[CH_STEERING] = CAL_CAPTURE_INIT_MIN;
    joystick_cal_temp_max[CH_STEERING] = CAL_CAPTURE_INIT_MAX;
    joystick_cal_temp_min[CH_THROTTLE] = CAL_CAPTURE_INIT_MIN;
    joystick_cal_temp_max[CH_THROTTLE] = CAL_CAPTURE_INIT_MAX;

    tui.log("[CAL] Swing both sticks full range within 5s...");
}

int mapJoystickAxis(int16_t pwm,
                    const AxisCalibration& cal,
                    bool enabled,
                    int16_t default_min,
                    int16_t default_mid,
                    int16_t default_max)
{
    if (!enabled) {
        if (pwm < default_mid) {
            long mapped = map(pwm, default_min, default_mid, -100, 0);
            return constrain(mapped, -100, 0);
        }
        long mapped = map(pwm, default_mid, default_max, 0, 100);
        return constrain(mapped, 0, 100);
    }

    if (pwm < cal.mid_pwm) {
        long mapped = map(pwm, cal.min_pwm, cal.mid_pwm, -100, 0);
        return constrain(mapped, -100, 0);
    }

    long mapped = map(pwm, cal.mid_pwm, cal.max_pwm, 0, 100);
    return constrain(mapped, 0, 100);
}

bool validateJoystickCalibration(const AxisCalibration& axis)
{
    if (axis.min_pwm >= axis.mid_pwm || axis.mid_pwm >= axis.max_pwm) {
        return false;
    }
    if ((axis.mid_pwm - axis.min_pwm) <= 100) {
        return false;
    }
    if ((axis.max_pwm - axis.mid_pwm) <= 100) {
        return false;
    }
    if (axis.min_pwm < RC_PWM_MIN || axis.max_pwm > RC_PWM_MAX) {
        return false;
    }
    return true;
}

void loadJoystickCalibration()
{
    joystick_cal.steering.min_pwm = RC_STEERING_MIN;
    joystick_cal.steering.mid_pwm = RC_STEERING_MID;
    joystick_cal.steering.max_pwm = RC_STEERING_MAX;
    joystick_cal.throttle.min_pwm = RC_THROTTLE_MIN;
    joystick_cal.throttle.mid_pwm = RC_THROTTLE_MID;
    joystick_cal.throttle.max_pwm = RC_THROTTLE_MAX;
    joystick_cal.steering_enabled = false;
    joystick_cal.throttle_enabled = false;

    if (!prefs().begin(MUS4_PREF_NAMESPACE, true)) {
        mus4LogLine("cal", "joystick load: prefs open failed");
        return;
    }

    const bool has_new_steer = prefs().isKey(MUS4_PREF_JOYSTICK_STEER_EN_KEY);
    const bool has_new_throt = prefs().isKey(MUS4_PREF_JOYSTICK_THROT_EN_KEY);

    if (has_new_steer) {
        joystick_cal.steering_enabled = prefs().getBool(MUS4_PREF_JOYSTICK_STEER_EN_KEY, false);
        if (joystick_cal.steering_enabled) {
            joystick_cal.steering.min_pwm = (int16_t)prefs().getShort(MUS4_PREF_JOYSTICK_STEER_MIN_KEY, RC_STEERING_MIN);
            joystick_cal.steering.mid_pwm = (int16_t)prefs().getShort(MUS4_PREF_JOYSTICK_STEER_MID_KEY, RC_STEERING_MID);
            joystick_cal.steering.max_pwm = (int16_t)prefs().getShort(MUS4_PREF_JOYSTICK_STEER_MAX_KEY, RC_STEERING_MAX);
        }
    } else if (prefs().isKey(MUS4_PREF_STEER_MIN_KEY)) {
        // Migrate legacy steering calibration to the new unified keys.
        joystick_cal.steering_enabled = prefs().getBool(MUS4_PREF_STEER_CAL_EN_KEY, false);
        if (joystick_cal.steering_enabled) {
            joystick_cal.steering.min_pwm = (int16_t)prefs().getShort(MUS4_PREF_STEER_MIN_KEY, RC_STEERING_MIN);
            joystick_cal.steering.mid_pwm = (int16_t)prefs().getShort(MUS4_PREF_STEER_MID_KEY, RC_STEERING_MID);
            joystick_cal.steering.max_pwm = (int16_t)prefs().getShort(MUS4_PREF_STEER_MAX_KEY, RC_STEERING_MAX);
        }
        prefs().end();
        saveJoystickCalibration();
        return;
    }

    if (has_new_throt) {
        joystick_cal.throttle_enabled = prefs().getBool(MUS4_PREF_JOYSTICK_THROT_EN_KEY, false);
        if (joystick_cal.throttle_enabled) {
            joystick_cal.throttle.min_pwm = (int16_t)prefs().getShort(MUS4_PREF_JOYSTICK_THROT_MIN_KEY, RC_THROTTLE_MIN);
            joystick_cal.throttle.mid_pwm = (int16_t)prefs().getShort(MUS4_PREF_JOYSTICK_THROT_MID_KEY, RC_THROTTLE_MID);
            joystick_cal.throttle.max_pwm = (int16_t)prefs().getShort(MUS4_PREF_JOYSTICK_THROT_MAX_KEY, RC_THROTTLE_MAX);
        }
    }

    prefs().end();

    mus4Logf("cal",
             "joystick load: steer_en=%d steer={%d,%d,%d} throt_en=%d throt={%d,%d,%d}",
             joystick_cal.steering_enabled ? 1 : 0,
             joystick_cal.steering.min_pwm, joystick_cal.steering.mid_pwm, joystick_cal.steering.max_pwm,
             joystick_cal.throttle_enabled ? 1 : 0,
             joystick_cal.throttle.min_pwm, joystick_cal.throttle.mid_pwm, joystick_cal.throttle.max_pwm);
}

bool saveJoystickCalibration()
{
    if (joystick_cal.steering_enabled) {
        if (!validateJoystickCalibration(joystick_cal.steering)) {
            mus4LogLine("cal", "joystick save failed: steering calibration invalid");
            return false;
        }
    } else {
        joystick_cal.steering.min_pwm = RC_STEERING_MIN;
        joystick_cal.steering.mid_pwm = RC_STEERING_MID;
        joystick_cal.steering.max_pwm = RC_STEERING_MAX;
    }

    if (joystick_cal.throttle_enabled) {
        if (!validateJoystickCalibration(joystick_cal.throttle)) {
            mus4LogLine("cal", "joystick save failed: throttle calibration invalid");
            return false;
        }
    } else {
        joystick_cal.throttle.min_pwm = RC_THROTTLE_MIN;
        joystick_cal.throttle.mid_pwm = RC_THROTTLE_MID;
        joystick_cal.throttle.max_pwm = RC_THROTTLE_MAX;
    }

    if (!prefs().begin(MUS4_PREF_NAMESPACE, false)) {
        mus4LogLine("cal", "joystick save: prefs open failed");
        return false;
    }

    prefs().putShort(MUS4_PREF_JOYSTICK_STEER_MIN_KEY, joystick_cal.steering.min_pwm);
    prefs().putShort(MUS4_PREF_JOYSTICK_STEER_MID_KEY, joystick_cal.steering.mid_pwm);
    prefs().putShort(MUS4_PREF_JOYSTICK_STEER_MAX_KEY, joystick_cal.steering.max_pwm);
    prefs().putBool(MUS4_PREF_JOYSTICK_STEER_EN_KEY, joystick_cal.steering_enabled);

    prefs().putShort(MUS4_PREF_JOYSTICK_THROT_MIN_KEY, joystick_cal.throttle.min_pwm);
    prefs().putShort(MUS4_PREF_JOYSTICK_THROT_MID_KEY, joystick_cal.throttle.mid_pwm);
    prefs().putShort(MUS4_PREF_JOYSTICK_THROT_MAX_KEY, joystick_cal.throttle.max_pwm);
    prefs().putBool(MUS4_PREF_JOYSTICK_THROT_EN_KEY, joystick_cal.throttle_enabled);

    prefs().end();

    mus4Logf("cal",
             "joystick saved: steer_en=%d steer={%d,%d,%d} throt_en=%d throt={%d,%d,%d}",
             joystick_cal.steering_enabled ? 1 : 0,
             joystick_cal.steering.min_pwm, joystick_cal.steering.mid_pwm, joystick_cal.steering.max_pwm,
             joystick_cal.throttle_enabled ? 1 : 0,
             joystick_cal.throttle.min_pwm, joystick_cal.throttle.mid_pwm, joystick_cal.throttle.max_pwm);
    return true;
}

void resetJoystickCalibration()
{
    joystick_cal.steering.min_pwm = RC_STEERING_MIN;
    joystick_cal.steering.mid_pwm = RC_STEERING_MID;
    joystick_cal.steering.max_pwm = RC_STEERING_MAX;
    joystick_cal.throttle.min_pwm = RC_THROTTLE_MIN;
    joystick_cal.throttle.mid_pwm = RC_THROTTLE_MID;
    joystick_cal.throttle.max_pwm = RC_THROTTLE_MAX;
    joystick_cal.steering_enabled = false;
    joystick_cal.throttle_enabled = false;

    if (prefs().begin(MUS4_PREF_NAMESPACE, false)) {
        prefs().remove(MUS4_PREF_JOYSTICK_STEER_MIN_KEY);
        prefs().remove(MUS4_PREF_JOYSTICK_STEER_MID_KEY);
        prefs().remove(MUS4_PREF_JOYSTICK_STEER_MAX_KEY);
        prefs().remove(MUS4_PREF_JOYSTICK_STEER_EN_KEY);
        prefs().remove(MUS4_PREF_JOYSTICK_THROT_MIN_KEY);
        prefs().remove(MUS4_PREF_JOYSTICK_THROT_MID_KEY);
        prefs().remove(MUS4_PREF_JOYSTICK_THROT_MAX_KEY);
        prefs().remove(MUS4_PREF_JOYSTICK_THROT_EN_KEY);
        prefs().end();
    }

    mus4LogLine("cal", "joystick calibration reset to defaults");
}

void printJoystickCalStatus(Print& out)
{
    out.printf("JOYSTICK_CAL steer_en=%d steer={%d,%d,%d} throt_en=%d throt={%d,%d,%d} state=%d\n",
               joystick_cal.steering_enabled ? 1 : 0,
               joystick_cal.steering.min_pwm, joystick_cal.steering.mid_pwm, joystick_cal.steering.max_pwm,
               joystick_cal.throttle_enabled ? 1 : 0,
               joystick_cal.throttle.min_pwm, joystick_cal.throttle.mid_pwm, joystick_cal.throttle.max_pwm,
               (int)joystick_cal_state);
}

bool startJoystickCalibration(Print& out)
{
    if (car_output.park != PARK_LOCKED) {
        out.println("NACK:PARK_REQUIRED");
        return false;
    }

    joystick_cal_state = JoystickCalState::CENTERING;
    joystick_cal_stage_start_ms = millis();
    joystick_cal_temp_min[CH_STEERING] = CAL_CAPTURE_INIT_MIN;
    joystick_cal_temp_max[CH_STEERING] = CAL_CAPTURE_INIT_MAX;
    joystick_cal_temp_min[CH_THROTTLE] = CAL_CAPTURE_INIT_MIN;
    joystick_cal_temp_max[CH_THROTTLE] = CAL_CAPTURE_INIT_MAX;
    resetCenteringWindow();

    tui.log("[CAL] Center both sticks, auto-capture in 3s...");
    mus4LogLine("cal", "joystick center stage started");
    return true;
}

void updateJoystickCalibration()
{
    if (joystick_cal_state == JoystickCalState::IDLE) return;

    const unsigned long now = millis();
    const unsigned long elapsed = now - joystick_cal_stage_start_ms;

    if (joystick_cal_state == JoystickCalState::CENTERING) {
        const int16_t steer_sample = (int16_t)pwm_filtered[CH_STEERING];
        const int16_t throt_sample = (int16_t)pwm_filtered[CH_THROTTLE];

        bool all_stable = true;
        for (int ch = 0; ch < 2; ++ch) {
            const int16_t sample = (ch == CH_STEERING) ? steer_sample : throt_sample;
            const size_t idx = center_sample_index[ch];
            center_samples[ch][idx] = sample;
            center_sample_index[ch] = (idx + 1) % CENTER_WINDOW_SIZE;
            if (center_sample_count[ch] < CENTER_WINDOW_SIZE) {
                ++center_sample_count[ch];
            }

            const int16_t median = computeWindowMedian(center_samples[ch], center_sample_count[ch]);
            if (abs(sample - median) <= CENTER_STABLE_THRESHOLD_US) {
                ++center_stable_count[ch];
            } else {
                center_stable_count[ch] = 0;
            }

            if (center_stable_count[ch] < CENTER_STABLE_COUNT_REQUIRED) {
                all_stable = false;
            }
        }

        if (all_stable) {
            captureCenterAndAdvance();
            return;
        }

        if (elapsed < 3000) return;

        tui.log("[CAL] Center capture unstable, using median");
        mus4LogLine("cal", "center capture unstable, using median");

        joystick_cal.steering.mid_pwm = computeWindowMedian(center_samples[CH_STEERING], center_sample_count[CH_STEERING]);
        joystick_cal.throttle.mid_pwm = computeWindowMedian(center_samples[CH_THROTTLE], center_sample_count[CH_THROTTLE]);

        char buf[96];
        snprintf(buf, sizeof(buf), "[CAL] Center captured: steer=%d throt=%d",
                 joystick_cal.steering.mid_pwm, joystick_cal.throttle.mid_pwm);
        tui.log(buf);
        mus4Logf("cal", "center captured: steer=%d throt=%d",
                 joystick_cal.steering.mid_pwm, joystick_cal.throttle.mid_pwm);

        joystick_cal_state = JoystickCalState::MINMAX;
        joystick_cal_stage_start_ms = now;
        joystick_cal_temp_min[CH_STEERING] = CAL_CAPTURE_INIT_MIN;
        joystick_cal_temp_max[CH_STEERING] = CAL_CAPTURE_INIT_MAX;
        joystick_cal_temp_min[CH_THROTTLE] = CAL_CAPTURE_INIT_MIN;
        joystick_cal_temp_max[CH_THROTTLE] = CAL_CAPTURE_INIT_MAX;

        tui.log("[CAL] Swing both sticks full range within 5s...");
    } else if (joystick_cal_state == JoystickCalState::MINMAX) {
        const int16_t steer_current = (int16_t)pwm_filtered[CH_STEERING];
        const int16_t throt_current = (int16_t)pwm_filtered[CH_THROTTLE];

        if (steer_current < joystick_cal_temp_min[CH_STEERING]) {
            joystick_cal_temp_min[CH_STEERING] = steer_current;
        }
        if (steer_current > joystick_cal_temp_max[CH_STEERING]) {
            joystick_cal_temp_max[CH_STEERING] = steer_current;
        }
        if (throt_current < joystick_cal_temp_min[CH_THROTTLE]) {
            joystick_cal_temp_min[CH_THROTTLE] = throt_current;
        }
        if (throt_current > joystick_cal_temp_max[CH_THROTTLE]) {
            joystick_cal_temp_max[CH_THROTTLE] = throt_current;
        }

        if (elapsed < 5000) return;

        joystick_cal.steering.min_pwm = joystick_cal_temp_min[CH_STEERING];
        joystick_cal.steering.max_pwm = joystick_cal_temp_max[CH_STEERING];
        joystick_cal.throttle.min_pwm = joystick_cal_temp_min[CH_THROTTLE];
        joystick_cal.throttle.max_pwm = joystick_cal_temp_max[CH_THROTTLE];

        char buf[128];
        snprintf(buf, sizeof(buf),
                 "[CAL] Range captured: steer={%d,%d} throt={%d,%d}",
                 joystick_cal.steering.min_pwm, joystick_cal.steering.max_pwm,
                 joystick_cal.throttle.min_pwm, joystick_cal.throttle.max_pwm);
        tui.log(buf);
        mus4Logf("cal",
                 "range captured: steer={%d,%d} throt={%d,%d}",
                 joystick_cal.steering.min_pwm, joystick_cal.steering.max_pwm,
                 joystick_cal.throttle.min_pwm, joystick_cal.throttle.max_pwm);

        joystick_cal_state = JoystickCalState::DONE;

        snprintf(buf, sizeof(buf),
                 "[CAL] Result: steer={%d,%d,%d} throt={%d,%d,%d}",
                 joystick_cal.steering.min_pwm, joystick_cal.steering.mid_pwm, joystick_cal.steering.max_pwm,
                 joystick_cal.throttle.min_pwm, joystick_cal.throttle.mid_pwm, joystick_cal.throttle.max_pwm);
        tui.log(buf);
        tui.log("[CAL] Send CAL_SAVE / CAL_RETRY / CAL_ABORT");
    }
}

void abortJoystickCalibration()
{
    joystick_cal_state = JoystickCalState::IDLE;
    mus4LogLine("cal", "joystick calibration aborted");
}
