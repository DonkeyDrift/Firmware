#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE
// Use static linkage to avoid ODR violations when this header is included by
// multiple translation units.
static const char* WIFI_CONSOLE_AP_DEFAULT_SSID = "MUS4-ESP";
static const char* WIFI_CONSOLE_AP_PASSWORD = "mus4-debug";
static const char* WIFI_AP_SSID_SUFFIX = "-ESP";
static const uint8_t WIFI_AP_SSID_PREFIX_MAX_LEN = 6;
static const uint16_t WIFI_CONSOLE_PORT = 2323;
static const uint16_t WIFI_WEB_CONSOLE_PORT = 80;
static const uint32_t WIFI_WEB_TELEMETRY_MIN_FREE_HEAP = 60000;
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
static const uint16_t WIFI_WEB_SOCKET_PORT = 81;
static const unsigned long WIFI_WEB_SOCKET_PUSH_INTERVAL_MS = 16;
static const uint8_t WIFI_WEB_SOCKET_MAX_POINTS_PER_FRAME = 8;
static const uint8_t WIFI_WEB_SOCKET_MAX_REPLAY_POINTS = 32;
static const uint16_t WIFI_WEB_SOCKET_KEEPALIVE_SECONDS = 60;
static const uint8_t WIFI_WEB_SOCKET_MAX_CLIENTS = 2;
#endif
static const uint8_t WIFI_CONSOLE_CHANNEL = 6;
static const uint8_t WIFI_CONSOLE_MAX_CLIENTS = 1;
static const unsigned long WIFI_CONSOLE_RETRY_INTERVAL_MS = 5000;
static const unsigned long WIFI_STA_CONNECT_TIMEOUT_MS = 15000;
static const unsigned long WIFI_STA_APPLY_DELAY_MS = 800;
// STA 连上拿到 IP 后，把 SoftAP 名临时改成 "MUS4-<sta_ip>" 广播此时长，
// 让用户在 Wi-Fi 列表直接读出设备 IP，然后自动关闭 AP 进入 STA-only。
static const unsigned long WIFI_STA_IP_DISPLAY_MS = 60000;
static const char* WIFI_STA_IP_AP_PREFIX = "MUS4-";
static const uint8_t WIFI_BOOT_RESET_PIN = 0;
static const unsigned long WIFI_BOOT_RESET_HOLD_MS = 3000;
// v1.7.18 起：AP/STA 互斥切换的去抖窗口。STA 进入 WL_CONNECTED 后等待
// WIFI_STA_GRACE_UP_MS 才关闭 AP；STA 脱离 WL_CONNECTED 后等待
// WIFI_STA_GRACE_DOWN_MS 才启动 AP，避免短暂抖动反复切换。
static const unsigned long WIFI_STA_GRACE_UP_MS = 1000;
static const unsigned long WIFI_STA_GRACE_DOWN_MS = 1000;
static const char* WIFI_OTA_HOSTNAME = "mus4-ota";
static const char* WIFI_OTA_PASSWORD = "mus4-debug";
static const uint16_t WIFI_OTA_PORT = 3232;
static const unsigned long WIFI_OTA_WINDOW_MS = 120000UL;
static const char* MUS4_PREF_NAMESPACE = "mus4";
static const char* MUS4_PREF_DEV_MODE_KEY = "dev_mode";
static const char* MUS4_PREF_AP_SSID_KEY = "ap_ssid";
static const char* MUS4_PREF_STA_ENABLED_KEY = "sta_en";
static const char* MUS4_PREF_STA_SSID_KEY = "sta_ssid";
static const char* MUS4_PREF_STA_PASSWORD_KEY = "sta_pass";
static const char* MUS4_PREF_STEER_MIN_KEY = "str_min";
static const char* MUS4_PREF_STEER_MID_KEY = "str_mid";
static const char* MUS4_PREF_STEER_MAX_KEY = "str_max";
static const char* MUS4_PREF_STEER_CAL_EN_KEY = "str_cal";

// --- Joystick calibration keys (unified steering + throttle) ---
static const char* MUS4_PREF_JOYSTICK_STEER_MIN_KEY = "js_st_min";
static const char* MUS4_PREF_JOYSTICK_STEER_MID_KEY = "js_st_mid";
static const char* MUS4_PREF_JOYSTICK_STEER_MAX_KEY = "js_st_max";
static const char* MUS4_PREF_JOYSTICK_STEER_EN_KEY  = "js_st_en";

