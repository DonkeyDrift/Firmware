#include "WifiStaConfig.h"

#include <WiFi.h>
#include <Preferences.h>

#include "Mus4Log.h"
#include "WifiStaHistory.h"

#ifdef ENABLE_WIFI_CONSOLE
#if __has_include("WirelessSecrets.h")
#include "WirelessSecrets.h"
#endif
#ifndef WIFI_STA_SSID
#define WIFI_STA_SSID ""
#endif
#ifndef WIFI_STA_PASSWORD
#define WIFI_STA_PASSWORD ""
#endif
static const uint8_t WIFI_STA_CONFIG_SSID_MAX_LEN = 32;
static const uint8_t WIFI_STA_CONFIG_PASSWORD_MAX_LEN = 63;
static const uint8_t WIFI_STA_CONFIG_PASSWORD_MIN_LEN = 8;
static const unsigned long WIFI_STA_CONFIG_APPLY_DELAY_MS = 800;
static const char* WIFI_STA_CONFIG_PREF_NAMESPACE = "mus4";
static const char* WIFI_STA_CONFIG_PREF_ENABLED_KEY = "sta_en";
static const char* WIFI_STA_CONFIG_PREF_SSID_KEY = "sta_ssid";
static const char* WIFI_STA_CONFIG_PREF_PASSWORD_KEY = "sta_pass";

static WifiRuntimeState* g_ws = nullptr;

extern void applyWifiStaCredentials();
extern void clearWifiStaHandoff();

void setWifiRuntimeState(WifiRuntimeState& ws)
{
    g_ws = &ws;
}

static inline WifiRuntimeState& ws()
{
    return *g_ws;
}

bool copyWifiStaSsid(const String& ssid)
{
    if (ssid.length() == 0 || ssid.length() > WIFI_STA_CONFIG_SSID_MAX_LEN) return false;
    ssid.toCharArray(ws().staSsid, WIFI_STA_CONFIG_SSID_MAX_LEN + 1);
    return true;
}

bool copyWifiStaPassword(const String& password)
{
    if (password.length() > 0 && (password.length() < WIFI_STA_CONFIG_PASSWORD_MIN_LEN || password.length() > WIFI_STA_CONFIG_PASSWORD_MAX_LEN)) return false;
    password.toCharArray(ws().staPassword, WIFI_STA_CONFIG_PASSWORD_MAX_LEN + 1);
    ws().staPasswordSet = password.length() > 0;
    return true;
}

String wifiStaIpText()
{
    return ws().staConnected ? WiFi.localIP().toString() : String("0.0.0.0");
}

String wifiApIpText()
{
    if (WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) return String("Disabled");
    return WiFi.softAPIP().toString();
}

void clearWifiStaLastError()
{
    ws().staLastError[0] = 0;
    ws().staLastErrorMessage[0] = 0;
}

void setWifiStaLastError(const char* code, const char* message, bool timedOut)
{
    // 保留本轮连接的首个失败原因，避免后续瞬态状态覆盖更有诊断价值的根因。
    if (ws().staLastError[0] != 0) return;
    snprintf(ws().staLastError, 24, "%s", code);
    snprintf(ws().staLastErrorMessage, 128, "%s", message);
    ws().staTimedOut = timedOut;
    ws().staConnecting = false;
    ws().staConnected = false;
    mus4Logf("wifi", "STA failed: %s", code);
}

void scheduleWifiStaApply()
{
    ws().staApplyPending = true;
    ws().staApplyDeadlineMs = millis() + WIFI_STA_CONFIG_APPLY_DELAY_MS;
}

