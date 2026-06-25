#include "WifiManager.h"

#ifdef ENABLE_WIFI_CONSOLE

#include "WifiConsoleTypes.h"
#include "WifiIdentity.h"
#include "WifiOta.h"
#include "WifiStaConfig.h"
#include "WebLogBuffer.h"
#include "WirelessConsole.h"
#include "Mus4Log.h"
#include "RuntimeState.h"
#include "StringPrint.h"
#include "Buzzer.h"
#include <WebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
#include <NetBIOS.h>
#endif
#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
#include <WiFiUdp.h>
#endif

// Hardware objects defined in MUS4_FW.ino
extern Preferences mus4Prefs;
extern WiFiServer wifiConsoleServer;
extern WiFiClient wifiConsoleClient;
extern WebServer wifiWebServer;
extern DNSServer wifiCaptiveDnsServer;
extern Buzzer buzzer;

// Runtime state defined in MUS4_FW.ino
extern WifiRuntimeState wifiRuntime;
extern OtaRuntimeState otaRuntime;

// Wi-Fi runtime aliases (defined in MUS4_FW.ino)
extern bool& wifiConsoleStarted;
extern bool& wifiConsoleAuthenticated;
extern bool& wifiStaConfigured;
extern bool& wifiStaConnected;
extern bool& wifiStaTimedOut;
extern bool& wifiStaConnecting;
extern char* const wifiStaLastError;
extern char* const wifiStaLastErrorMessage;
extern bool& wifiStaApplyPending;
extern bool& wifiStaApplyFromAp;
extern bool& wifiApRestartPending;
extern bool& wifiMdnsStarted;
extern bool& wifiStaHandoffActive;
extern char* const wifiStaHandoffTargetSsid;
extern char* const wifiStaHandoffStaIp;
extern char* const wifiStaHandoffApSsid;
extern unsigned long& wifiStaHandoffStartedMs;
extern bool& wifiDevModeEnabled;
extern char* const wifiApSsid;
extern char* const wifiStaSsid;
extern char* const wifiStaPassword;
extern bool& wifiStaPasswordSet;
extern unsigned long& lastWifiConsoleStartAttemptMs;
extern unsigned long& wifiStaConnectStartMs;
extern unsigned long& wifiStaApplyDeadlineMs;
extern unsigned long& wifiApRestartDeadlineMs;
extern unsigned long& wifiStaUpGraceDeadlineMs;
extern unsigned long& wifiStaDownGraceDeadlineMs;
extern bool& wifiInApOnlyMode;
extern bool& wifiOtaStarted;
extern bool& wifiOtaWindowOpen;
extern bool& wifiOtaInProgress;
extern bool& wifiOtaParkGuardActive;
extern unsigned long& wifiOtaDeadlineMs;
extern uint8_t& wifiOtaLastProgressPct;

// Shared serial buffer (defined in MUS4_FW.ino)
extern SerialBuf wifiConsoleBuf;

// Shared web data (defined in MUS4_FW.ino)
extern WebDataPoint wifiWebData[];
extern uint16_t wifiWebDataHead;
extern uint16_t wifiWebDataCount;
extern uint32_t wifiWebDataSeq;

// Wi-Fi scan cache (defined in MUS4_FW.ino)
extern WifiScanEntry wifiScanCache[];
extern uint8_t wifiScanCacheCount;

#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
static bool wifiNetbiosStarted = false;
#endif

#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
static WiFiUDP wifiLlmnrUdp;
static bool wifiLlmnrStarted = false;
static const uint16_t LLMNR_PORT = 5355;
static const IPAddress LLMNR_MULTICAST_IP(224, 0, 0, 252);

static bool isLlmnrQueryForHost(const uint8_t* data, uint16_t len, const String& host)
{
    if (len < host.length() + 5) return false;
    uint8_t labelLen = data[0];
    if (labelLen != host.length()) return false;
    for (uint8_t i = 0; i < labelLen; i++) {
        if (tolower(data[1 + i]) != tolower(host[i])) return false;
    }
    if (data[1 + labelLen] != 0) return false;
    uint16_t qtype = (data[2 + labelLen] << 8) | data[3 + labelLen];
    uint16_t qclass = (data[4 + labelLen] << 8) | data[5 + labelLen];
    return qtype == 0x0001 && qclass == 0x0001;
}

