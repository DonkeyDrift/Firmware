#include "WifiStaConfig.h"

#include <WiFi.h>

#ifdef ENABLE_WIFI_CONSOLE
extern bool wifiStaConfigured;
extern bool wifiStaConnected;
extern bool wifiStaTimedOut;
extern bool wifiStaConnecting;
extern bool wifiStaPasswordSet;
extern char wifiStaSsid[];
extern char wifiStaLastError[];
extern char wifiStaLastErrorMessage[];

extern String wifiStaIpText();
extern bool saveWifiStaSsidPreference(const String& ssid);
extern bool saveWifiStaPasswordPreference(const String& password);
extern void applyWifiStaCredentials();
extern bool clearWifiStaPreference();

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
