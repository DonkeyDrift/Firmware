#include "WebTelemetry.h"

#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY

#include "Mus4Log.h"
#include "RuntimeState.h"
#include "SharedTypes.h"
#include "WifiConsoleTypes.h"
#include <WiFi.h>

// External shared web data buffer (defined in MUS4_FW.ino)
extern WebDataPoint wifiWebData[];
extern uint16_t wifiWebDataHead;
extern uint16_t wifiWebDataCount;
extern uint32_t wifiWebDataSeq;

// External runtime state
extern WifiRuntimeState wifiRuntime;
extern OtaRuntimeState otaRuntime;

// WebSocket server instances
AsyncWebServer wifiWebSocketServer(WIFI_WEB_SOCKET_PORT);
AsyncWebSocket wifiWebSocket("/");

// WebSocket telemetry state
bool wifiWebSocketClientConnected = false;
uint32_t wifiWebSocketClientId = 0;
AsyncWebSocketClient* wifiWebSocketClient = nullptr;
uint32_t wifiWebSocketClientLastSeq = 0;
uint32_t wifiWebSocketDroppedPoints = 0;
uint32_t wifiWebSocketQueueFullSkips = 0;
uint32_t wifiWebSocketHeapSkips = 0;
uint32_t wifiWebSocketFramesSent = 0;
uint32_t wifiWebSocketMaxBacklog = 0;
uint32_t wifiWebSocketConnects = 0;
uint32_t wifiWebSocketDisconnects = 0;
uint32_t wifiWebSocketMaxDtMs = 0;

static unsigned long lastWifiWebSocketPushMs = 0;
static String wifiWebSocketPayload;
static uint8_t wifiWebSocketBinaryPayload[256];

static uint16_t wifiWebDataIndexForSeq(uint32_t seq)
{
    for (uint16_t i = 0; i < wifiWebDataCount; i++) {
        uint16_t index = (wifiWebDataHead + WIFI_WEB_DATA_CAPACITY - wifiWebDataCount + i) % WIFI_WEB_DATA_CAPACITY;
        if (wifiWebData[index].seq == seq) return index;
    }
    return WIFI_WEB_DATA_CAPACITY;
}

static void sendWifiWebSocketHello(AsyncWebSocketClient* client)
{
    if (!client) return;
    wifiWebSocketPayload = "{\"type\":\"hello\",\"seq\":";
    wifiWebSocketPayload += wifiWebDataSeq;
    wifiWebSocketPayload += '}';
    client->text(wifiWebSocketPayload);
}

static void handleWifiWebSocketMessage(AsyncWebSocketClient* client, uint8_t* data, size_t length)
{
    if (!client || !wifiWebSocketClientConnected || wifiWebSocketClientId != client->id()) return;
    String message;
    message.reserve(length + 1);
    for (size_t i = 0; i < length; i++) message += (char)data[i];
    message.trim();
    if (message.startsWith("since:")) {
        uint32_t seq = (uint32_t)message.substring(6).toInt();
        uint32_t replayFloor = wifiWebDataSeq > WIFI_WEB_SOCKET_MAX_REPLAY_POINTS ? wifiWebDataSeq - WIFI_WEB_SOCKET_MAX_REPLAY_POINTS : 0;
        if (seq >= replayFloor && seq <= wifiWebDataSeq) wifiWebSocketClientLastSeq = seq;
    }
}

static void handleWifiWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t length)
{
    (void)server;
    if (type == WS_EVT_CONNECT) {
        if (wifiWebSocketClientConnected && wifiWebSocketClientId != client->id()) {
            client->close();
            return;
        }
        wifiWebSocketClientConnected = true;
        wifiWebSocketClientId = client->id();
        wifiWebSocketClient = client;
        wifiWebSocketClientLastSeq = wifiWebDataSeq;
        wifiWebSocketConnects++;
        client->keepAlivePeriod(WIFI_WEB_SOCKET_KEEPALIVE_SECONDS);
        client->setCloseClientOnQueueFull(false);
        sendWifiWebSocketHello(client);
        mus4LogLine("web", "ws connected");
        return;
    }
    if (type == WS_EVT_DISCONNECT) {
        if (wifiWebSocketClientConnected && wifiWebSocketClientId == client->id()) {
            wifiWebSocketClientConnected = false;
            wifiWebSocketClient = nullptr;
            wifiWebSocketClientLastSeq = wifiWebDataSeq;
            wifiWebSocketDisconnects++;
            lastWifiWebSocketPushMs = millis();
            mus4LogLine("web", "ws disconnected");
        }
        return;
    }
    if (type == WS_EVT_DATA) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (info && info->final && info->index == 0 && info->len == length && info->opcode == WS_TEXT) {
            handleWifiWebSocketMessage(client, data, length);
        }
    }
}

