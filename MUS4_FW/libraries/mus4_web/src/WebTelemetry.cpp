#include "WebTelemetry.h"

#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY

#include "JsonUtil.h"
#include "Mus4Log.h"
#include "RuntimeState.h"
#include "SharedTypes.h"
#include "WebLogBuffer.h"
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
uint32_t wifiWebSocketBroadcastLastSeq = 0;
uint32_t wifiWebSocketDroppedPoints = 0;
uint32_t wifiWebSocketQueueFullSkips = 0;
uint32_t wifiWebSocketHeapSkips = 0;
uint32_t wifiWebSocketFramesSent = 0;
uint32_t wifiWebSocketMaxBacklog = 0;
uint32_t wifiWebSocketConnects = 0;
uint32_t wifiWebSocketDisconnects = 0;
uint32_t wifiWebSocketMaxDtMs = 0;

// v1.7.16：AsyncTCP task 上的 onEvent 回调不得直接 mus4LogLine —— 那会触发
// sendWebLogToSocket 在 AsyncTCP task 上写共享 String，与 main loop 的同一 String
// 写入并发 realloc 撕裂堆。改为只在回调里翻 volatile 标志，main loop 在
// `updateWifiWebSocket()` 里读到后再打日志。
// v1.7.17：上一刀还漏了 hello —— 它在 CONNECT 回调里直接调，也写共享 String。
// 把 hello 一起搬到 main loop 消费 pendingWsConnectEvent 时发出；并彻底删掉
// 共享 String，让所有 text JSON 都用栈上局部 String，杜绝跨上下文 realloc 同一块。
// v1.7.26：支持多客户端并发观看曲线，但仍保持「String 写入只在 main loop」
// 的约束；广播通过 binaryAll/textAll 共享同一份 payload，避免每个客户端重复序列化。
static volatile bool pendingWsConnectEvent = false;
static volatile bool pendingWsDisconnectEvent = false;
static volatile uint32_t pendingWsConnectClientId = 0;
static volatile uint32_t pendingWsDisconnectClientId = 0;

static unsigned long lastWifiWebSocketPushMs = 0;
static uint8_t wifiWebSocketBinaryPayload[384];

static uint16_t wifiWebDataIndexForSeq(uint32_t seq)
{
    for (uint16_t i = 0; i < wifiWebDataCount; i++) {
        uint16_t index = (wifiWebDataHead + WIFI_WEB_DATA_CAPACITY - wifiWebDataCount + i) % WIFI_WEB_DATA_CAPACITY;
        if (wifiWebData[index].seq == seq) return index;
    }
    return WIFI_WEB_DATA_CAPACITY;
}

static void sendWifiWebSocketHello(uint32_t clientId)
{
    // 栈上局部 String：realloc 只触碰本函数私有堆块，不会被另一上下文撕。
    String payload;
    payload.reserve(64);
    payload = "{\"type\":\"hello\",\"seq\":";
    payload += wifiWebDataSeq;
    payload += '}';
    wifiWebSocket.text(clientId, payload);
}

static void sendWebLogToSocket(uint32_t seq, unsigned long t, const char* source, const char* line)
{
    if (!wifiWebSocketClientConnected) return;
    if (otaRuntime.inProgress) return;
    if (ESP.getFreeHeap() < WIFI_WEB_TELEMETRY_MIN_FREE_HEAP) return;

    // 栈上局部 String：v1.7.17 起不再使用共享 static String。
    // 多客户端下通过 textAll 广播，payload 在内部被多个 client 共享，避免重复拼装。
    String payload;
    payload.reserve(192);
    payload = "{\"type\":\"log\",\"seq\":";
    payload += seq;
    payload += ",\"t\":";
    payload += t;
    payload += ",\"src\":\"";
    payload += source;
    payload += "\",\"line\":";
    appendJsonString(payload, line);
    payload += '}';
    wifiWebSocket.textAll(payload);
}

static void handleWifiWebSocketMessage(AsyncWebSocketClient* client, uint8_t* data, size_t length)
{
    // 多客户端模式下，不再按客户端单独维护 lastSeq，避免一个客户端的慢速消费
    // 拖慢其他客户端。hello + 连续广播已经让每台设备同步到最新 seq。
    // 这里保留函数入口，仅用于参数消音，避免 -Wunused-parameter 警告。
    (void)client;
    (void)data;
    (void)length;
}

