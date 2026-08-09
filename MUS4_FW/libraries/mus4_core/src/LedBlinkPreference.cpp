#include "LedBlinkPreference.h"

#include <Preferences.h>

// Runtime mirror of NVS "webui"/"ledblink"; default red+green+blue.
static uint8_t ledBlinkMask = 7;

void loadLedBlinkPreference()
{
    Preferences prefs;
    if (!prefs.begin("webui", true)) {
        ledBlinkMask = 7;
        return;
    }
    ledBlinkMask = prefs.getUChar("ledblink", 7) & 7;
    prefs.end();
}

bool saveLedBlinkPreference(uint8_t mask)
{
    mask &= 7;
    Preferences prefs;
    if (!prefs.begin("webui", false)) return false;
    size_t written = prefs.putUChar("ledblink", mask);
    prefs.end();
    if (written == 0) return false;
    ledBlinkMask = mask;
    return true;
}

uint8_t getLedBlinkMask()
{
    return ledBlinkMask;
}
