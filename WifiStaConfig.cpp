#include "WifiStaConfig.h"

#include <WiFi.h>

#include "Mus4Log.h"

#ifdef ENABLE_WIFI_CONSOLE
static const uint8_t WIFI_STA_CONFIG_SSID_MAX_LEN = 32;
static const uint8_t WIFI_STA_CONFIG_PASSWORD_MAX_LEN = 63;
static const uint8_t WIFI_STA_CONFIG_PASSWORD_MIN_LEN = 8;
static const unsigned long WIFI_STA_CONFIG_APPLY_DELAY_MS = 800;

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

extern bool saveWifiStaSsidPreference(const String& ssid);
extern bool saveWifiStaPasswordPreference(const String& password);
extern void applyWifiStaCredentials();
extern bool clearWifiStaPreference();

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