static const char* MUS4_PREF_JOYSTICK_THROT_MIN_KEY = "js_th_min";
static const char* MUS4_PREF_JOYSTICK_THROT_MID_KEY = "js_th_mid";
static const char* MUS4_PREF_JOYSTICK_THROT_MAX_KEY = "js_th_max";
static const char* MUS4_PREF_JOYSTICK_THROT_EN_KEY  = "js_th_en";
static const char* MUS4_PREF_JUDGE_COLLISION_THRESHOLD_KEY = "jd_col_th";
static const char* MUS4_PREF_JUDGE_BIG_TURN_THRESHOLD_KEY = "jd_turn_th";
static const char* MUS4_PREF_JUDGE_WINDOW_SIZE_KEY = "jd_win_sz";
static const char* MUS4_PREF_JUDGE_COLLISION_PENALTY_KEY = "jd_col_pen";
static const char* MUS4_PREF_JUDGE_TURN_SMOOTHNESS_WEIGHT_KEY = "jd_turn_w";
static const char* MUS4_PREF_JUDGE_RANGE_MATCH_WEIGHT_KEY = "jd_rng_w";
static const char* MUS4_PREF_JUDGE_GYRO_STABILITY_WEIGHT_KEY = "jd_gyro_w";
static const char* MUS4_PREF_JUDGE_BIG_TURN_STABILITY_WEIGHT_KEY = "jd_big_w";
static const char* MUS4_PREF_JUDGE_SPEED_STABILITY_WEIGHT_KEY = "jd_spd_w";
static const char* MUS4_PREF_JUDGE_THROTTLE_STABILITY_WEIGHT_KEY = "jd_thr_w";

static const float WIFI_JUDGE_COLLISION_THRESHOLD_DEFAULT = 2.8f;
static const float WIFI_JUDGE_COLLISION_THRESHOLD_MIN = 0.5f;
static const float WIFI_JUDGE_COLLISION_THRESHOLD_MAX = 8.0f;
static const float WIFI_JUDGE_BIG_TURN_THRESHOLD_DEFAULT = 1.6f;
static const float WIFI_JUDGE_BIG_TURN_THRESHOLD_MIN = 0.3f;
static const float WIFI_JUDGE_BIG_TURN_THRESHOLD_MAX = 4.0f;
static const uint8_t WIFI_JUDGE_WINDOW_SIZE_DEFAULT = 20;
static const uint8_t WIFI_JUDGE_WINDOW_SIZE_MIN = 5;
static const uint8_t WIFI_JUDGE_WINDOW_SIZE_MAX = 64;
static const float WIFI_JUDGE_COLLISION_PENALTY_DEFAULT = 10.0f;
static const float WIFI_JUDGE_COLLISION_PENALTY_MIN = 1.0f;
static const float WIFI_JUDGE_COLLISION_PENALTY_MAX = 30.0f;
static const float WIFI_JUDGE_TURN_SMOOTHNESS_WEIGHT_DEFAULT = 35.0f;
static const float WIFI_JUDGE_TURN_SMOOTHNESS_WEIGHT_MIN = 10.0f;
static const float WIFI_JUDGE_TURN_SMOOTHNESS_WEIGHT_MAX = 80.0f;
static const float WIFI_JUDGE_RANGE_MATCH_WEIGHT_DEFAULT = 42.0f;
static const float WIFI_JUDGE_RANGE_MATCH_WEIGHT_MIN = 10.0f;
static const float WIFI_JUDGE_RANGE_MATCH_WEIGHT_MAX = 100.0f;
static const float WIFI_JUDGE_GYRO_STABILITY_WEIGHT_DEFAULT = 40.0f;
static const float WIFI_JUDGE_GYRO_STABILITY_WEIGHT_MIN = 10.0f;
static const float WIFI_JUDGE_GYRO_STABILITY_WEIGHT_MAX = 100.0f;
static const float WIFI_JUDGE_BIG_TURN_STABILITY_WEIGHT_DEFAULT = 34.0f;
static const float WIFI_JUDGE_BIG_TURN_STABILITY_WEIGHT_MIN = 10.0f;
static const float WIFI_JUDGE_BIG_TURN_STABILITY_WEIGHT_MAX = 90.0f;
static const float WIFI_JUDGE_SPEED_STABILITY_WEIGHT_DEFAULT = 220.0f;
static const float WIFI_JUDGE_SPEED_STABILITY_WEIGHT_MIN = 40.0f;
static const float WIFI_JUDGE_SPEED_STABILITY_WEIGHT_MAX = 400.0f;
static const float WIFI_JUDGE_THROTTLE_STABILITY_WEIGHT_DEFAULT = 180.0f;
static const float WIFI_JUDGE_THROTTLE_STABILITY_WEIGHT_MIN = 40.0f;
static const float WIFI_JUDGE_THROTTLE_STABILITY_WEIGHT_MAX = 360.0f;

static const uint8_t WIFI_AP_SSID_MAX_LEN = 32;
static const uint8_t WIFI_STA_SSID_MAX_LEN = 32;
static const uint8_t WIFI_STA_PASSWORD_MAX_LEN = 63;
static const uint8_t WIFI_STA_PASSWORD_MIN_LEN = 8;
static const uint8_t WIFI_WEB_LOG_CAPACITY = 64;
static const uint8_t SERIAL1_WEB_LOG_CAPACITY = 64;
static const uint16_t WIFI_WEB_DATA_CAPACITY = 256;
static const unsigned long WIFI_WEB_DATA_INTERVAL_MS = 16;

