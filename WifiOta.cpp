#include "WifiOta.h"

#include <ArduinoOTA.h>
#include <WiFi.h>

#include "Mus4Log.h"
#include "SharedTypes.h"

#ifdef ENABLE_WIFI_CONSOLE
static const char* WIFI_CONSOLE_AP_PASSWORD = "mus4-debug";
static const uint16_t WIFI_OTA_PORT = 3232;
static const unsigned long WIFI_OTA_WINDOW_MS = 120000UL;

extern bool wifiOtaStarted;
extern bool wifiOtaWindowOpen;
extern bool wifiOtaInProgress;
extern bool wifiOtaParkGuardActive;
extern bool wifiDevModeEnabled;
extern unsigned long wifiOtaDeadlineMs;
extern uint8_t wifiOtaLastProgressPct;
extern ControlData rc_data;
extern ControlData car_output;

extern void ensureWifiOtaStarted();

void forceWifiOtaParkLocked()
{
    rc_data.park = PARK_LOCKED;
    car_output.park = PARK_LOCKED;
    car_output.throttle = 0;
}

void keepDevModeOtaWindowActive()
{
    if (!wifiDevModeEnabled) return;
    ensureWifiOtaStarted();
    wifiOtaWindowOpen = true;
    wifiOtaDeadlineMs = millis() + WIFI_OTA_WINDOW_MS;
}

bool shouldEmitSerial1Telemetry()
{
    return !wifiOtaWindowOpen && !wifiOtaInProgress;
}

void openLocalWifiOtaWindow(const String& line, Print& out, SerialBuf& sb)
{
    if (!line.substring(11).equals(WIFI_CONSOLE_AP_PASSWORD)) {
        out.println("NACK:AUTH_REQUIRED");
        sb.errors++;
        return;
    }
    wifiOtaParkGuardActive = true;
    forceWifiOtaParkLocked();
    ensureWifiOtaStarted();
    wifiOtaWindowOpen = true;
    wifiOtaDeadlineMs = millis() + WIFI_OTA_WINDOW_MS;
    wifiOtaLastProgressPct = 0;
    out.printf("OTA_READY ip=%s port=%u ttl_ms=%lu\n", WiFi.localIP().toString().c_str(), WIFI_OTA_PORT, WIFI_OTA_WINDOW_MS);
    mus4LogLine("ota", "ready: local");
}

void closeWifiOtaWindow(const char* reason)
{
    wifiOtaWindowOpen = false;
    wifiOtaDeadlineMs = 0;
    wifiOtaInProgress = false;
    wifiOtaParkGuardActive = false;
    wifiOtaLastProgressPct = 0;
    if (wifiOtaStarted) {
        ArduinoOTA.end();
        wifiOtaStarted = false;
    }
    mus4LogLine("ota", String("closed: ") + reason);
}

unsigned long wifiOtaTtlMs()
{
    if (!wifiOtaWindowOpen) return 0;
    if (wifiDevModeEnabled) return WIFI_OTA_WINDOW_MS;
    unsigned long now = millis();
    if ((long)(wifiOtaDeadlineMs - now) <= 0) return 0;
    return wifiOtaDeadlineMs - now;
}

void printWifiOtaStatus(Print& out)
{
    out.printf("OTA_STATUS started=%d window=%d in_progress=%d ttl_ms=%lu progress=%u park=%d dev_mode=%d park_guard=%d\n",
        wifiOtaStarted ? 1 : 0,
        wifiOtaWindowOpen ? 1 : 0,
        wifiOtaInProgress ? 1 : 0,
        wifiOtaTtlMs(),
        wifiOtaLastProgressPct,
        car_output.park ? 1 : 0,
        wifiDevModeEnabled ? 1 : 0,
        wifiOtaParkGuardActive ? 1 : 0);
}
#endif