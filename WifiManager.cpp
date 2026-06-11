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
    snprintf(wifiStaHandoffApSsid, sizeof(wifiStaHandoffApSsid), "%s", wifiApSsid);
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
    wifiStaApplyPending = false;
    wifiStaConnected = false;
    wifiStaTimedOut = false;
    wifiStaConnecting = true;
    clearWifiStaLastError();
    wifiStaConnectStartMs = millis();
    disconnectWifiStaOnly();
    WiFi.begin(wifiStaSsid, wifiStaPassword);
    mus4Logf("wifi", "STA connecting: %s", wifiStaSsid);
}

void scheduleWifiApRestart()
{
    wifiApRestartPending = true;
    wifiApRestartDeadlineMs = millis() + WIFI_STA_APPLY_DELAY_MS;
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
    mus4Logf("wifi", "%s ssid=%s IP: %s", logPrefix, wifiApSsid, WiFi.softAPIP().toString().c_str());
    return true;
}

bool startWifiApServices(const char* logPrefix)
{
    configureWifiSoftApNetwork();
    bool started = WiFi.softAP(
        wifiApSsid,
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
    setupWifiWebConsole();
    if (!startWifiApServices("AP started")) {
        return;
    }
    mus4Logf("wifi", "AP %s IP: %s Port: %u Web: %u", wifiApSsid, WiFi.softAPIP().toString().c_str(), WIFI_CONSOLE_PORT, WIFI_WEB_CONSOLE_PORT);
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
            mus4Logf("wifi", "STA connected IP: %s", WiFi.localIP().toString().c_str());
            finishWifiStaHandoff();
        }
        return;
    }
    if (wifiStaConnected) {
        wifiStaConnected = false;
        stopWifiMdnsIfNeeded();
        mus4LogLine("wifi", "STA disconnected");
        ensureWifiApAvailable();
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
