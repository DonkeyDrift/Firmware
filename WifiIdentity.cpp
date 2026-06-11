#include "WifiIdentity.h"

#ifdef ENABLE_WIFI_CONSOLE
#include "WifiConsoleTypes.h"

static const uint8_t WIFI_IDENTITY_AP_SSID_MAX_LEN = 32;

static WifiRuntimeState* g_ws = nullptr;

void setWifiIdentityRuntimeState(WifiRuntimeState& ws)
{
    g_ws = &ws;
}

static inline char* apSsid()
{
    return g_ws ? g_ws->apSsid : nullptr;
}

bool isMdnsSafeHostnameChar(char c)
{
    return (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') ||
        c == '-';
}

bool isMdnsSafeHostname(const String& value)
{
    if (value.length() == 0 || value.length() > WIFI_IDENTITY_AP_SSID_MAX_LEN) return false;
    if (value[0] == '-' || value[value.length() - 1] == '-') return false;
    for (uint8_t i = 0; i < value.length(); i++) {
        if (!isMdnsSafeHostnameChar(value[i])) return false;
    }
    return true;
}

bool isValidApSsidPrefix(const String& value)
{
    if (value.length() == 0 || value.length() > WIFI_AP_SSID_PREFIX_MAX_LEN) return false;
    for (uint8_t i = 0; i < value.length(); i++) {
        char c = value[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) {
            return false;
        }
    }
    return true;
}

bool copyWifiApSsid(const String& ssid)
{
    if (!isMdnsSafeHostname(ssid)) return false;
    char* s = apSsid();
    if (!s) return false;
    ssid.toCharArray(s, WIFI_IDENTITY_AP_SSID_MAX_LEN + 1);
    return true;
}

String wifiMdnsHostText()
{
    char* s = apSsid();
    String host = s ? String(s) : String("");
    host.toLowerCase();
    return host;
}

String wifiMdnsUrlText()
{
    return String("http://") + wifiMdnsHostText() + ".local/";
}
#endif