static void processLlmnrPacket()
{
    if (!wifiLlmnrStarted) return;
    int packetSize = wifiLlmnrUdp.parsePacket();
    if (packetSize < 9) return;

    uint8_t buffer[256];
    int len = wifiLlmnrUdp.read(buffer, sizeof(buffer));
    if (len < 9) return;

    // Must be a query (QR bit clear)
    if (len >= 3 && (buffer[2] & 0x80)) return;

    String host = wifiMdnsHostText();
    if (len < 12 + (int)host.length() + 5) return;
    if (!isLlmnrQueryForHost(buffer + 12, len - 12, host)) return;

    IPAddress remoteIp = wifiLlmnrUdp.remoteIP();
    uint16_t remotePort = wifiLlmnrUdp.remotePort();
    uint16_t queryId = (buffer[0] << 8) | buffer[1];

    uint8_t response[128];
    uint16_t idx = 0;

    // Header
    response[idx++] = (queryId >> 8) & 0xFF;
    response[idx++] = queryId & 0xFF;
    response[idx++] = 0x80; // QR=1 (response)
    response[idx++] = 0x00;
    response[idx++] = 0x00; // QDCOUNT = 1
    response[idx++] = 0x01;
    response[idx++] = 0x00; // ANCOUNT = 1
    response[idx++] = 0x01;
    response[idx++] = 0x00; // NSCOUNT = 0
    response[idx++] = 0x00;
    response[idx++] = 0x00; // ARCOUNT = 0
    response[idx++] = 0x00;

    // Question section (embedded name at offset 12 for C00C pointer)
    uint8_t nameLen = host.length();
    response[idx++] = nameLen;
    for (uint8_t i = 0; i < nameLen; i++) {
        response[idx++] = tolower(host[i]);
    }
    response[idx++] = 0x00;
    response[idx++] = 0x00; // Type A
    response[idx++] = 0x01;
    response[idx++] = 0x00; // Class IN
    response[idx++] = 0x01;

    // Answer section (pointer C00C to question name)
    response[idx++] = 0xC0;
    response[idx++] = 0x0C;
    response[idx++] = 0x00; // Type A
    response[idx++] = 0x01;
    response[idx++] = 0x00; // Class IN
    response[idx++] = 0x01;
    response[idx++] = 0x00; // TTL = 300
    response[idx++] = 0x00;
    response[idx++] = 0x01;
    response[idx++] = 0x2C;
    response[idx++] = 0x00; // RDLENGTH = 4
    response[idx++] = 0x04;

    IPAddress localIp = WiFi.localIP();
    response[idx++] = localIp[0];
    response[idx++] = localIp[1];
    response[idx++] = localIp[2];
    response[idx++] = localIp[3];

    wifiLlmnrUdp.beginPacket(remoteIp, remotePort);
    wifiLlmnrUdp.write(response, idx);
    wifiLlmnrUdp.endPacket();
}
#endif // ENABLE_WIFI_LLMNR_DISCOVERY

// Web console layer (defined in other modules)
extern void setupWebConsoleServer();
extern void updateWebConsoleServer();
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
extern void setupWifiWebSocket();
extern void updateWifiWebSocket();
#endif

void loadDevModePreference()
{
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, true)) {
        wifiDevModeEnabled = false;
        mus4LogLine("wifi", "dev_mode load failed");
        return;
    }
    wifiDevModeEnabled = mus4Prefs.getBool(MUS4_PREF_DEV_MODE_KEY, false);
    mus4Prefs.end();
    mus4Logf("wifi", "dev_mode=%d", wifiDevModeEnabled ? 1 : 0);
}

bool saveDevModePreference(bool enabled)
{
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    size_t written = mus4Prefs.putBool(MUS4_PREF_DEV_MODE_KEY, enabled);
    mus4Prefs.end();
    if (written == 0) return false;
    wifiDevModeEnabled = enabled;
    if (wifiDevModeEnabled) {
        keepDevModeOtaWindowActive(otaRuntime, wifiRuntime);
    } else if (wifiOtaWindowOpen && !wifiOtaInProgress) {
        closeWifiOtaWindow("DEV_MODE_OFF", otaRuntime);
    }
    mus4Logf("wifi", "dev_mode saved=%d", wifiDevModeEnabled ? 1 : 0);
    // v1.7.8 起：切换 DEV 不再触发 AP 重启——AP SSID 派生只看 STA 连接状态，
    // 与 DEV 状态无关，避免切换 DEV 时丢一次 AP/Web Console 连接。
    return true;
}

