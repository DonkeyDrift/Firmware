#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE
bool isMdnsSafeHostnameChar(char c);
bool isMdnsSafeHostname(const String& value);
bool copyWifiApSsid(const String& ssid);
String wifiMdnsHostText();
String wifiMdnsUrlText();
#endif
