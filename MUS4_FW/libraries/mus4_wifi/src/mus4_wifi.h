// mus4_wifi.h - aggregate header for the MUS4 Wi-Fi library.
//
// Re-exports the public headers of the mus4_wifi library. The main sketch
// (MUS4_FW.ino) and any other mus4_* library that needs Wi-Fi state
// management, OTA, STA config, or identity can simply include
// <mus4_wifi.h>.
//
// Depends on: mus4_core, mus4_command.
//
// Individual headers remain addressable as <mus4_wifi/<Header>.h>.

#pragma once

#include "WifiManager.h"
#include "WifiOta.h"
#include "WifiStaConfig.h"
#include "WifiIdentity.h"