static void pushWifiWebSocketData()
{
    if (!wifiWebSocketClientConnected) return;
    if (!wifiWebSocketClient || !wifiWebSocketClient->canSend() || wifiWebSocketClient->queueIsFull()) {
        wifiWebSocketQueueFullSkips++;
        return;
    }
    if (!wifiWebSocket.availableForWrite(wifiWebSocketClientId)) {
        wifiWebSocketQueueFullSkips++;
        return;
    }
    if (otaRuntime.inProgress) return;
    if (ESP.getFreeHeap() < WIFI_WEB_TELEMETRY_MIN_FREE_HEAP) {
        wifiWebSocketHeapSkips++;
        return;
    }
    unsigned long now = millis();
    if (now - lastWifiWebSocketPushMs < WIFI_WEB_SOCKET_PUSH_INTERVAL_MS) return;
    if (wifiWebDataCount == 0 || wifiWebDataSeq <= wifiWebSocketClientLastSeq) return;
    uint32_t firstSeq = wifiWebSocketClientLastSeq + 1;
    uint32_t oldestSeq = wifiWebDataSeq - wifiWebDataCount + 1;
    if (firstSeq < oldestSeq) {
        wifiWebSocketDroppedPoints += oldestSeq - firstSeq;
        firstSeq = oldestSeq;
    }
    uint32_t available = wifiWebDataSeq - firstSeq + 1;
    if (available > wifiWebSocketMaxBacklog) wifiWebSocketMaxBacklog = available;
    if (available > WIFI_WEB_SOCKET_MAX_POINTS_PER_FRAME) {
        uint32_t skipped = available - WIFI_WEB_SOCKET_MAX_POINTS_PER_FRAME;
        wifiWebSocketDroppedPoints += skipped;
        firstSeq += skipped;
        available = WIFI_WEB_SOCKET_MAX_POINTS_PER_FRAME;
    }
    uint8_t* cursor = wifiWebSocketBinaryPayload;
    auto writeU8 = [&](uint8_t value) { *cursor++ = value; };
    auto writeU16 = [&](uint16_t value) { memcpy(cursor, &value, sizeof(value)); cursor += sizeof(value); };
    auto writeU32 = [&](uint32_t value) { memcpy(cursor, &value, sizeof(value)); cursor += sizeof(value); };
    auto writeI16 = [&](int16_t value) { memcpy(cursor, &value, sizeof(value)); cursor += sizeof(value); };
    auto writeF32 = [&](float value) { memcpy(cursor, &value, sizeof(value)); cursor += sizeof(value); };
    uint8_t pointCount = 0;
    uint32_t lastSentSeq = wifiWebSocketClientLastSeq;
    uint16_t latestIndex = (wifiWebDataHead + WIFI_WEB_DATA_CAPACITY - 1) % WIFI_WEB_DATA_CAPACITY;
    WebDataPoint& latest = wifiWebData[latestIndex];
    writeU8('M');
    writeU8('4');
    writeU8(1);
    writeU8(0);
    writeU32(wifiWebSocketDroppedPoints);
    writeU32(latest.seq);
    writeU32((uint32_t)latest.t);
    writeU16(latest.dtMs);
    writeI16((int16_t)latest.throttle);
    writeI16((int16_t)latest.steering);
    writeF32(latest.gyroZ);
    writeU8((uint8_t)latest.mode);
    writeU8(latest.park ? 1 : 0);
    for (uint8_t i = 0; i < RC_CHANNEL_COUNT; i++) writeU16((uint16_t)latest.rcChannels[i]);
    writeI16((int16_t)latest.rcThrottle);
    writeI16((int16_t)latest.rcSteering);
    writeI16((int16_t)latest.pilotThrottle);
    writeI16((int16_t)latest.pilotSteering);
    writeF32(latest.gyroZFiltered);
    writeF32(latest.driftCompensation);
    writeU8(latest.driftEnabled ? 1 : 0);
    writeU8(latest.driftActive ? 1 : 0);
    writeF32(latest.voltage);
    uint8_t* pointCountSlot = cursor++;
    for (uint32_t seq = firstSeq; seq < firstSeq + available; seq++) {
        uint16_t index = wifiWebDataIndexForSeq(seq);
        if (index == WIFI_WEB_DATA_CAPACITY) continue;
        WebDataPoint& point = wifiWebData[index];
        writeU32(point.seq);
        writeU32((uint32_t)point.t);
        writeU16(point.dtMs);
        writeI16((int16_t)point.throttle);
        writeI16((int16_t)point.steering);
        writeF32(point.gyroZ);
        pointCount++;
        lastSentSeq = seq;
    }
    *pointCountSlot = pointCount;
    if (pointCount > 0 && wifiWebSocketClient && wifiWebSocketClient->canSend()) {
        wifiWebSocketClient->binary(wifiWebSocketBinaryPayload, cursor - wifiWebSocketBinaryPayload);
        wifiWebSocketClientLastSeq = lastSentSeq;
        wifiWebSocketFramesSent++;
        lastWifiWebSocketPushMs = now;
    }
}

void setupWifiWebSocket()
{
    wifiWebSocketPayload.reserve(1536);
    wifiWebSocket.onEvent(handleWifiWebSocketEvent);
    wifiWebSocketServer.addHandler(&wifiWebSocket);
    wifiWebSocketServer.begin();
    mus4Logf("web", "ws telemetry port=%u", WIFI_WEB_SOCKET_PORT);
}

void updateWifiWebSocket()
{
    unsigned long stageStart = millis();
    wifiWebSocket.cleanupClients();
    pushWifiWebSocketData();
    uint32_t stageDt = (uint32_t)(millis() - stageStart);
    if (stageDt > wifiWebSocketMaxDtMs) wifiWebSocketMaxDtMs = stageDt;
}

#endif // ENABLE_WIFI_WEBSOCKET_TELEMETRY