void startWifiMdnsIfNeeded()
{
#ifdef DISABLE_WIFI_NAME_DISCOVERY
    return;
#endif
    if (wifiMdnsStarted) return;
    if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) return;
    if (!MDNS.begin(wifiMdnsHostText().c_str())) {
        mus4LogLine("wifi", "mDNS start failed");
        return;
    }
    MDNS.addService("http", "tcp", WIFI_WEB_CONSOLE_PORT);
    wifiMdnsStarted = true;
    mus4Logf("wifi", "mDNS started: %s.local", wifiMdnsHostText().c_str());
}

bool ensureWifiApAvailable();
bool restartWifiAp();
bool startWifiApServices(const char* logPrefix);

void stopWifiMdnsIfNeeded()
{
    if (!wifiMdnsStarted) return;
    MDNS.end();
    wifiMdnsStarted = false;
    mus4LogLine("wifi", "mDNS stopped");
}

#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
void startWifiNetbiosIfNeeded()
{
    if (wifiNetbiosStarted) return;
    if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) return;
    if (!NBNS.begin(wifiMdnsHostText().c_str())) {
        mus4LogLine("wifi", "NetBIOS start failed");
        return;
    }
    wifiNetbiosStarted = true;
    mus4Logf("wifi", "NetBIOS started: %s", wifiMdnsHostText().c_str());
}

void stopWifiNetbiosIfNeeded()
{
    if (!wifiNetbiosStarted) return;
    NBNS.end();
    wifiNetbiosStarted = false;
    mus4LogLine("wifi", "NetBIOS stopped");
}
#endif

#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
void startWifiLlmnrIfNeeded()
{
    if (wifiLlmnrStarted) return;
    if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) return;
    if (!wifiLlmnrUdp.begin(LLMNR_PORT)) {
        mus4LogLine("wifi", "LLMNR start failed");
        return;
    }
    wifiLlmnrUdp.beginMulticast(LLMNR_MULTICAST_IP, LLMNR_PORT);
    wifiLlmnrStarted = true;
    mus4Logf("wifi", "LLMNR started: %s", wifiMdnsHostText().c_str());
}

void stopWifiLlmnrIfNeeded()
{
    if (!wifiLlmnrStarted) return;
    wifiLlmnrUdp.stop();
    wifiLlmnrStarted = false;
    mus4LogLine("wifi", "LLMNR stopped");
}
#endif

// v1.7.18 起 AP/STA 互斥切换：STA→STA 切换由「保存新配置 → applyWifiStaCredentials()
// 直接接管」处理，handoff 三态共存逻辑退役。保留下面三个接口和 staHandoff*
// 字段是为了让 WebConsoleServer.cpp 与 wifiStaJson() 的调用面继续编译通过；
// handoff_active 永远停留在 false，前端 wifiStaHandoffModal 不会再被触发。
void clearWifiStaHandoff()
{
    wifiStaHandoffActive = false;
    wifiStaHandoffTargetSsid[0] = 0;
    wifiStaHandoffStaIp[0] = 0;
    wifiStaHandoffApSsid[0] = 0;
    wifiStaHandoffStartedMs = 0;
}

void finishWifiStaHandoff()
{
    clearWifiStaHandoff();
}

void startWifiStaHandoff(const String& /*targetSsid*/)
{
    clearWifiStaHandoff();
}

void disconnectWifiStaOnly()
{
    esp_wifi_disconnect();
}

