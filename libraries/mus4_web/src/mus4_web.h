// mus4_web.h - aggregate header for the MUS4 Web Console library.
//
// Re-exports the public headers of the mus4_web library. The main sketch
// (MUS4_FW.ino) and any other mus4_* library that needs HTTP routes,
// WebSocket telemetry, or web log buffering can simply include
// <mus4_web.h>.
//
// Depends on: mus4_core, mus4_log, mus4_wifi, mus4_command.
//
// Individual headers remain addressable as <mus4_web/<Header>.h>.

#pragma once

#include "WebConsoleServer.h"
#include "WebTelemetry.h"
#include "WebLogBuffer.h"
#include "WebConsoleAssets.h"