struct WebLogEntry { uint32_t seq; unsigned long t; char source[8]; char line[160]; };
struct WifiScanEntry { char ssid[WIFI_STA_SSID_MAX_LEN + 1]; int32_t rssi; int32_t channel; bool secure; };
struct JudgeConfig {
    float collisionThreshold;
    float bigTurnThreshold;
    uint8_t windowSize;
    float collisionPenalty;
    float turnSmoothnessWeight;
    float rangeMatchWeight;
    float gyroStabilityWeight;
    float bigTurnStabilityWeight;
    float speedStabilityWeight;
    float throttleStabilityWeight;
};

static inline JudgeConfig defaultJudgeConfig()
{
    JudgeConfig config = {
        WIFI_JUDGE_COLLISION_THRESHOLD_DEFAULT,
        WIFI_JUDGE_BIG_TURN_THRESHOLD_DEFAULT,
        WIFI_JUDGE_WINDOW_SIZE_DEFAULT,
        WIFI_JUDGE_COLLISION_PENALTY_DEFAULT,
        WIFI_JUDGE_TURN_SMOOTHNESS_WEIGHT_DEFAULT,
        WIFI_JUDGE_RANGE_MATCH_WEIGHT_DEFAULT,
        WIFI_JUDGE_GYRO_STABILITY_WEIGHT_DEFAULT,
        WIFI_JUDGE_BIG_TURN_STABILITY_WEIGHT_DEFAULT,
        WIFI_JUDGE_SPEED_STABILITY_WEIGHT_DEFAULT,
        WIFI_JUDGE_THROTTLE_STABILITY_WEIGHT_DEFAULT
    };
    return config;
}

static inline bool isValidJudgeConfig(const JudgeConfig& config)
{
    return !isnan(config.collisionThreshold) &&
        !isnan(config.bigTurnThreshold) &&
        config.collisionThreshold >= WIFI_JUDGE_COLLISION_THRESHOLD_MIN &&
        config.collisionThreshold <= WIFI_JUDGE_COLLISION_THRESHOLD_MAX &&
        config.bigTurnThreshold >= WIFI_JUDGE_BIG_TURN_THRESHOLD_MIN &&
        config.bigTurnThreshold <= WIFI_JUDGE_BIG_TURN_THRESHOLD_MAX &&
        config.windowSize >= WIFI_JUDGE_WINDOW_SIZE_MIN &&
        config.windowSize <= WIFI_JUDGE_WINDOW_SIZE_MAX &&
        !isnan(config.collisionPenalty) &&
        !isnan(config.turnSmoothnessWeight) &&
        !isnan(config.rangeMatchWeight) &&
        !isnan(config.gyroStabilityWeight) &&
        !isnan(config.bigTurnStabilityWeight) &&
        !isnan(config.speedStabilityWeight) &&
        !isnan(config.throttleStabilityWeight) &&
        config.collisionPenalty >= WIFI_JUDGE_COLLISION_PENALTY_MIN &&
        config.collisionPenalty <= WIFI_JUDGE_COLLISION_PENALTY_MAX &&
        config.turnSmoothnessWeight >= WIFI_JUDGE_TURN_SMOOTHNESS_WEIGHT_MIN &&
        config.turnSmoothnessWeight <= WIFI_JUDGE_TURN_SMOOTHNESS_WEIGHT_MAX &&
        config.rangeMatchWeight >= WIFI_JUDGE_RANGE_MATCH_WEIGHT_MIN &&
        config.rangeMatchWeight <= WIFI_JUDGE_RANGE_MATCH_WEIGHT_MAX &&
        config.gyroStabilityWeight >= WIFI_JUDGE_GYRO_STABILITY_WEIGHT_MIN &&
        config.gyroStabilityWeight <= WIFI_JUDGE_GYRO_STABILITY_WEIGHT_MAX &&
        config.bigTurnStabilityWeight >= WIFI_JUDGE_BIG_TURN_STABILITY_WEIGHT_MIN &&
        config.bigTurnStabilityWeight <= WIFI_JUDGE_BIG_TURN_STABILITY_WEIGHT_MAX &&
        config.speedStabilityWeight >= WIFI_JUDGE_SPEED_STABILITY_WEIGHT_MIN &&
        config.speedStabilityWeight <= WIFI_JUDGE_SPEED_STABILITY_WEIGHT_MAX &&
        config.throttleStabilityWeight >= WIFI_JUDGE_THROTTLE_STABILITY_WEIGHT_MIN &&
        config.throttleStabilityWeight <= WIFI_JUDGE_THROTTLE_STABILITY_WEIGHT_MAX;
}

struct WebDataPoint {
    uint32_t seq;
    unsigned long t;
    uint16_t dtMs;
    int throttle;
    int steering;
    int mode;
    bool park;
    int rcThrottle;
    int rcSteering;
    int rcChannels[RC_CHANNEL_COUNT];
    int pilotThrottle;
    int pilotSteering;
    float currentMa;
    float voltage;
    float gyroZ;
    float gyroX;
    float gyroY;
    float accelX;
    float accelY;
    float accelZ;
    bool driftEnabled;
    bool driftActive;
    float driftCompensation;
    float gyroZFiltered;
    float pseudoSpeed;
};
#endif
