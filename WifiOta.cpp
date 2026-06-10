#include "WifiOta.h"

#include <ArduinoOTA.h>

#include "Mus4Log.h"
#include "SharedTypes.h"

#ifdef ENABLE_WIFI_CONSOLE
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