void applyWifiStaCredentials()
{
    if (!wifiStaConfigured) return;
    stopWifiMdnsIfNeeded();
#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
    stopWifiNetbiosIfNeeded();
#endif
#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
    stopWifiLlmnrIfNeeded();
#endif
    wifiStaApplyPending = false;
    wifiStaConnected = false;
    wifiStaTimedOut = false;
    wifiStaConnecting = true;
    // v1.7.18 起 AP/STA 互斥切换：发起新一轮 STA 连接前清空两个 grace 锚点，避免上一轮残留触发误关 AP。
    wifiStaUpGraceDeadlineMs = 0;
    wifiStaDownGraceDeadlineMs = 0;
    clearWifiStaLastError();
    wifiStaConnectStartMs = millis();
    // v1.7.21 起：开机模式即为 WIFI_AP_STA，常态下不会触发 mode 切换。
    // 仅当当前在 STA-only（落地后用户在 web 端发起重新连接）时才需要切回 AP_STA；
    // 同时确保 AP 服务在线，给 STA 失败提供兜底入口。
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        mus4LogLine("wifi", "STA apply: switching to AP_STA");
        // ESP-IDF 切到 AP_STA 后 STA netif 需要一小段时间初始化；不加这个延时
        // 紧接着的 STA begin 可能拿不到信道，会导致 timeout。
        delay(50);
    }
    if (WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
        startWifiApServices("AP restored for STA apply");
    }
    wifiInApOnlyMode = false;
    disconnectWifiStaOnly();
    WiFi.setHostname(wifiMdnsHostText().c_str());
    WiFi.begin(wifiStaSsid, wifiStaPassword);
    mus4Logf("wifi", "STA connecting: %s", wifiStaSsid);
}

void scheduleWifiApRestart()
{
    wifiApRestartPending = true;
    wifiApRestartDeadlineMs = millis() + WIFI_STA_APPLY_DELAY_MS;
}

String getActiveWifiApSsid()
{
    // v1.7.22 起：AP/STA 互斥切换下 AP 与 STA 永远不会同时广播，原本用于在
    // STA 上线后给 AP 名称追加「短码 + IP 尾段」的派生逻辑失去意义；统一返回
    // 基础 AP SSID。三个历史辅助函数（短码 / IP 尾段 / 派生组装）已删除。
    return String(wifiApSsid);
}

bool configureWifiSoftApNetwork()
{
    IPAddress apIp(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    return WiFi.softAPConfig(apIp, apIp, subnet);
}

bool startWifiConsoleServices(const char* logPrefix)
{
    // 先关闭再重新打开监听套接字，确保接口变化（如 STA 断开、AP 重启）后
    // 服务端能绑定到当前活动的网络接口，避免 AP 恢复后 HTTP/TCP 无法访问。
    mus4LogLine("wifi", "console services: close old sockets");
    wifiWebServer.close();
    wifiConsoleServer.stop();
    mus4LogLine("wifi", "console services: DNS start");
    wifiCaptiveDnsServer.start(53, "*", WiFi.softAPIP());
    mus4LogLine("wifi", "console services: TCP begin");
    wifiConsoleServer.begin();
    wifiConsoleServer.setNoDelay(true);
    mus4LogLine("wifi", "console services: HTTP begin");
    wifiWebServer.begin();
    wifiConsoleStarted = true;
    mus4Logf("wifi", "%s ssid=%s IP: %s", logPrefix, getActiveWifiApSsid().c_str(), WiFi.softAPIP().toString().c_str());
    return true;
}

bool startWifiApServices(const char* logPrefix)
{
    mus4LogLine("wifi", "AP services: config network");
    configureWifiSoftApNetwork();
    mus4LogLine("wifi", "AP services: softAP begin");
    String activeSsid = getActiveWifiApSsid();
    bool started = WiFi.softAP(
        activeSsid.c_str(),
        WIFI_CONSOLE_AP_PASSWORD,
        WIFI_CONSOLE_CHANNEL,
        false,
        WIFI_CONSOLE_MAX_CLIENTS
    );
    mus4LogLine("wifi", "AP services: softAP done");
    if (!started) {
        wifiConsoleStarted = false;
        mus4Logf("wifi", "%s failed", logPrefix);
        return false;
    }
    bool ok = startWifiConsoleServices(logPrefix);
    if (ok) {
        buzzer.playWifiApStartSound();
    }
    return ok;
}

bool ensureWifiApAvailable()
{
    wifiApRestartPending = false;
    // AP 接口可能仍在（WiFi.softAPIP() != 0），但 STA 扫描/连接阶段会把 SoftAP
    // 信道带跑；直接复用旧接口只重启 console 服务无法让 AP 回到正确信道，
    // 客户端重连后会出现「ping 通但 HTTP 无响应」。
    // 这里用 softAPdisconnect(false) 清掉当前 AP 配置并重启一次 AP 接口
    //（不切换底层 WIFI_AP_STA mode），再由 startWifiApServices() 重新下发
    // 正确配置、启动完整服务。
    if (WiFi.softAPIP() != IPAddress(0, 0, 0, 0)) {
        mus4LogLine("wifi", "AP interface up, soft reset before reconfigure");
        wifiCaptiveDnsServer.stop();
        WiFi.softAPdisconnect(false);
        delay(100);
    }
    return startWifiApServices("AP ensured");
}

bool restartWifiAp()
{
    wifiApRestartPending = false;
    wifiCaptiveDnsServer.stop();
    WiFi.softAPdisconnect(true);
    delay(100);
    // 保持 WIFI_AP_STA 不变，避免 AP-only ↔ AP_STA 反复切换重置 SoftAP、踢掉客户端。
    // STA 是否在线由 wifiStaConnected / wifiInApOnlyMode 语义管理，不依赖 mode。
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        delay(50);
    }
    wifiInApOnlyMode = !wifiStaConnected;
    return startWifiApServices("AP restarted");
}

