#include "DriftAssist.h"

#include "FirmwareConfig.h"
#include "SharedTypes.h"

// --- 漂移辅助编译期开关 ---
#define DRIFT_ASSIST_ENABLED     1        // 编译期开启漂移辅助

bool drift_assist_enabled = false;   // 用户是否已启用辅助（RC CH5）
bool drift_assist_active = false;    // 辅助当前是否正在介入
float drift_compensation = 0.0f;     // 当前平滑后的转向补偿量（-100~100）
float gyro_z_filtered = 0.0f;        // 滤波后的 gyroZ
float drift_assist_scale = 1.0f;     // CH6 漂移辅助强度比例
float drift_yaw_error = 0.0f;        // 当前 yaw rate 跟踪误差（rad/s）
float drift_steering_correction = 0.0f; // 当前转向纠正量（-100~100）
int8_t drift_throttle_mode = 0;      // 0=透传/滑行, 1=点动油门, 2=持续油门
float last_driver_steering_norm = 0.0f; // 上一次驾驶员转向输入（-1~1）

static DriftConfig drift_config = {
    WIFI_DRIFT_STEERING_GYRO_SIGN_DEFAULT,
    WIFI_DRIFT_MAX_YAW_RATE_DEFAULT,
    WIFI_DRIFT_KP_DEFAULT,
    WIFI_DRIFT_KD_DEFAULT,
    WIFI_DRIFT_MAX_STEERING_CORRECTION_DEFAULT,
    WIFI_DRIFT_GYRO_FILTER_ALPHA_DEFAULT,
    WIFI_DRIFT_SPIN_THRESHOLD_DEFAULT,
    WIFI_DRIFT_STEERING_THRESHOLD_DEFAULT,
    WIFI_DRIFT_CONTINUOUS_THROTTLE_DEFAULT,
    WIFI_DRIFT_PULSE_THROTTLE_DEFAULT,
    WIFI_DRIFT_PULSE_FREQ_HZ_DEFAULT,
    WIFI_DRIFT_PULSE_DUTY_DEFAULT
};

extern uint16_t pwm_filtered[];
extern SensorData mpu6050Data;
extern ControlData car_output;

void load_drift_config(const DriftConfig& config)
{
    if (!isValidDriftConfig(config)) return;
    drift_config = config;
}

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
        drift_yaw_error = 0.0f;
        drift_steering_correction = 0.0f;
        drift_throttle_mode = 0;
    }
    drift_assist_enabled = enabled;
}

// --- 漂移辅助逻辑 ---
// 输入：driver_steering 为驾驶员原始转向输入（-100~100）。
// 输出：叠加漂移补偿后的最终转向值（-100~100）。
int apply_drift_assist(int driver_steering) {
#if DRIFT_ASSIST_ENABLED
    if (!drift_assist_enabled) {
        drift_assist_active = false;
        drift_compensation = 0.0f;
        drift_yaw_error = 0.0f;
        drift_steering_correction = 0.0f;
        last_driver_steering_norm = driver_steering / 100.0f;
        return driver_steering;
    }

    // 1. 低通滤波 gyroZ
    gyro_z_filtered = gyro_z_filtered * (1.0f - drift_config.gyroFilterAlpha) +
                      mpu6050Data.gyroZ * drift_config.gyroFilterAlpha;

    // 2. 记录驾驶员转向（归一化 -1~1），供油门策略使用
    last_driver_steering_norm = driver_steering / 100.0f;

    // 3. 计算期望 yaw rate 与误差
    float target_yaw = last_driver_steering_norm * drift_config.maxYawRate *
                       (float)drift_config.steeringGyroSign;
    drift_yaw_error = target_yaw - gyro_z_filtered;

    // 4. P+D 反馈生成转向纠正量（归一化 -1~1）
    float correction_norm = (drift_config.kp * drift_yaw_error -
                             drift_config.kd * gyro_z_filtered) *
                            (float)drift_config.steeringGyroSign;
    correction_norm = constrain(correction_norm,
                                -drift_config.maxSteeringCorrection,
                                drift_config.maxSteeringCorrection);

    // 5. 叠加 CH6 强度比例并平滑输出
    float scaled_correction = correction_norm * 100.0f * drift_assist_scale;
    float max_corr = min(drift_config.maxSteeringCorrection * 100.0f * drift_assist_scale, 100.0f);
    scaled_correction = constrain(scaled_correction, -max_corr, max_corr);

    drift_compensation = drift_compensation * (1.0f - drift_config.gyroFilterAlpha) +
                         scaled_correction * drift_config.gyroFilterAlpha;
    drift_steering_correction = drift_compensation;

    // 6. 判定当前是否正在介入
    drift_assist_active = fabsf(gyro_z_filtered) > drift_config.spinThreshold * 0.3f ||
                          fabsf(drift_compensation) > 0.5f;

    // 7. 叠加补偿并限制输出
    int final_steering = driver_steering + (int)drift_compensation;
    final_steering = constrain(final_steering, -100, 100);

    return final_steering;
#else
    drift_assist_active = false;
    drift_compensation = 0.0f;
    drift_yaw_error = 0.0f;
    drift_steering_correction = 0.0f;
    last_driver_steering_norm = driver_steering / 100.0f;
    return driver_steering;
#endif
}

// --- 漂移油门策略 ---
// 输入：driver_throttle 为当前已合并的油门输出（-100~100）。
// 输出：漂移辅助调整后的油门输出（-100~100）。
int apply_drift_throttle(int driver_throttle) {
#if DRIFT_ASSIST_ENABLED
    if (!drift_assist_enabled || !mpu6050Data.valid) {
        drift_throttle_mode = 0;
        return driver_throttle;
    }

    // 仅对前进油门进行漂移调制，保留刹车/倒车
    if (driver_throttle < 0) {
        drift_throttle_mode = 0;
        return driver_throttle;
    }

    // 转向小且陀螺仪未进入旋转：透传原油门
    if (fabsf(last_driver_steering_norm) < drift_config.steeringThreshold &&
        fabsf(gyro_z_filtered) < drift_config.spinThreshold * 0.6f) {
        drift_throttle_mode = 0;
        return driver_throttle;
    }

    // 高 yaw rate：持续小油门（原地/小半径旋转）
    if (fabsf(gyro_z_filtered) > drift_config.spinThreshold) {
        drift_throttle_mode = 2;
        return (int)(drift_config.continuousThrottle * 100.0f * drift_assist_scale);
    }

    // 否则按频率/占空比生成点动油门（大半径漂移）
    float freq = max(drift_config.pulseFreqHz, 0.5f);
    unsigned long period_ms = (unsigned long)(1000.0f / freq);
    unsigned long on_ms = (unsigned long)(period_ms * drift_config.pulseDuty);
    unsigned long phase = millis() % period_ms;
    if (phase < on_ms) {
        drift_throttle_mode = 1;
        return (int)(drift_config.pulseThrottle * 100.0f * drift_assist_scale);
    }
    drift_throttle_mode = 0;
    return 0;
#else
    drift_throttle_mode = 0;
    return driver_throttle;
#endif
}
