// mus4_command.h - aggregate header for the MUS4 command-processing library.
//
// Re-exports the public headers of the mus4_command library. The main sketch
// (MUS4_FW.ino) and any other mus4_* library that needs command parsing,
// dispatch, or line buffering can simply include <mus4_command.h>.
//
// Depends on: mus4_core (for FirmwareConfig.h, SharedTypes.h,
// RuntimeState.h, SerialBufferTypes.h).
//
// Individual headers remain addressable as <mus4_command/<Header>.h>.

#pragma once

#include "CommandParser.h"
#include "CommandDispatcher.h"
#include "LocalCommands.h"
#include "WirelessConsole.h"
#include "SerialLineReader.h"