// STA 稳定 grace 通过后，主动把 SoftAP 关掉；保持底层 mode 为 WIFI_AP_STA，
// 避免后续从 STA-only 再切回 AP_STA 时重置接口、踢掉客户端。
// Captive DNS 与 wifiConsoleServer 都依赖 SoftAP，AP 关闭时一并停掉，避免
// 出现「socket 还监听但 AP 已下线」的悬空状态。
static void stopWifiApForStaOnly()
{
    wifiStaUpGraceDeadlineMs = 0;
    wifiStaApplyFromAp = false;
    wifiApRestartPending = false;
    wifiCaptiveDnsServer.stop();
    WiFi.softAPdisconnect(true);
    // 保持 WIFI_AP_STA：STA 接口继续使用，AP 接口已停用，对外等效于 STA-only。
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        delay(50);
    }
    // 切到 STA-only 后重新绑定 WebServer，让监听套接字包含 STA 接口。
    // 同时避免在 AP 仍在线的 grace 窗口内 close/begin，导致配置页面中断。
    wifiWebServer.close();
    wifiWebServer.begin();
    mus4LogLine("wifi", "WebServer re-bound for STA");
    // 关 AP 后 SoftAP 端的 TCP 控制台监听句柄已失效；这里把 wifiConsoleStarted
    // 置 false 让 updateWifiConsole() 不再尝试 accept，但 wifiWebServer 仍可在
    // STA 接口上响应。
    wifiConsoleStarted = false;
    wifiInApOnlyMode = false;
    mus4LogLine("wifi", "AP stopped after STA connected");
    buzzer.playWifiApStopSound();
}

// STA 持续断开 / STA 失败 / WIFI_STA_CLEAR 路径都收敛到这里：停止 STA 并
// 确保 AP 兜底可用。保持底层 mode 为 WIFI_AP_STA，不再切到 WIFI_AP，避免
// 下一次从 AP-only 保存 STA 时做破坏性的 AP↔AP_STA 切换、踢掉配置客户端。
// v1.7.20 起本函数不再写 lastError，由调用方按场景决定错误码；这样四条失败路径
// （down grace 后的 sta_lost / timeout / auth_failed / no_ssid）能各自保留语义。
static void restoreApAfterStaLost()
{
    wifiStaUpGraceDeadlineMs = 0;
    wifiStaDownGraceDeadlineMs = 0;
    wifiStaApplyFromAp = false;
    buzzer.playWifiStaDisconnectedSound();
    wifiStaConnected = false;
    wifiStaConnecting = false;
    stopWifiMdnsIfNeeded();
#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
    stopWifiNetbiosIfNeeded();
#endif
#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
    stopWifiLlmnrIfNeeded();
#endif
    // esp_wifi_disconnect() 把 ESP32 自带的 STA 自动重连机制停掉，避免设备在
    // AP-only 下后台不断重试错误密码、干扰 RF 调度。
    esp_wifi_disconnect();
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        delay(50);
    }
    wifiInApOnlyMode = true;
    ensureWifiApAvailable();
}

