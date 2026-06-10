#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE
const char* WIFI_CONSOLE_AP_DEFAULT_SSID = "MUS4-DEBUG";
const char* WIFI_CONSOLE_AP_PASSWORD = "mus4-debug";
const uint16_t WIFI_CONSOLE_PORT = 2323;
const uint16_t WIFI_WEB_CONSOLE_PORT = 80;
const uint32_t WIFI_WEB_TELEMETRY_MIN_FREE_HEAP = 60000;
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
const uint16_t WIFI_WEB_SOCKET_PORT = 81;
const unsigned long WIFI_WEB_SOCKET_PUSH_INTERVAL_MS = 16;
const uint8_t WIFI_WEB_SOCKET_MAX_POINTS_PER_FRAME = 8;
const uint8_t WIFI_WEB_SOCKET_MAX_REPLAY_POINTS = 32;
const uint16_t WIFI_WEB_SOCKET_KEEPALIVE_SECONDS = 60;
#endif
const uint8_t WIFI_CONSOLE_CHANNEL = 6;
const uint8_t WIFI_CONSOLE_MAX_CLIENTS = 1;
const unsigned long WIFI_CONSOLE_RETRY_INTERVAL_MS = 5000;
const unsigned long WIFI_STA_CONNECT_TIMEOUT_MS = 15000;
const unsigned long WIFI_STA_APPLY_DELAY_MS = 800;
const char* WIFI_OTA_HOSTNAME = "mus4-ota";
const char* WIFI_OTA_PASSWORD = "mus4-debug";
const uint16_t WIFI_OTA_PORT = 3232;
const unsigned long WIFI_OTA_WINDOW_MS = 120000UL;
const char* MUS4_PREF_NAMESPACE = "mus4";
const char* MUS4_PREF_DEV_MODE_KEY = "dev_mode";
const char* MUS4_PREF_AP_SSID_KEY = "ap_ssid";
const char* MUS4_PREF_STA_ENABLED_KEY = "sta_en";
const char* MUS4_PREF_STA_SSID_KEY = "sta_ssid";
const char* MUS4_PREF_STA_PASSWORD_KEY = "sta_pass";
const char* MUS4_PREF_STEER_MIN_KEY = "str_min";
const char* MUS4_PREF_STEER_MID_KEY = "str_mid";
const char* MUS4_PREF_STEER_MAX_KEY = "str_max";
const char* MUS4_PREF_STEER_CAL_EN_KEY = "str_cal";
const uint8_t WIFI_AP_SSID_MAX_LEN = 32;
const uint8_t WIFI_STA_SSID_MAX_LEN = 32;
const uint8_t WIFI_STA_PASSWORD_MAX_LEN = 63;
const uint8_t WIFI_STA_PASSWORD_MIN_LEN = 8;
const uint8_t WIFI_WEB_LOG_CAPACITY = 64;
const uint16_t WIFI_WEB_DATA_CAPACITY = 256;
const unsigned long WIFI_WEB_DATA_INTERVAL_MS = 16;

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
