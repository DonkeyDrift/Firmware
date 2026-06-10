#include "WifiStaConfig.h"

#include <WiFi.h>
#include <Preferences.h>

#include "Mus4Log.h"

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

extern bool wifiStaConfigured;
extern bool wifiStaConnected;
extern bool wifiStaTimedOut;
extern bool wifiStaConnecting;
extern bool wifiStaPasswordSet;
extern bool wifiStaApplyPending;
extern char wifiStaSsid[];
extern char wifiStaPassword[];
extern char wifiStaLastError[];
extern char wifiStaLastErrorMessage[];
extern unsigned long wifiStaApplyDeadlineMs;
extern Preferences mus4Prefs;

extern void applyWifiStaCredentials();
extern void clearWifiStaHandoff();

bool copyWifiStaSsid(const String& ssid)
{
    if (ssid.length() == 0 || ssid.length() > WIFI_STA_CONFIG_SSID_MAX_LEN) return false;
    ssid.toCharArray(wifiStaSsid, WIFI_STA_CONFIG_SSID_MAX_LEN + 1);
    return true;
}

bool copyWifiStaPassword(const String& password)
{
    if (password.length() > 0 && (password.length() < WIFI_STA_CONFIG_PASSWORD_MIN_LEN || password.length() > WIFI_STA_CONFIG_PASSWORD_MAX_LEN)) return false;
    password.toCharArray(wifiStaPassword, WIFI_STA_CONFIG_PASSWORD_MAX_LEN + 1);
    wifiStaPasswordSet = password.length() > 0;
    return true;
}

String wifiStaIpText()
{
    return wifiStaConnected ? WiFi.localIP().toString() : String("0.0.0.0");
}

void clearWifiStaLastError()
{
    wifiStaLastError[0] = 0;
    wifiStaLastErrorMessage[0] = 0;
}

void setWifiStaLastError(const char* code, const char* message, bool timedOut)
{
    // 保留本轮连接的首个失败原因，避免后续瞬态状态覆盖更有诊断价值的根因。
    if (wifiStaLastError[0] != 0) return;
    snprintf(wifiStaLastError, 24, "%s", code);
    snprintf(wifiStaLastErrorMessage, 128, "%s", message);
    wifiStaTimedOut = timedOut;
    wifiStaConnecting = false;
    wifiStaConnected = false;
    mus4Logf("wifi", "STA failed: %s", code);
}

void scheduleWifiStaApply()
{
    wifiStaApplyPending = true;
    wifiStaApplyDeadlineMs = millis() + WIFI_STA_CONFIG_APPLY_DELAY_MS;
}

