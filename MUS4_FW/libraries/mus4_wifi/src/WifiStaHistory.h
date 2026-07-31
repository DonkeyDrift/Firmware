#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE

// STA 连接历史：记录最近成功连接的 5 个 WiFi（SSID+密码，NVS 掉电保留），
// 槽 0 为最近一次成功连接（优先级最高）。NVS 命名空间 "mus4"，
// 槽位键 sta_h{0..4}s / sta_h{0..4}p；所有 mutation 立即持久化。

uint8_t wifiStaHistoryCount();
bool copyWifiStaHistorySsid(uint8_t index, String& ssidOut);
bool findWifiStaHistoryEntry(const String& ssid, String& passwordOut);
int8_t wifiStaHistoryRankOf(const String& ssid);
bool recordWifiStaHistory(const String& ssid, const String& password);
bool removeWifiStaHistoryEntry(const String& ssid);
void clearWifiStaHistory();
void loadWifiStaHistory();
#else
inline uint8_t wifiStaHistoryCount()
{
    return 0;
}
inline bool copyWifiStaHistorySsid(uint8_t index, String& ssidOut)
{
    (void)index;
    (void)ssidOut;
    return false;
}
inline bool findWifiStaHistoryEntry(const String& ssid, String& passwordOut)
{
    (void)ssid;
    (void)passwordOut;
    return false;
}
inline int8_t wifiStaHistoryRankOf(const String& ssid)
{
    (void)ssid;
    return -1;
}
inline bool recordWifiStaHistory(const String& ssid, const String& password)
{
    (void)ssid;
    (void)password;
    return false;
}
inline bool removeWifiStaHistoryEntry(const String& ssid)
{
    (void)ssid;
    return false;
}
inline void clearWifiStaHistory() {}
inline void loadWifiStaHistory() {}
#endif