void loadWifiApPreference()
{
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, true)) {
        copyWifiApSsid(String(WIFI_CONSOLE_AP_DEFAULT_SSID));
        mus4LogLine("wifi", "AP SSID load failed, using default");
        return;
    }
    String ssid = mus4Prefs.getString(MUS4_PREF_AP_SSID_KEY, WIFI_CONSOLE_AP_DEFAULT_SSID);
    mus4Prefs.end();
    ssid.trim();
    if (!copyWifiApSsid(ssid)) {
        copyWifiApSsid(String(WIFI_CONSOLE_AP_DEFAULT_SSID));
        mus4LogLine("wifi", "AP SSID invalid, using default");
    }
}

bool saveWifiApPreference(const String& ssid)
{
    String trimmed = ssid;
    trimmed.trim();
    if (!copyWifiApSsid(trimmed)) return false;
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    size_t ssidWritten = mus4Prefs.putString(MUS4_PREF_AP_SSID_KEY, wifiApSsid);
    mus4Prefs.end();
    return ssidWritten > 0;
}

void setupWifiWebConsole()
{
    setupWebConsoleServer();
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
    setupWifiWebSocket();
#endif
}

void updateWifiWebConsole()
{
    updateWebConsoleServer();
    if (wifiApRestartPending && (long)(millis() - wifiApRestartDeadlineMs) >= 0) {
        restartWifiAp();
    }
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
    updateWifiWebSocket();
#endif
}

void setupWifiConsole()
{
    mus4LogLine("wifi", "setup: entered");
    webLogBufferInit();
    mus4SetWebLogSink(appendWebLog);
    lastWifiConsoleStartAttemptMs = millis();
    wifiStaConnected = false;
    wifiStaTimedOut = false;
    wifiStaConnecting = false;
    wifiApRestartPending = false;
    // 底层 mode 始终维持 WIFI_AP_STA；AP/STA 的「互斥」通过启用/停用 SoftAP
    // 以及 STA 连接状态来管理，避免 AP-only ↔ AP_STA 反复切换重置接口、踢掉客户端。
    wifiStaUpGraceDeadlineMs = 0;
    wifiStaDownGraceDeadlineMs = 0;
    // wifiInApOnlyMode 表示「当前是否按 AP-only 行为运行」（STA 不活跃），
    // 不再与底层 WiFi mode 严格一一对应。
    wifiInApOnlyMode = false;
    clearWifiStaLastError();
    mus4LogLine("wifi", "setup: disconnect");
    WiFi.disconnect(true, true);
    mus4LogLine("wifi", "setup: mode OFF");
    WiFi.mode(WIFI_OFF);
    delay(100);
    // 全程保持 WIFI_AP_STA。SoftAP 负责 AP 兜底入口，STA 部分在配置存在时才
    // begin；AP 关闭（stopWifiApForStaOnly）或 STA 失败（restoreApAfterStaLost）
    // 时不再切 mode，只启停对应接口。这样彻底避免 AP↔AP_STA 反复切换导致的
    // SoftAP 重置、配置页面断连和 STA netif race。
    // 历史 v1.7.17 全程 WIFI_AP_STA 已验证 newhome_iot 等路由器可正常连接。
    mus4LogLine("wifi", "setup: mode AP_STA");
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);
    {
        String host = wifiMdnsHostText();
        if (host.length() > 0) {
            WiFi.setHostname(host.c_str());
        }
    }
    mus4LogLine("wifi", "setup: web console");
    setupWifiWebConsole();
    mus4LogLine("wifi", "setup: start AP services");
    if (!startWifiApServices("AP started")) {
        return;
    }
    mus4Logf("wifi", "AP %s IP: %s Port: %u Web: %u", getActiveWifiApSsid().c_str(), WiFi.softAPIP().toString().c_str(), WIFI_CONSOLE_PORT, WIFI_WEB_CONSOLE_PORT);
    if (wifiStaConfigured) {
        mus4LogLine("wifi", "setup: apply STA credentials");
        applyWifiStaCredentials();
    }
}