static void handleWifiWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t length)
{
    (void)server;
    if (type == WS_EVT_CONNECT) {
        // OTA 实际传输期间拒绝新的 WebSocket 连接，避免 WS 数据流与 OTA 挤占资源。
        // DEV 模式下 windowOpen 长期为 true，因此只以 inProgress 为准。
        if (otaRuntime.inProgress) {
            client->close();
            return;
        }
        // 允许最多 WIFI_WEB_SOCKET_MAX_CLIENTS 个并发 telemetry 客户端；
        // 超过上限时关闭新连接，让前台标签保持高效 WS，而不是被迫回退到 HTTP 轮询。
        if (wifiWebSocket.count() > WIFI_WEB_SOCKET_MAX_CLIENTS) {
            client->close();
            return;
        }
        wifiWebSocketConnects++;
        client->keepAlivePeriod(WIFI_WEB_SOCKET_KEEPALIVE_SECONDS);
        client->setCloseClientOnQueueFull(false);
        // v1.7.17：不再在 AsyncTCP task 上下文做任何 String 写入。
        // hello 和日志都由 main loop 单一上下文发出，杜绝跨上下文撕堆。
        pendingWsConnectClientId = client->id();
        pendingWsConnectEvent = true;
        return;
    }
    if (type == WS_EVT_DISCONNECT) {
        wifiWebSocketDisconnects++;
        pendingWsDisconnectClientId = client->id();
        pendingWsDisconnectEvent = true;
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
    if (otaRuntime.inProgress) return;
    if (ESP.getFreeHeap() < WIFI_WEB_TELEMETRY_MIN_FREE_HEAP) {
        wifiWebSocketHeapSkips++;
        return;
    }
    unsigned long now = millis();
    if (now - lastWifiWebSocketPushMs < WIFI_WEB_SOCKET_PUSH_INTERVAL_MS) return;
    if (wifiWebDataCount == 0 || wifiWebDataSeq <= wifiWebSocketBroadcastLastSeq) return;
    uint32_t firstSeq = wifiWebSocketBroadcastLastSeq + 1;
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
    uint32_t lastSentSeq = wifiWebSocketBroadcastLastSeq;
    uint16_t latestIndex = (wifiWebDataHead + WIFI_WEB_DATA_CAPACITY - 1) % WIFI_WEB_DATA_CAPACITY;
    WebDataPoint& latest = wifiWebData[latestIndex];
    writeU8('M');
    writeU8('4');
    writeU8(2);
    writeU8(0);
    writeU32(wifiWebSocketDroppedPoints);
    writeU32(latest.seq);
    writeU32((uint32_t)latest.t);
    writeU16(latest.dtMs);
    writeI16((int16_t)latest.throttle);
    writeI16((int16_t)latest.steering);
    writeF32(latest.gyroZ);
    // IMU 五轴（schema v2 新增）：与 HTTP /api/data latest 的 gx/gy/ax/ay/az 一一对应，
    // 仅写 latest 区，逐点 history 仍保持紧凑（不为漂移模型增加每帧广播体积）。
    writeF32(latest.gyroX);
    writeF32(latest.gyroY);
    writeF32(latest.accelX);
    writeF32(latest.accelY);
    writeF32(latest.accelZ);
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
    writeF32(latest.pseudoSpeed);
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
    if (pointCount > 0) {
        // 多客户端广播：只序列化一次，binaryAll 内部用共享 buffer 分发给所有 client。
        // 某个 client 队列满时仅跳过该 client，不影响其他客户端。
        size_t payloadLen = cursor - wifiWebSocketBinaryPayload;
        AsyncWebSocket::SendStatus status = wifiWebSocket.binaryAll(wifiWebSocketBinaryPayload, payloadLen);
        if (status != AsyncWebSocket::SendStatus::DISCARDED) {
            wifiWebSocketBroadcastLastSeq = lastSentSeq;
            wifiWebSocketFramesSent++;
            lastWifiWebSocketPushMs = now;
        } else {
            wifiWebSocketQueueFullSkips++;
        }
    }
}

void setupWifiWebSocket()
{
    // v1.7.17：共享 static String 删除后，setup 不再需要预 reserve；
    // 各 send 路径自己用栈上局部 String。
    wifiWebSocket.onEvent(handleWifiWebSocketEvent);
    wifiWebSocketServer.addHandler(&wifiWebSocket);
    wifiWebSocketServer.begin();
    webLogBufferSetSocketSink(sendWebLogToSocket);
    mus4Logf("web", "ws telemetry port=%u max_clients=%u", WIFI_WEB_SOCKET_PORT, WIFI_WEB_SOCKET_MAX_CLIENTS);
}

void updateWifiWebSocket()
{
    unsigned long stageStart = millis();
    // 保持并发客户端数量上限，避免资源被无限增长的标签页占满。
    wifiWebSocket.cleanupClients(WIFI_WEB_SOCKET_MAX_CLIENTS);
    wifiWebSocketClientConnected = wifiWebSocket.count() > 0;

    // OTA 上传期间，主循环里关闭并发的 WebSocket 遥测连接，
    // 并拒绝新的 WS 连接，避免 WS 数据流与 OTA 传输挤占 AsyncTCP 资源。
    // closeWsPending 在 OTA 窗口打开/上传开始时被设置，用于在 inProgress 之前尽早清场。
    if ((otaRuntime.closeWsPending || otaRuntime.inProgress) && wifiWebSocketClientConnected) {
        otaRuntime.closeWsPending = false;
        wifiWebSocket.closeAll(1000, "ota");
        wifiWebSocketClientConnected = false;
        mus4LogLine("web", "ws closed for ota");
    }

    // 消费 AsyncTCP task 通过 volatile 标志报来的连接事件，由 main loop 单一上下文
    // 处理 hello + 日志 —— 保证 String 写入永远只在 main loop 执行，与
    // pushWifiWebSocketData / appendWebLog 走同一条线性时间线，不再有共享/撕堆 race。
    // 先消费 disconnect 再消费 connect：避免短时间断连重连的两条日志倒序。
    if (pendingWsDisconnectEvent) {
        pendingWsDisconnectEvent = false;
        mus4LogLine("web", "ws disconnected");
    }
    if (pendingWsConnectEvent) {
        pendingWsConnectEvent = false;
        uint32_t helloId = pendingWsConnectClientId;
        // 先发 hello，让前端尽早拿到 seq 起点；再打日志（也会触发 sink）。
        sendWifiWebSocketHello(helloId);
        mus4LogLine("web", "ws connected");
    }
    pushWifiWebSocketData();
    uint32_t stageDt = (uint32_t)(millis() - stageStart);
    if (stageDt > wifiWebSocketMaxDtMs) wifiWebSocketMaxDtMs = stageDt;
}

#endif // ENABLE_WIFI_WEBSOCKET_TELEMETRY
