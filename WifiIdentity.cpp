#include "WifiIdentity.h"

#ifdef ENABLE_WIFI_CONSOLE
static const uint8_t WIFI_IDENTITY_AP_SSID_MAX_LEN = 32;

extern char wifiApSsid[];

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

bool copyWifiApSsid(const String& ssid)
{
    if (!isMdnsSafeHostname(ssid)) return false;
    ssid.toCharArray(wifiApSsid, WIFI_IDENTITY_AP_SSID_MAX_LEN + 1);
    return true;
}

String wifiMdnsHostText()
{
    String host = String(wifiApSsid);
    host.toLowerCase();
    return host;
}

String wifiMdnsUrlText()
{
    return String("http://") + wifiMdnsHostText() + ".local/";
}
#endif
