#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE
#include "WifiConsoleTypes.h"

// Aggregated Wi-Fi runtime state to replace scattered extern bool/char variables
// in MUS4_FW.ino. This structure is owned by the main sketch and passed by
// reference into the wireless/STA/OTA modules.
struct WifiRuntimeState {
    // Console authentication
    bool consoleStarted = false;
    bool consoleAuthenticated = false;

    // STA configuration
    bool staConfigured = false;
    bool staConnected = false;
    bool staTimedOut = false;
    bool staConnecting = false;
    char staLastError[24] = {0};
    char staLastErrorMessage[128] = {0};
    bool staApplyPending = false;
    bool staPasswordSet = false;
    char staSsid[WIFI_STA_SSID_MAX_LEN + 1] = {0};
    char staPassword[WIFI_STA_PASSWORD_MAX_LEN + 1] = {0};

    // AP / mDNS / handoff
    bool apRestartPending = false;
    bool mdnsStarted = false;
    bool staHandoffActive = false;
    char staHandoffTargetSsid[WIFI_STA_SSID_MAX_LEN + 1] = {0};
    char staHandoffStaIp[16] = {0};
    char staHandoffApSsid[WIFI_AP_SSID_MAX_LEN + 1] = {0};
    unsigned long staHandoffStartedMs = 0;
    char apSsid[WIFI_AP_SSID_MAX_LEN + 1] = {0};

    // Dev mode
    bool devModeEnabled = false;

    // Timing
    unsigned long staConnectStartMs = 0;
    unsigned long staApplyDeadlineMs = 0;
    unsigned long apRestartDeadlineMs = 0;
    unsigned long lastConsoleStartAttemptMs = 0;

    // Shared Preferences instance (pointer to the global Preferences in MUS4_FW.ino)
    Preferences* prefs = nullptr;
};

// Aggregated OTA runtime state to replace scattered extern OTA variables.
struct OtaRuntimeState {
    bool started = false;
    bool windowOpen = false;
    bool inProgress = false;
    bool parkGuardActive = false;
    unsigned long deadlineMs = 0;
    uint8_t lastProgressPct = 0;
};

#endif // ENABLE_WIFI_CONSOLE
