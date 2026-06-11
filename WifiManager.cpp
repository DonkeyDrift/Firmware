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
    scheduleWifiApRestart();
    return true;
}

void startWifiMdnsIfNeeded()
{
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
    if (!wifiStaHandoffActive) return;
    snprintf(wifiStaHandoffStaIp, sizeof(wifiStaHandoffStaIp), "%s", WiFi.localIP().toString().c_str());
    mus4Logf("wifi", "STA handoff ready ssid=%s ip=%s", wifiStaHandoffTargetSsid, wifiStaHandoffStaIp);
}

void startWifiStaHandoff(const String& targetSsid)
{
    wifiStaHandoffActive = true;
    targetSsid.toCharArray(wifiStaHandoffTargetSsid, sizeof(wifiStaHandoffTargetSsid));
    snprintf(wifiStaHandoffApSsid, sizeof(wifiStaHandoffApSsid), "%s", getActiveWifiApSsid().c_str());
    wifiStaHandoffStaIp[0] = 0;
    wifiStaHandoffStartedMs = millis();
    ensureWifiApAvailable();
    mus4Logf("wifi", "STA handoff started target=%s", wifiStaHandoffTargetSsid);
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
    clearWifiStaLastError();
    wifiStaConnectStartMs = millis();
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

static String wifiStaSsidShortUpper()
{
    String sta = WiFi.SSID();
    if (sta.length() == 0) return String();
    String out;
    out.reserve(3);
    for (uint8_t i = 0; i < sta.length() && out.length() < 3; i++) {
        char c = sta[i];
        if (c & 0x80) continue; // Skip non-ASCII bytes to keep SSID printable
        out += (char)toupper(c);
    }
    return out.length() == 3 ? out : String();
}

static String wifiStaIpTailText()
{
    IPAddress ip = WiFi.localIP();
    return String(ip[2]) + "." + String(ip[3]);
}

static String buildWifiDevApSsid(const String& baseSsid)
{
    if (!baseSsid.endsWith(WIFI_AP_SSID_SUFFIX)) return baseSsid;
    String prefix = baseSsid.substring(0, baseSsid.length() - strlen(WIFI_AP_SSID_SUFFIX));
    String staShort = wifiStaSsidShortUpper();
    if (staShort.length() == 0) return baseSsid;
    return prefix + WIFI_AP_SSID_SUFFIX + "-" + staShort + "-" + wifiStaIpTailText();
}

String getActiveWifiApSsid()
{
    if (!wifiDevModeEnabled || !wifiStaConnected) return String(wifiApSsid);
    return buildWifiDevApSsid(wifiApSsid);
}

bool configureWifiSoftApNetwork()
{
    IPAddress apIp(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    return WiFi.softAPConfig(apIp, apIp, subnet);
}

bool startWifiConsoleServices(const char* logPrefix)
{
    wifiCaptiveDnsServer.start(53, "*", WiFi.softAPIP());
    wifiConsoleServer.begin();
    wifiConsoleServer.setNoDelay(true);
    wifiWebServer.begin();
    wifiConsoleStarted = true;
    mus4Logf("wifi", "%s ssid=%s IP: %s", logPrefix, getActiveWifiApSsid().c_str(), WiFi.softAPIP().toString().c_str());
    return true;
}

bool startWifiApServices(const char* logPrefix)
{
    configureWifiSoftApNetwork();
    String activeSsid = getActiveWifiApSsid();
    bool started = WiFi.softAP(
        activeSsid.c_str(),
        WIFI_CONSOLE_AP_PASSWORD,
        WIFI_CONSOLE_CHANNEL,
        false,
        WIFI_CONSOLE_MAX_CLIENTS
    );
    if (!started) {
        wifiConsoleStarted = false;
        mus4Logf("wifi", "%s failed", logPrefix);
        return false;
    }
    return startWifiConsoleServices(logPrefix);
}

bool ensureWifiApAvailable()
{
    wifiApRestartPending = false;
    if (WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
        return startWifiApServices("AP ensured");
    }
    return startWifiConsoleServices("AP ensured");
}

bool restartWifiAp()
{
    wifiApRestartPending = false;
    wifiCaptiveDnsServer.stop();
    WiFi.softAPdisconnect(true);
    delay(100);
    WiFi.mode(WIFI_AP_STA);
    return startWifiApServices("AP restarted");
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
    webLogBufferInit();
    mus4SetWebLogSink(appendWebLog);
    lastWifiConsoleStartAttemptMs = millis();
    wifiStaConnected = false;
    wifiStaTimedOut = false;
    wifiStaConnecting = false;
    wifiApRestartPending = false;
    clearWifiStaLastError();
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);
    {
        String host = wifiMdnsHostText();
        if (host.length() > 0) {
            WiFi.setHostname(host.c_str());
        }
    }
    setupWifiWebConsole();
    if (!startWifiApServices("AP started")) {
        return;
    }
    mus4Logf("wifi", "AP %s IP: %s Port: %u Web: %u", getActiveWifiApSsid().c_str(), WiFi.softAPIP().toString().c_str(), WIFI_CONSOLE_PORT, WIFI_WEB_CONSOLE_PORT);
    if (wifiStaConfigured) {
        applyWifiStaCredentials();
    }
}

void updateWifiSta()
{
    if (!wifiStaConfigured) return;
    if (wifiStaApplyPending && (long)(millis() - wifiStaApplyDeadlineMs) >= 0) {
        applyWifiStaCredentials();
    }
    wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED) {
        if (!wifiStaConnected) {
            wifiStaConnected = true;
            wifiStaTimedOut = false;
            wifiStaConnecting = false;
            clearWifiStaLastError();
            startWifiMdnsIfNeeded();
#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
            startWifiNetbiosIfNeeded();
#endif
#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
            startWifiLlmnrIfNeeded();
#endif
            // Re-bind WebServer so the STA interface is included in the listen
            // socket. LwIP may not auto-add new interfaces to an existing
            // INADDR_ANY socket on some Arduino-ESP32 builds.
            wifiWebServer.close();
            wifiWebServer.begin();
            mus4LogLine("wifi", "WebServer re-bound for STA");
            mus4Logf("wifi", "STA connected IP: %s", WiFi.localIP().toString().c_str());
            if (wifiDevModeEnabled) {
                String targetSsid = getActiveWifiApSsid();
                if (!targetSsid.equals(WiFi.softAPSSID())) {
                    mus4Logf("wifi", "dev AP SSID update: %s", targetSsid.c_str());
                    scheduleWifiApRestart();
                }
            }
            finishWifiStaHandoff();
        }
        return;
    }
    if (wifiStaConnected) {
        wifiStaConnected = false;
        stopWifiMdnsIfNeeded();
#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
        stopWifiNetbiosIfNeeded();
#endif
#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
        stopWifiLlmnrIfNeeded();
#endif
        mus4LogLine("wifi", "STA disconnected");
        if (!String(wifiApSsid).equals(WiFi.softAPSSID())) {
            scheduleWifiApRestart();
        } else {
            ensureWifiApAvailable();
        }
    }
    if (!wifiStaConnecting) return;
    if (status == WL_NO_SSID_AVAIL) {
        setWifiStaLastError("no_ssid", "未找到目标 SSID，请检查网络名称或距离。", false);
        return;
    }
    if (status == WL_CONNECT_FAILED) {
        setWifiStaLastError("auth_failed", "STA 认证失败，请检查 Wi-Fi 密码。", false);
        return;
    }
    if (!wifiStaTimedOut && millis() - wifiStaConnectStartMs >= WIFI_STA_CONNECT_TIMEOUT_MS) {
        setWifiStaLastError("timeout", "STA 连接超时，请检查 SSID、密码与路由器信号。", true);
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
        if (millis() - lastWifiConsoleStartAttemptMs >= WIFI_CONSOLE_RETRY_INTERVAL_MS) {
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
