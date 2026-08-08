#include "MutePreference.h"

#include <Preferences.h>

// Runtime mirror of NVS "webui"/"muted"; default unmuted.
static bool systemMuted = false;

void loadMutePreference()
{
    Preferences prefs;
    if (!prefs.begin("webui", true)) {
        systemMuted = false;
        return;
    }
    systemMuted = prefs.getUChar("muted", 0) != 0;
    prefs.end();
}

bool saveMutePreference(bool muted)
{
    Preferences prefs;
    if (!prefs.begin("webui", false)) return false;
    size_t written = prefs.putUChar("muted", muted ? 1 : 0);
    prefs.end();
    if (written == 0) return false;
    systemMuted = muted;
    return true;
}

bool isSystemMuted()
{
    return systemMuted;
}
