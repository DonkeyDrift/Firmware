#include "WifiStaHistory.h"

#ifdef ENABLE_WIFI_CONSOLE

#include <Preferences.h>

#include "Mus4Log.h"
#include "WifiConsoleTypes.h"

// Hardware objects defined in MUS4_FW.ino
extern Preferences mus4Prefs;

// NVS 槽位键（命名空间 "mus4"，键长 <= 15 字符）：槽 0 为最近一次成功连接。
static const char* const WIFI_STA_HISTORY_SSID_KEYS[WIFI_STA_HISTORY_SIZE] = {
    "sta_h0s", "sta_h1s", "sta_h2s", "sta_h3s", "sta_h4s"
};
static const char* const WIFI_STA_HISTORY_PASS_KEYS[WIFI_STA_HISTORY_SIZE] = {
    "sta_h0p", "sta_h1p", "sta_h2p", "sta_h3p", "sta_h4p"
};

// STA 连接历史条目静态缓存：槽 0 为最近（优先级最高），与 NVS 保持同步。
struct WifiStaHistoryEntry {
    String ssid;
    String password;
};

static WifiStaHistoryEntry g_wifiStaHistory[WIFI_STA_HISTORY_SIZE];
static uint8_t g_wifiStaHistoryCount = 0;

uint8_t wifiStaHistoryCount()
{
    return g_wifiStaHistoryCount;
}

bool copyWifiStaHistorySsid(uint8_t index, String& ssidOut)
{
    if (index >= g_wifiStaHistoryCount) return false;
    ssidOut = g_wifiStaHistory[index].ssid;
    return true;
}

int8_t wifiStaHistoryRankOf(const String& ssid)
{
    if (ssid.length() == 0) return -1;
    for (uint8_t i = 0; i < g_wifiStaHistoryCount; i++) {
        if (g_wifiStaHistory[i].ssid == ssid) return (int8_t)i;
    }
    return -1;
}

bool findWifiStaHistoryEntry(const String& ssid, String& passwordOut)
{
    int8_t rank = wifiStaHistoryRankOf(ssid);
    if (rank < 0) return false;
    passwordOut = g_wifiStaHistory[rank].password;
    return true;
}

// 把缓存的历史列表整体写回 NVS：有效槽位写入 SSID/密码，其余槽位键移除，
// 避免已删除条目在下一次 load 时复活。
static bool persistWifiStaHistory()
{
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    bool ok = true;
    for (uint8_t i = 0; i < WIFI_STA_HISTORY_SIZE; i++) {
        if (i < g_wifiStaHistoryCount) {
            size_t ssidWritten = mus4Prefs.putString(WIFI_STA_HISTORY_SSID_KEYS[i], g_wifiStaHistory[i].ssid);
            size_t passWritten = mus4Prefs.putString(WIFI_STA_HISTORY_PASS_KEYS[i], g_wifiStaHistory[i].password);
            if (ssidWritten == 0 || (g_wifiStaHistory[i].password.length() > 0 && passWritten == 0)) {
                ok = false;
            }
        } else {
            mus4Prefs.remove(WIFI_STA_HISTORY_SSID_KEYS[i]);
            mus4Prefs.remove(WIFI_STA_HISTORY_PASS_KEYS[i]);
        }
    }
    mus4Prefs.end();
    return ok;
}

