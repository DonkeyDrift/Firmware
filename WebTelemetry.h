#pragma once

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// WebSocket server objects
extern AsyncWebServer wifiWebSocketServer;
extern AsyncWebSocket wifiWebSocket;

// WebSocket telemetry state (extern declarations)
extern bool wifiWebSocketClientConnected;
extern uint32_t wifiWebSocketClientId;
extern AsyncWebSocketClient* wifiWebSocketClient;
extern uint32_t wifiWebSocketClientLastSeq;
extern uint32_t wifiWebSocketDroppedPoints;
extern uint32_t wifiWebSocketQueueFullSkips;
extern uint32_t wifiWebSocketHeapSkips;
extern uint32_t wifiWebSocketFramesSent;
extern uint32_t wifiWebSocketMaxBacklog;
extern uint32_t wifiWebSocketConnects;
extern uint32_t wifiWebSocketDisconnects;
extern uint32_t wifiWebSocketMaxDtMs;

void setupWifiWebSocket();
void updateWifiWebSocket();

#endif // ENABLE_WIFI_WEBSOCKET_TELEMETRY
