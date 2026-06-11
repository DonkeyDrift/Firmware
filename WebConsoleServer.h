#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE

// Register all HTTP routes for the Web Console and start the WebServer.
void setupWebConsoleServer();

// Process one update tick for the Web Console: sample telemetry data,
// handle pending HTTP clients, and record handler timing statistics.
// Does NOT handle WebSocket (see WebTelemetry), AP restart, or DNS server.
void updateWebConsoleServer();

#endif