void updateWifiSta()
{
    if (wifiStaApplyPending && (long)(millis() - wifiStaApplyDeadlineMs) >= 0) {
        applyWifiStaCredentials();
    }
    // v1.7.18 起 AP/STA 互斥切换：未配置 STA 时停留在 AP-only；但若刚被
    // WIFI_STA_CLEAR 清空，而设备此时还在 STA_ONLY，需要立即恢复 AP。
    if (!wifiStaConfigured) {
        if (!wifiInApOnlyMode) {
            clearWifiStaLastError();
            restoreApAfterStaLost();
            mus4LogLine("wifi", "AP restored after STA cleared");
        }
        return;
    }

    wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED) {
        if (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
            // ESP-IDF 偶尔会出现 Wi-Fi 事件先报告 WL_CONNECTED、netif IP 尚未写入的
            // 短暂窗口；此时若标记 connected，/api/wifi-sta 会把 sta_ip 填成 0.0.0.0，
            // Web Console 就没办法在 AP 关闭前把局域网访问地址展示给用户。等待下一轮。
            return;
        }
        if (!wifiStaConnected) {
            // 首次进入 WL_CONNECTED：标记 STA 在线，启动 mDNS，并武装 up grace。
            // 关 AP 的动作要等 WIFI_STA_GRACE_UP_MS 后由本函数下一轮触发，
            // 避免连接刚握手就被 softAP 抖动干扰。
            wifiStaConnected = true;
            wifiStaTimedOut = false;
            wifiStaConnecting = false;
            clearWifiStaLastError();
            buzzer.playWifiStaConnectedSound();
            startWifiMdnsIfNeeded();
#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
            startWifiNetbiosIfNeeded();
#endif
#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
            startWifiLlmnrIfNeeded();
#endif
            mus4Logf("wifi", "STA connected IP: %s", WiFi.localIP().toString().c_str());
            wifiStaUpGraceDeadlineMs = millis() + (wifiStaApplyFromAp ? WIFI_STA_AP_CONFIG_SUCCESS_GRACE_MS : WIFI_STA_GRACE_UP_MS);
            wifiStaDownGraceDeadlineMs = 0;
            // handoff 在新方案里已经退役；保留 finish 调用是为了把残留字段清空。
            finishWifiStaHandoff();
        } else if (wifiStaDownGraceDeadlineMs != 0) {
            // STA 抖动后又恢复（grace 窗口内），取消 down grace，保持 STA_ONLY。
            wifiStaDownGraceDeadlineMs = 0;
            mus4LogLine("wifi", "STA recovered within grace window");
        }
        if (wifiStaUpGraceDeadlineMs != 0 && (long)(millis() - wifiStaUpGraceDeadlineMs) >= 0) {
            stopWifiApForStaOnly();
        }
        return;
    }

    if (wifiStaConnected) {
        // 进入 down grace：先不关闭 mDNS、也不切模式；只武装截止时间，等下一轮判定。
        if (wifiStaDownGraceDeadlineMs == 0) {
            wifiStaDownGraceDeadlineMs = millis() + WIFI_STA_GRACE_DOWN_MS;
            mus4LogLine("wifi", "STA link lost, arming down grace");
        }
        if ((long)(millis() - wifiStaDownGraceDeadlineMs) >= 0) {
            setWifiStaLastError("sta_lost", "STA 连接已断开，已切回 AP 模式。", true);
            restoreApAfterStaLost();
            mus4LogLine("wifi", "AP restored after STA lost");
        }
        return;
    }

    if (!wifiStaConnecting) return;
    // v1.7.20 起：三种失败路径（no_ssid / auth_failed / timeout）都先写 lastError，
    // 再走 restoreApAfterStaLost() 切回 AP-only。否则设备会卡在 AP_STA + ESP32
    // 后台 auto-reconnect 错密码的死循环里，AP 也不会重启。
    if (status == WL_NO_SSID_AVAIL) {
        setWifiStaLastError("no_ssid", "未找到目标 SSID，请检查网络名称或距离。", false);
        restoreApAfterStaLost();
        mus4LogLine("wifi", "AP restored after STA no_ssid");
        return;
    }
    if (status == WL_CONNECT_FAILED) {
        setWifiStaLastError("auth_failed", "STA 认证失败，请检查 Wi-Fi 密码。", false);
        restoreApAfterStaLost();
        mus4LogLine("wifi", "AP restored after STA auth_failed");
        return;
    }
    if (!wifiStaTimedOut && millis() - wifiStaConnectStartMs >= WIFI_STA_CONNECT_TIMEOUT_MS) {
        setWifiStaLastError("timeout", "STA 连接超时，请检查 SSID、密码与路由器信号。", true);
        restoreApAfterStaLost();
        mus4LogLine("wifi", "AP restored after STA timeout");
    }
}

