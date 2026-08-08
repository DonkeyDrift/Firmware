#pragma once

// System-wide sound mute preference (v1.7.46).
//
// Single source of truth for the Web Console mute toggle, shared by every
// firmware sound producer (the GPIO2 Buzzer melodies: mode change, Park
// lock/unlock, Wi-Fi AP/STA events). Persisted in NVS namespace "webui",
// key "muted" (UChar 0/1); default 0 (unmuted) when the key is absent, and
// the choice is restored after power off / reboot.
//
// loadMutePreference() must run early in setup(), before setupWifiConsole()
// can play the AP start melody, so the boot chime is already gated. The
// Buzzer checks isSystemMuted() at every sound start, so a POST /api/mute
// toggle takes effect immediately, without a restart.

void loadMutePreference();
bool saveMutePreference(bool muted);
bool isSystemMuted();
