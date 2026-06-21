#include "DriftAssist.h"

#include "FirmwareConfig.h"
#include "SharedTypes.h"

// --- 漂移辅助常量与运行状态 ---
#define DRIFT_ASSIST_ENABLED     1        // 编译期开启漂移辅助
#define DRIFT_ASSIST_GAIN        25.0f    // 反打增益：gyroZ rad/s -> ±100 补偿
#define DRIFT_ASSIST_THRESHOLD   1.2f     // 低于该角速度阈值时不介入
#define DRIFT_ASSIST_MAX_COMP    70       // 限制最大补偿角度，避免过度反打
#define DRIFT_ASSIST_SMOOTH      0.25f    // 一阶平滑系数，降低输出抖动
#define DRIFT_ASSIST_DECAY       0.85f    // 未触发漂移时的补偿衰减系数

bool drift_assist_enabled = false;   // 用户是否已启用辅助
bool drift_assist_active = false;    // 辅助当前是否正在介入
float drift_compensation = 0.0f;     // 当前平滑后的补偿量
float gyro_z_filtered = 0.0f;        // 滤波后的 gyroZ
float drift_assist_scale = 1.0f;     // CH6 漂移辅助强度比例
// ------------------------------------------------------

extern uint16_t pwm_filtered[];
extern SensorData mpu6050Data;
extern ControlData car_output;

void update_drift_assist_control(bool driftValid, bool driftScaleValid)
{
    if (driftScaleValid) {
        uint16_t scalePwm = constrain(pwm_filtered[CH_DRIFT_SCALE], 1000, 2000);
        drift_assist_scale = (scalePwm - 1000) / 500.0f;
    } else {
        drift_assist_scale = 1.0f;
    }

    bool enabled = driftValid && pwm_filtered[CH_DRIFT] > 1500;
    if (!enabled) {
        drift_assist_active = false;
        drift_compensation = 0.0f;
        gyro_z_filtered = 0.0f;
    }
    drift_assist_enabled = enabled;
}

// --- 漂移辅助逻辑 ---
// 输入：driver_steering 为驾驶员原始转向输入（-100~100）。
// 输出：叠加漂移补偿后的最终转向值（-100~100）。
int apply_drift_assist(int driver_steering) {
#if DRIFT_ASSIST_ENABLED
    // 当漂移辅助已启用时介入（手动/半自动/全自动均可）。
    if (!drift_assist_enabled) {
        drift_assist_active = false;
        drift_compensation = 0.0f;
        return driver_steering;
    }

    // 1. Apply a first-order low-pass filter to gyroZ to remove sensor noise
    gyro_z_filtered = gyro_z_filtered * (1.0f - DRIFT_ASSIST_SMOOTH) +
                      mpu6050Data.gyroZ * DRIFT_ASSIST_SMOOTH;

    // 2. Determine whether drift is triggered
    float abs_gyro = fabs(gyro_z_filtered);
    if (abs_gyro > DRIFT_ASSIST_THRESHOLD) {
        // 3. Calculate counter-steer compensation. A negative sign would invert direction
        // (clockwise slide -> negative gyro -> positive compensation -> steer right?),
        // but the physical direction must be aligned first.
        // User definition: clockwise rear slide gives negative gyroZ -> counter-steer left is needed (<1439 -> negative value).
        // Therefore, negative gyroZ should produce negative compensation.
        // Formula: comp = gyroZ * GAIN (keep the same sign).
        // Compensation direction:
        // - gyroZ > 0: vehicle is rotating counter-clockwise.
        // - gyroZ < 0: vehicle is rotating clockwise.
        // User calibration:
        // Steering PWM center is 1439.
        // PWM < 1439 means steering left (negative normalized steering).
        // PWM > 1439 means steering right (positive normalized steering).
        // Clockwise rear slide (rear swings right / oversteers right) -> counter-steer left -> add a negative steering value.
        // At this moment gyroZ is negative -> compensation should also be negative.
        // Therefore compensation = gyroZ * GAIN.
        // Implement this logic first; adjust the sign during real-vehicle tuning if needed.
        float raw_comp = gyro_z_filtered * DRIFT_ASSIST_GAIN * drift_assist_scale;

        // 4. Clamp compensation
        float effectiveMaxComp = min(DRIFT_ASSIST_MAX_COMP * drift_assist_scale, 100.0f);
        raw_comp = constrain(raw_comp, -effectiveMaxComp, effectiveMaxComp);

        // 5. Smooth compensation output
        drift_compensation = drift_compensation * (1.0f - DRIFT_ASSIST_SMOOTH) +
                             raw_comp * DRIFT_ASSIST_SMOOTH;

        drift_assist_active = true;
    } else {
        // Below threshold; gradually decay compensation to 0
        drift_compensation *= DRIFT_ASSIST_DECAY;
        if (fabs(drift_compensation) < 0.5f) {
            drift_compensation = 0.0f;
            drift_assist_active = false;
        } else {
            drift_assist_active = true;
        }
    }

    // 6. Add compensation to the raw driver input and clamp the result
    int final_steering = driver_steering + (int)drift_compensation;
    final_steering = constrain(final_steering, -100, 100);

    return final_steering;
#else
    drift_assist_active = false;
    drift_compensation = 0.0f;
    return driver_steering;
#endif
}