bool saveWifiStaPreference(const String& ssid, const String& password)
{
    if (!copyWifiStaSsid(ssid) || !copyWifiStaPassword(password)) return false;
    if (!mus4Prefs.begin(WIFI_STA_CONFIG_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = mus4Prefs.putBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, true);
    size_t ssidWritten = mus4Prefs.putString(WIFI_STA_CONFIG_PREF_SSID_KEY, wifiStaSsid);
    size_t passwordWritten = mus4Prefs.putString(WIFI_STA_CONFIG_PREF_PASSWORD_KEY, wifiStaPassword);
    mus4Prefs.end();
    if (enabledWritten == 0 || ssidWritten == 0 || (wifiStaPasswordSet && passwordWritten == 0)) return false;
    wifiStaConfigured = true;
    return true;
}

bool saveWifiStaSsidPreference(const String& ssid)
{
    if (!copyWifiStaSsid(ssid)) return false;
    if (!mus4Prefs.begin(WIFI_STA_CONFIG_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = mus4Prefs.putBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, true);
    size_t ssidWritten = mus4Prefs.putString(WIFI_STA_CONFIG_PREF_SSID_KEY, wifiStaSsid);
    mus4Prefs.end();
    if (enabledWritten == 0 || ssidWritten == 0) return false;
    wifiStaConfigured = true;
    return true;
}

bool saveWifiStaPasswordPreference(const String& password)
{
    if (!copyWifiStaPassword(password)) return false;
    if (!mus4Prefs.begin(WIFI_STA_CONFIG_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = mus4Prefs.putBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, true);
    size_t passwordWritten = mus4Prefs.putString(WIFI_STA_CONFIG_PREF_PASSWORD_KEY, wifiStaPassword);
    mus4Prefs.end();
    if (enabledWritten == 0 || (wifiStaPasswordSet && passwordWritten == 0)) return false;
    return true;
}

void clearWifiStaRuntimeStateWithoutDisconnect()
{
    wifiStaSsid[0] = 0;
    wifiStaPassword[0] = 0;
    wifiStaPasswordSet = false;
    wifiStaConfigured = false;
    wifiStaConnected = false;
    wifiStaTimedOut = false;
    wifiStaConnecting = false;
    clearWifiStaLastError();
    wifiStaApplyPending = false;
    clearWifiStaHandoff();
}

bool clearWifiStaPreference()
{
    if (!mus4Prefs.begin(WIFI_STA_CONFIG_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = mus4Prefs.putBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, false);
    mus4Prefs.remove(WIFI_STA_CONFIG_PREF_SSID_KEY);
    mus4Prefs.remove(WIFI_STA_CONFIG_PREF_PASSWORD_KEY);
    mus4Prefs.end();
    if (enabledWritten == 0) return false;
    clearWifiStaRuntimeStateWithoutDisconnect();
    return true;
}

void loadWifiStaPreference()
{
    wifiStaSsid[0] = 0;
    wifiStaPassword[0] = 0;
    wifiStaPasswordSet = false;
    wifiStaConfigured = false;
    if (!mus4Prefs.begin(WIFI_STA_CONFIG_PREF_NAMESPACE, true)) {
        copyWifiStaSsid(String(WIFI_STA_SSID));
        copyWifiStaPassword(String(WIFI_STA_PASSWORD));
        wifiStaConfigured = strlen(wifiStaSsid) > 0;
        mus4LogLine("wifi", "STA config load failed, using build defaults");
        return;
    }
    bool hasStaEnabled = mus4Prefs.isKey(WIFI_STA_CONFIG_PREF_ENABLED_KEY);
    bool staEnabled = mus4Prefs.getBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, false);
    String ssid = hasStaEnabled && staEnabled ? mus4Prefs.getString(WIFI_STA_CONFIG_PREF_SSID_KEY, "") : String(WIFI_STA_SSID);
    String password = hasStaEnabled && staEnabled ? mus4Prefs.getString(WIFI_STA_CONFIG_PREF_PASSWORD_KEY, "") : String(WIFI_STA_PASSWORD);
    mus4Prefs.end();
    if (hasStaEnabled && !staEnabled) {
        mus4LogLine("wifi", "STA disabled by preference");
        return;
    }
    if (copyWifiStaSsid(ssid) && copyWifiStaPassword(password)) {
        wifiStaConfigured = strlen(wifiStaSsid) > 0;
    } else {
        wifiStaSsid[0] = 0;
        wifiStaPassword[0] = 0;
        wifiStaPasswordSet = false;
        wifiStaConfigured = false;
        mus4LogLine("wifi", "STA config invalid");
    }
}

void printWifiStaStatus(Print& out)
{
    out.printf("WIFI_STA configured=%d connected=%d timed_out=%d connecting=%d ssid=\"%s\" password_set=%d ap_ip=%s sta_ip=%s last_error=\"%s\" last_error_message=\"%s\"\n",
        wifiStaConfigured ? 1 : 0,
        wifiStaConnected ? 1 : 0,
        wifiStaTimedOut ? 1 : 0,
        wifiStaConnecting ? 1 : 0,
        wifiStaSsid,
        wifiStaPasswordSet ? 1 : 0,
        WiFi.softAPIP().toString().c_str(),
        wifiStaIpText().c_str(),
        wifiStaLastError,
        wifiStaLastErrorMessage);
}

bool processWifiStaConfigCommand(const String& line, Print& out)
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
        out.printf("WIFI_STA_SSID_SAVED configured=%d\n", wifiStaConfigured ? 1 : 0);
        return true;
    }
    if (line.startsWith("WIFI_STA_PASSWORD:")) {
        String password = line.substring(18);
        if (!saveWifiStaPasswordPreference(password)) {
            out.println("NACK:WIFI_STA_PASSWORD");
            return true;
        }
        out.printf("WIFI_STA_PASSWORD_SAVED password_set=%d\n", wifiStaPasswordSet ? 1 : 0);
        return true;
    }
    if (line.equalsIgnoreCase("WIFI_STA_APPLY")) {
        if (!wifiStaConfigured) {
            out.println("NACK:WIFI_STA_NOT_CONFIGURED");
            return true;
        }
        applyWifiStaCredentials();
        out.printf("WIFI_STA_APPLY_OK ssid=\"%s\"\n", wifiStaSsid);
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
