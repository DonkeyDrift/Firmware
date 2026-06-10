#include "WifiOta.h"

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
extern ControlData car_output;

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