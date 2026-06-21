// mus4_core.h - aggregate header for the MUS4 core library.
//
// Re-exports the public headers of the MUS4 core library. The main sketch
// (MUS4_FW.ino) and any other mus4_* library that needs foundational types,
// configuration, or runtime state can simply include <mus4_core.h>.
//
// Individual headers remain addressable as <mus4_core/<Header>.h>.

#pragma once

#include "BuildInfo.h"
#include "FirmwareConfig.h"
#include "SharedTypes.h"
#include "RuntimeState.h"
#include "WifiConsoleTypes.h"
#include "SerialBufferTypes.h"
#include "StringPrint.h"
#include "WirelessSecrets.example.h"
