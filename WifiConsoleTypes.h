#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE
// Use static linkage to avoid ODR violations when this header is included by
// multiple translation units.
static const char* WIFI_CONSOLE_AP_DEFAULT_SSID = "MUS4-ESP";
static const char* WIFI_CONSOLE_AP_PASSWORD = "mus4-debug";
static const char* WIFI_AP_SSID_SUFFIX = "-ESP";
static const uint8_t WIFI_AP_SSID_PREFIX_MAX_LEN = 8;
static const uint16_t WIFI_CONSOLE_PORT = 2323;
static const uint16_t WIFI_WEB_CONSOLE_PORT = 80;
static const uint32_t WIFI_WEB_TELEMETRY_MIN_FREE_HEAP = 60000;
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
static const uint16_t WIFI_WEB_SOCKET_PORT = 81;
static const unsigned long WIFI_WEB_SOCKET_PUSH_INTERVAL_MS = 16;
static const uint8_t WIFI_WEB_SOCKET_MAX_POINTS_PER_FRAME = 8;
static const uint8_t WIFI_WEB_SOCKET_MAX_REPLAY_POINTS = 32;
static const uint16_t WIFI_WEB_SOCKET_KEEPALIVE_SECONDS = 60;
#endif
static const uint8_t WIFI_CONSOLE_CHANNEL = 6;
static const uint8_t WIFI_CONSOLE_MAX_CLIENTS = 1;
static const unsigned long WIFI_CONSOLE_RETRY_INTERVAL_MS = 5000;
static const unsigned long WIFI_STA_CONNECT_TIMEOUT_MS = 15000;
static const unsigned long WIFI_STA_APPLY_DELAY_MS = 800;
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
static const uint8_t WIFI_AP_SSID_MAX_LEN = 32;
static const uint8_t WIFI_STA_SSID_MAX_LEN = 32;
static const uint8_t WIFI_STA_PASSWORD_MAX_LEN = 63;
static const uint8_t WIFI_STA_PASSWORD_MIN_LEN = 8;
static const uint8_t WIFI_WEB_LOG_CAPACITY = 64;
static const uint16_t WIFI_WEB_DATA_CAPACITY = 256;
static const unsigned long WIFI_WEB_DATA_INTERVAL_MS = 16;

struct WebLogEntry { uint32_t seq; unsigned long t; char source[8]; char line[160]; };
struct WifiScanEntry { char ssid[WIFI_STA_SSID_MAX_LEN + 1]; int32_t rssi; int32_t channel; bool secure; };
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
    bool driftEnabled;
    bool driftActive;
    float driftCompensation;
    float gyroZFiltered;
};
#endif