bool recordWifiStaHistory(const String& ssid, const String& password)
{
    if (ssid.length() == 0 || ssid.length() > WIFI_STA_SSID_MAX_LEN) return false;
    if (password.length() > WIFI_STA_PASSWORD_MAX_LEN) return false;
    if (password.length() > 0 && password.length() < WIFI_STA_PASSWORD_MIN_LEN) return false;
    int8_t existing = wifiStaHistoryRankOf(ssid);
    if (existing == 0 && g_wifiStaHistory[0].password == password) {
        // 已置顶且密码未变：无变化，避免重复 NVS 写入磨损 flash。
        return true;
    }
    if (existing >= 0) {
        // 去重：更新密码并置顶，其上条目顺移一格。
        WifiStaHistoryEntry entry = g_wifiStaHistory[existing];
        entry.password = password;
        for (int8_t i = existing; i > 0; i--) {
            g_wifiStaHistory[i] = g_wifiStaHistory[i - 1];
        }
        g_wifiStaHistory[0] = entry;
    } else {
        // 新条目置顶，其余顺移，超出容量的最旧条目（第 6 条）丢弃。
        uint8_t shiftLimit = g_wifiStaHistoryCount < WIFI_STA_HISTORY_SIZE
            ? g_wifiStaHistoryCount
            : WIFI_STA_HISTORY_SIZE - 1;
        for (int8_t i = (int8_t)shiftLimit; i > 0; i--) {
            g_wifiStaHistory[i] = g_wifiStaHistory[i - 1];
        }
        g_wifiStaHistory[0].ssid = ssid;
        g_wifiStaHistory[0].password = password;
        if (g_wifiStaHistoryCount < WIFI_STA_HISTORY_SIZE) {
            g_wifiStaHistoryCount++;
        }
    }
    return persistWifiStaHistory();
}

bool removeWifiStaHistoryEntry(const String& ssid)
{
    int8_t rank = wifiStaHistoryRankOf(ssid);
    if (rank < 0) return false;
    for (uint8_t i = (uint8_t)rank; i + 1 < g_wifiStaHistoryCount; i++) {
        g_wifiStaHistory[i] = g_wifiStaHistory[i + 1];
    }
    g_wifiStaHistoryCount--;
    g_wifiStaHistory[g_wifiStaHistoryCount].ssid = "";
    g_wifiStaHistory[g_wifiStaHistoryCount].password = "";
    return persistWifiStaHistory();
}

void clearWifiStaHistory()
{
    g_wifiStaHistoryCount = 0;
    for (uint8_t i = 0; i < WIFI_STA_HISTORY_SIZE; i++) {
        g_wifiStaHistory[i].ssid = "";
        g_wifiStaHistory[i].password = "";
    }
    persistWifiStaHistory();
}

void loadWifiStaHistory()
{
    g_wifiStaHistoryCount = 0;
    for (uint8_t i = 0; i < WIFI_STA_HISTORY_SIZE; i++) {
        g_wifiStaHistory[i].ssid = "";
        g_wifiStaHistory[i].password = "";
    }
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, true)) {
        mus4LogLine("wifi", "STA history load failed");
        return;
    }
    // 历史列表始终保持紧凑（mutation 时顺移/压实），读到首个空槽即停。
    for (uint8_t i = 0; i < WIFI_STA_HISTORY_SIZE; i++) {
        String ssid = mus4Prefs.getString(WIFI_STA_HISTORY_SSID_KEYS[i], "");
        if (ssid.length() == 0 || ssid.length() > WIFI_STA_SSID_MAX_LEN) break;
        g_wifiStaHistory[i].ssid = ssid;
        g_wifiStaHistory[i].password = mus4Prefs.getString(WIFI_STA_HISTORY_PASS_KEYS[i], "");
        g_wifiStaHistoryCount++;
    }
    bool migrated = false;
    if (g_wifiStaHistoryCount == 0) {
        // 旧单槽配置迁移：sta_en=true 且 sta_ssid 存在时迁移为历史槽 0（不删旧键）。
        bool staEnabled = mus4Prefs.getBool(MUS4_PREF_STA_ENABLED_KEY, false);
        String ssid = mus4Prefs.getString(MUS4_PREF_STA_SSID_KEY, "");
        if (staEnabled && ssid.length() > 0 && ssid.length() <= WIFI_STA_SSID_MAX_LEN) {
            g_wifiStaHistory[0].ssid = ssid;
            g_wifiStaHistory[0].password = mus4Prefs.getString(MUS4_PREF_STA_PASSWORD_KEY, "");
            g_wifiStaHistoryCount = 1;
            migrated = true;
        }
    }
    mus4Prefs.end();
    if (migrated && !persistWifiStaHistory()) {
        mus4LogLine("wifi", "STA history migrate persist failed");
    }
    mus4Logf("wifi", "STA history: count=%u", g_wifiStaHistoryCount);
}

#endif // ENABLE_WIFI_CONSOLE
