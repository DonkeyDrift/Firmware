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
    bool staApplyFromAp = false;
    bool staPasswordSet = false;
    char staSsid[WIFI_STA_SSID_MAX_LEN + 1] = {0};
    char staPassword[WIFI_STA_PASSWORD_MAX_LEN + 1] = {0};
    // 一次性 AP 配网目标信道：0 表示未知/不预对齐，1..14 为本次从 scan 选中的目标路由器信道。
    // 仅运行时使用，不持久化；STA apply 前消费，成功关闭 AP 或失败恢复 AP 时清零。
    uint8_t staTargetChannel = 0;

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
    // v1.7.18 起 AP/STA 互斥切换的去抖锚点：
    // staUpGraceDeadlineMs   - STA 首次 WL_CONNECTED 后等待关 AP 的截止时间，0 表示未武装；
    // staDownGraceDeadlineMs - STA 脱离 WL_CONNECTED 后等待起 AP 的截止时间，0 表示未武装；
    // inApOnlyMode           - 当前是否按 AP-only 行为运行（STA 不活跃），用于驱动状态机分支；
    //                          底层 WiFi mode 始终为 WIFI_AP_STA，不再与该标志一一对应。
    unsigned long staUpGraceDeadlineMs = 0;
    unsigned long staDownGraceDeadlineMs = 0;
    bool inApOnlyMode = true;

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