bool saveWifiStaPreference(const String& ssid, const String& password)
{
    if (!copyWifiStaSsid(ssid) || !copyWifiStaPassword(password)) return false;
    if (!ws().prefs->begin(WIFI_STA_CONFIG_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = ws().prefs->putBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, true);
    size_t ssidWritten = ws().prefs->putString(WIFI_STA_CONFIG_PREF_SSID_KEY, ws().staSsid);
    size_t passwordWritten = ws().prefs->putString(WIFI_STA_CONFIG_PREF_PASSWORD_KEY, ws().staPassword);
    ws().prefs->end();
    if (enabledWritten == 0 || ssidWritten == 0 || (ws().staPasswordSet && passwordWritten == 0)) return false;
    ws().staConfigured = true;
    return true;
}

bool saveWifiStaSsidPreference(const String& ssid)
{
    if (!copyWifiStaSsid(ssid)) return false;
    if (!ws().prefs->begin(WIFI_STA_CONFIG_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = ws().prefs->putBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, true);
    size_t ssidWritten = ws().prefs->putString(WIFI_STA_CONFIG_PREF_SSID_KEY, ws().staSsid);
    ws().prefs->end();
    if (enabledWritten == 0 || ssidWritten == 0) return false;
    ws().staConfigured = true;
    return true;
}

bool saveWifiStaPasswordPreference(const String& password)
{
    if (!copyWifiStaPassword(password)) return false;
    if (!ws().prefs->begin(WIFI_STA_CONFIG_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = ws().prefs->putBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, true);
    size_t passwordWritten = ws().prefs->putString(WIFI_STA_CONFIG_PREF_PASSWORD_KEY, ws().staPassword);
    ws().prefs->end();
    if (enabledWritten == 0 || (ws().staPasswordSet && passwordWritten == 0)) return false;
    ws().staConfigured = true;
    return true;
}

void clearWifiStaRuntimeStateWithoutDisconnect()
{
    ws().staSsid[0] = 0;
    ws().staPassword[0] = 0;
    ws().staPasswordSet = false;
    ws().staConfigured = false;
    ws().staConnected = false;
    ws().staTimedOut = false;
    ws().staConnecting = false;
    clearWifiStaLastError();
    ws().staApplyPending = false;
    clearWifiStaHandoff();
}

bool clearWifiStaPreference()
{
    if (!ws().prefs->begin(WIFI_STA_CONFIG_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = ws().prefs->putBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, false);
    ws().prefs->remove(WIFI_STA_CONFIG_PREF_SSID_KEY);
    ws().prefs->remove(WIFI_STA_CONFIG_PREF_PASSWORD_KEY);
    ws().prefs->end();
    if (enabledWritten == 0) return false;
    clearWifiStaRuntimeStateWithoutDisconnect();
    clearWifiStaHistory();
    return true;
}

void loadWifiStaPreference()
{
    ws().staSsid[0] = 0;
    ws().staPassword[0] = 0;
    ws().staPasswordSet = false;
    ws().staConfigured = false;
    if (!ws().prefs->begin(WIFI_STA_CONFIG_PREF_NAMESPACE, true)) {
        copyWifiStaSsid(String(WIFI_STA_SSID));
        copyWifiStaPassword(String(WIFI_STA_PASSWORD));
        ws().staConfigured = strlen(ws().staSsid) > 0;
        mus4LogLine("wifi", "STA config load failed, using build defaults");
        return;
    }
    bool hasStaEnabled = ws().prefs->isKey(WIFI_STA_CONFIG_PREF_ENABLED_KEY);
    bool staEnabled = ws().prefs->getBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, false);
    String ssid = hasStaEnabled && staEnabled ? ws().prefs->getString(WIFI_STA_CONFIG_PREF_SSID_KEY, "") : String(WIFI_STA_SSID);
    String password = hasStaEnabled && staEnabled ? ws().prefs->getString(WIFI_STA_CONFIG_PREF_PASSWORD_KEY, "") : String(WIFI_STA_PASSWORD);
    ws().prefs->end();
    if (hasStaEnabled && !staEnabled) {
        mus4LogLine("wifi", "STA disabled by preference");
        return;
    }
    if (copyWifiStaSsid(ssid) && copyWifiStaPassword(password)) {
        ws().staConfigured = strlen(ws().staSsid) > 0;
    } else {
        ws().staSsid[0] = 0;
        ws().staPassword[0] = 0;
        ws().staPasswordSet = false;
        ws().staConfigured = false;
        mus4LogLine("wifi", "STA config invalid");
    }
}

void printWifiStaStatus(Print& out)
{
    out.printf("WIFI_STA configured=%d connected=%d timed_out=%d connecting=%d ssid=\"%s\" password_set=%d ap_ip=%s sta_ip=%s last_error=\"%s\" last_error_message=\"%s\"\n",
        ws().staConfigured ? 1 : 0,
        ws().staConnected ? 1 : 0,
        ws().staTimedOut ? 1 : 0,
        ws().staConnecting ? 1 : 0,
        ws().staSsid,
        ws().staPasswordSet ? 1 : 0,
        wifiApIpText().c_str(),
        wifiStaIpText().c_str(),
        ws().staLastError,
        ws().staLastErrorMessage);
}

bool processWifiStaConfigCommand(const String& line, Print& out, WifiRuntimeState& /*ws*/)
{
    if (line.equalsIgnoreCase("WIFI_STA_STATUS")) {
        printWifiStaStatus(out);
        return true;
    }
    if (line.startsWith("WIFI_STA_SSID:")) {
        String ssid = line.substring(14);
        ssid.trim();
        if (!saveWifiStaSsidPreference(ssid)) {
            out.println("NACK:WIFI_STA_SSID");
            return true;
        }
        out.printf("WIFI_STA_SSID_SAVED configured=%d\n", ws().staConfigured ? 1 : 0);
        return true;
    }
    if (line.startsWith("WIFI_STA_PASSWORD:")) {
        String password = line.substring(18);
        if (!saveWifiStaPasswordPreference(password)) {
            out.println("NACK:WIFI_STA_PASSWORD");
            return true;
        }
        out.printf("WIFI_STA_PASSWORD_SAVED password_set=%d\n", ws().staPasswordSet ? 1 : 0);
        return true;
    }
    if (line.equalsIgnoreCase("WIFI_STA_APPLY")) {
        if (!ws().staConfigured) {
            out.println("NACK:WIFI_STA_NOT_CONFIGURED");
            return true;
        }
        applyWifiStaCredentials();
        out.printf("WIFI_STA_APPLY_OK ssid=\"%s\"\n", ws().staSsid);
        return true;
    }
    if (line.equalsIgnoreCase("WIFI_STA_CLEAR")) {
        if (!clearWifiStaPreference()) {
            out.println("NACK:WIFI_STA_CLEAR");
            return true;
        }
        out.println("WIFI_STA_CLEARED");
        return true;
    }
    return false;
}
#endif