void updateWifiConsole()
{
    if (wifiConsoleStarted) {
        wifiCaptiveDnsServer.processNextRequest();
    }
#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
    processLlmnrPacket();
#endif
    // Keep mDNS alive: IGMP-snooping routers may drop multicast forwarding
    // after the group membership times out. Periodically restart mDNS to
    // force a fresh IGMP join and service announcement.
    if (wifiMdnsStarted) {
        static unsigned long lastMdnsRestartMs = 0;
        if (millis() - lastMdnsRestartMs >= 60000) {
            lastMdnsRestartMs = millis();
            MDNS.end();
            wifiMdnsStarted = false;
            startWifiMdnsIfNeeded();
            if (wifiMdnsStarted) {
                mus4LogLine("wifi", "mDNS restarted for refresh");
            } else {
                mus4LogLine("wifi", "mDNS refresh failed");
            }
        }
    }
    if (!wifiConsoleStarted) {
        // v1.7.18 起 AP/STA 互斥切换：STA_ONLY 状态下 AP 已主动关闭，wifiConsoleStarted
        // 也跟着置 false，但这并非异常，不能用 WIFI_CONSOLE_RETRY_INTERVAL_MS 周期
        // 重新拉起整个 console（那会把 AP 又开回来，破坏互斥语义）。
        // v1.7.21 调整：STA 在线即认为不需要重试 console；其它情况（包括开机 AP 起不来
        // 在 AP_STA 模式下、AP-only 下 AP 异常）都按 WIFI_CONSOLE_RETRY_INTERVAL_MS 重试。
        if (!wifiStaConnected && millis() - lastWifiConsoleStartAttemptMs >= WIFI_CONSOLE_RETRY_INTERVAL_MS) {
            setupWifiConsole();
        }
        return;
    }
    if (!wifiConsoleClient || !wifiConsoleClient.connected()) {
        WiFiClient nextClient = wifiConsoleServer.available();
        if (nextClient) {
            if (wifiConsoleClient) wifiConsoleClient.stop();
            wifiConsoleClient = nextClient;
            wifiConsoleClient.setNoDelay(true);
            wifiConsoleAuthenticated = false;
            wifiConsoleClient.println("MUS4 WiFi Console Ready");
            wifiConsoleClient.println("Use AUTH:<password> to unlock control commands");
            appendWebLog("tcp", "client connected");
        }
        return;
    }
    while (wifiConsoleClient.available()) {
        int c = wifiConsoleClient.read();
        if (c < 0) break;
        if (c == '\r') continue;
        if (c == '\n') {
            wifiConsoleBuf.buf[wifiConsoleBuf.len] = 0;
            String line = String(wifiConsoleBuf.buf);
            line.trim();
            String response;
            StringPrint out(response);
            appendWebLog("tcp", String("> ") + redactWirelessConsoleLine(line));
            processWirelessConsoleLine(line, out, WIRELESS_ORIGIN_TCP);
            wifiConsoleClient.print(response);
            appendWebLogLines("cmd", response);
            wifiConsoleBuf.len = 0;
            wifiConsoleBuf.overflow = false;
        } else {
            if (wifiConsoleBuf.len < sizeof(wifiConsoleBuf.buf) - 1) {
                wifiConsoleBuf.buf[wifiConsoleBuf.len++] = (char)c;
            } else {
                wifiConsoleBuf.len = 0;
                wifiConsoleBuf.overflow = true;
                wifiConsoleBuf.errors++;
                wifiConsoleClient.println("NACK:OVERFLOW");
            }
        }
    }
}

#endif // ENABLE_WIFI_CONSOLE
