#include "MUS4.h"

// This file groups related MUS4 firmware implementation sections so the
// Arduino project stays easy to browse without changing runtime behavior.

// ============================================================================
// Section: WirelessConsole.cpp
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE


extern ControlData car_output;
extern ControlData pilot_data;
extern ControlData rc_data;
extern SensorData ina219Data;
extern SensorData mpu6050Data;
extern SerialBuf wifiConsoleBuf;
extern WifiRuntimeState wifiRuntime;
extern OtaRuntimeState otaRuntime;

extern bool drift_assist_enabled;
extern bool drift_assist_active;
extern float drift_compensation;
extern float gyro_z_filtered;
extern int pwm_filtered[RC_CHANNEL_COUNT];

extern uint32_t wifiWebUpdateMaxDtMs;
extern uint32_t wifiWebSampleMaxDtMs;
extern uint32_t wifiWebHttpMaxDtMs;
extern uint32_t wifiWebSocketMaxDtMs;
extern uint32_t wifiWebStatusRequests;
extern uint32_t wifiWebLogRequests;
extern uint32_t wifiWebDataRequests;
extern uint32_t wifiWebCommandRequests;
extern uint32_t wifiWebStatusMaxDtMs;
extern uint32_t wifiWebLogMaxDtMs;
extern uint32_t wifiWebDataMaxDtMs;
extern uint32_t wifiWebCommandMaxDtMs;

#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
extern bool wifiWebSocketClientConnected;
extern uint32_t wifiWebSocketDroppedPoints;
extern uint32_t wifiWebSocketQueueFullSkips;
extern uint32_t wifiWebSocketHeapSkips;
extern uint32_t wifiWebSocketFramesSent;
extern uint32_t wifiWebSocketMaxBacklog;
extern uint32_t wifiWebSocketConnects;
extern uint32_t wifiWebSocketDisconnects;
#endif

String redactWirelessConsoleLine(const String& line)
{
    if (line.startsWith("AUTH:")) return "AUTH:<redacted>";
    if (line.startsWith("WIFI_STA_PASSWORD:")) return "WIFI_STA_PASSWORD:<redacted>";
    return line;
}

bool isWirelessControlCommand(const String& line)
{
    int firstColon = line.indexOf(':');
    if (firstColon <= 0) return false;
    String throttleText = line.substring(0, firstColon);
    int secondColon = line.indexOf(':', firstColon + 1);
    int star = line.indexOf('*', firstColon + 1);
    int end = line.length();
    if (secondColon > firstColon) end = secondColon;
    if (star > firstColon && star < end) end = star;
    String steeringText = line.substring(firstColon + 1, end);
    throttleText.trim();
    steeringText.trim();
    if (throttleText.length() == 0 || steeringText.length() == 0) return false;
    for (uint16_t i = 0; i < throttleText.length(); i++) {
        char c = throttleText.charAt(i);
        if (!(isDigit(c) || (i == 0 && c == '-'))) return false;
    }
    for (uint16_t i = 0; i < steeringText.length(); i++) {
        char c = steeringText.charAt(i);
        if (!(isDigit(c) || (i == 0 && c == '-'))) return false;
    }
    return true;
}

bool isWirelessOtaOpenCommand(const String& line)
{
    return line.equalsIgnoreCase("ENABLE_OTA");
}

bool isLocalOtaOpenCommand(const String& line)
{
    return line.startsWith("ENABLE_OTA:");
}

bool isWirelessOtaStatusCommand(const String& line)
{
    return line.equalsIgnoreCase("OTA_STATUS");
}

bool isWirelessOtaCloseCommand(const String& line)
{
    return line.equalsIgnoreCase("DISABLE_OTA");
}

bool isWifiStaConfigCommand(const String& line)
{
    return line.startsWith("WIFI_STA_SSID:") ||
        line.startsWith("WIFI_STA_PASSWORD:") ||
        line.equalsIgnoreCase("WIFI_STA_APPLY") ||
        line.equalsIgnoreCase("WIFI_STA_CLEAR");
}

bool isParkLockedWirelessCommand(const String& line)
{
    return isWirelessOtaOpenCommand(line) ||
        line.equalsIgnoreCase("STEER_CAL") ||
        line.equalsIgnoreCase("CAL_SAVE") ||
        line.equalsIgnoreCase("CAL_RETRY") ||
        line.equalsIgnoreCase("CAL_ABORT") ||
        line.equalsIgnoreCase("CAL_RESET") ||
        line.equalsIgnoreCase("CAL_STATUS") ||
        line.equalsIgnoreCase("TEST") ||
        line.equalsIgnoreCase("TEST_TUI") ||
        line.equalsIgnoreCase("BENCH") ||
        line.equalsIgnoreCase("STRESS") ||
        line.equalsIgnoreCase("REGRESS") ||
        line.equalsIgnoreCase("FILTER_TEST");
}

bool isWirelessCommandAllowed(const String& line, WirelessCommandOrigin origin, WifiRuntimeState& ws)
{
    bool webDevMode = ws.devModeEnabled && origin == WIRELESS_ORIGIN_WEB;
    if (line.equalsIgnoreCase("PING") || line.equalsIgnoreCase("STATUS") || line.equalsIgnoreCase("WIFI_STA_STATUS")) return true;
    if (line.startsWith("AUTH:")) return true;
    if (isWirelessOtaOpenCommand(line)) return (webDevMode || ws.consoleAuthenticated) && car_output.park == PARK_LOCKED;
    if (isWirelessOtaStatusCommand(line) || isWirelessOtaCloseCommand(line)) return webDevMode || ws.consoleAuthenticated;
    if (!ws.consoleAuthenticated && !webDevMode) return false;
    if (isParkLockedWirelessCommand(line)) return car_output.park == PARK_LOCKED;
    if (line.equalsIgnoreCase("ANSI") || line.equalsIgnoreCase("NOANSI") || line.equalsIgnoreCase("FILTER_DEBUG") || line.equalsIgnoreCase("LOG_WEB") || line.equalsIgnoreCase("LOG_SERIAL") || isWifiStaConfigCommand(line)) return true;
    return isWirelessControlCommand(line);
}

void processWirelessConsoleLine(const String& line, Print& out, WirelessCommandOrigin origin)
{
    if (line.equalsIgnoreCase("PING")) {
        out.println("PONG");
        return;
    }
    if (line.equalsIgnoreCase("STATUS")) {
        printWirelessStatus(out);
        return;
    }
    if (line.startsWith("AUTH:")) {
        wifiRuntime.consoleAuthenticated = line.substring(5).equals(WIFI_CONSOLE_AP_PASSWORD);
        out.println(wifiRuntime.consoleAuthenticated ? "AUTH_OK" : "AUTH_FAIL");
        return;
    }
    if (!isWirelessCommandAllowed(line, origin, wifiRuntime)) {
        bool webDevMode = wifiRuntime.devModeEnabled && origin == WIRELESS_ORIGIN_WEB;
        if (isParkLockedWirelessCommand(line) && car_output.park != PARK_LOCKED && (wifiRuntime.consoleAuthenticated || webDevMode)) {
            out.println("NACK:PARK_REQUIRED");
        } else {
            out.println("NACK:UNAUTHORIZED");
        }
        wifiConsoleBuf.errors++;
        return;
    }
    if (isWirelessOtaOpenCommand(line)) {
        openWifiOtaWindow(out, origin, otaRuntime, wifiRuntime);
        return;
    }
    if (isWirelessOtaStatusCommand(line)) {
        printWifiOtaStatus(out, otaRuntime, wifiRuntime);
        return;
    }
    if (isWirelessOtaCloseCommand(line)) {
        closeWifiOtaWindow("USER", otaRuntime);
        out.println("OTA_CLOSED");
        return;
    }
    if (processWifiStaConfigCommand(line, out, wifiRuntime)) {
        return;
    }
    dispatchCommandLine(line, out, wifiConsoleBuf);
}
#endif

// ============================================================================
// Section: WifiIdentity.cpp
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE

static const uint8_t WIFI_IDENTITY_AP_SSID_MAX_LEN = 32;

static WifiRuntimeState* g_ws = nullptr;

void setWifiIdentityRuntimeState(WifiRuntimeState& ws)
{
    g_ws = &ws;
}

static inline char* apSsid()
{
    return g_ws ? g_ws->apSsid : nullptr;
}

bool isMdnsSafeHostnameChar(char c)
{
    return (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') ||
        c == '-';
}

bool isMdnsSafeHostname(const String& value)
{
    if (value.length() == 0 || value.length() > WIFI_IDENTITY_AP_SSID_MAX_LEN) return false;
    if (value[0] == '-' || value[value.length() - 1] == '-') return false;
    for (uint8_t i = 0; i < value.length(); i++) {
        if (!isMdnsSafeHostnameChar(value[i])) return false;
    }
    return true;
}

bool isValidApSsidPrefix(const String& value)
{
    if (value.length() == 0 || value.length() > WIFI_AP_SSID_PREFIX_MAX_LEN) return false;
    for (uint8_t i = 0; i < value.length(); i++) {
        char c = value[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) {
            return false;
        }
    }
    return true;
}

bool copyWifiApSsid(const String& ssid)
{
    if (!isMdnsSafeHostname(ssid)) return false;
    char* s = apSsid();
    if (!s) return false;
    ssid.toCharArray(s, WIFI_IDENTITY_AP_SSID_MAX_LEN + 1);
    return true;
}

String wifiMdnsHostText()
{
    char* s = apSsid();
    String host = s ? String(s) : String("");
    host.toLowerCase();
    return host;
}

String wifiMdnsUrlText()
{
    return String("http://") + wifiMdnsHostText() + ".local/";
}
#endif

// ============================================================================
// Section: WifiStaConfig.cpp
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE
#if __has_include("WirelessSecrets.h")
#endif
#ifndef WIFI_STA_SSID
#define WIFI_STA_SSID ""
#endif
#ifndef WIFI_STA_PASSWORD
#define WIFI_STA_PASSWORD ""
#endif
static const uint8_t WIFI_STA_CONFIG_SSID_MAX_LEN = 32;
static const uint8_t WIFI_STA_CONFIG_PASSWORD_MAX_LEN = 63;
static const uint8_t WIFI_STA_CONFIG_PASSWORD_MIN_LEN = 8;
static const unsigned long WIFI_STA_CONFIG_APPLY_DELAY_MS = 800;
static const char* WIFI_STA_CONFIG_PREF_NAMESPACE = "mus4";
static const char* WIFI_STA_CONFIG_PREF_ENABLED_KEY = "sta_en";
static const char* WIFI_STA_CONFIG_PREF_SSID_KEY = "sta_ssid";
static const char* WIFI_STA_CONFIG_PREF_PASSWORD_KEY = "sta_pass";

static WifiRuntimeState* g_ws = nullptr;

extern void applyWifiStaCredentials();
extern void clearWifiStaHandoff();

void setWifiRuntimeState(WifiRuntimeState& ws)
{
    g_ws = &ws;
}

static inline WifiRuntimeState& ws()
{
    return *g_ws;
}

bool copyWifiStaSsid(const String& ssid)
{
    if (ssid.length() == 0 || ssid.length() > WIFI_STA_CONFIG_SSID_MAX_LEN) return false;
    ssid.toCharArray(ws().staSsid, WIFI_STA_CONFIG_SSID_MAX_LEN + 1);
    return true;
}

bool copyWifiStaPassword(const String& password)
{
    if (password.length() > 0 && (password.length() < WIFI_STA_CONFIG_PASSWORD_MIN_LEN || password.length() > WIFI_STA_CONFIG_PASSWORD_MAX_LEN)) return false;
    password.toCharArray(ws().staPassword, WIFI_STA_CONFIG_PASSWORD_MAX_LEN + 1);
    ws().staPasswordSet = password.length() > 0;
    return true;
}

String wifiStaIpText()
{
    return ws().staConnected ? WiFi.localIP().toString() : String("0.0.0.0");
}

void clearWifiStaLastError()
{
    ws().staLastError[0] = 0;
    ws().staLastErrorMessage[0] = 0;
}

void setWifiStaLastError(const char* code, const char* message, bool timedOut)
{
    // 保留本轮连接的首个失败原因，避免后续瞬态状态覆盖更有诊断价值的根因。
    if (ws().staLastError[0] != 0) return;
    snprintf(ws().staLastError, 24, "%s", code);
    snprintf(ws().staLastErrorMessage, 128, "%s", message);
    ws().staTimedOut = timedOut;
    ws().staConnecting = false;
    ws().staConnected = false;
    mus4Logf("wifi", "STA failed: %s", code);
}

void scheduleWifiStaApply()
{
    ws().staApplyPending = true;
    ws().staApplyDeadlineMs = millis() + WIFI_STA_CONFIG_APPLY_DELAY_MS;
}

bool saveWifiStaPreference(const String& ssid, const String& password)
{
    if (!copyWifiStaSsid(ssid) || !copyWifiStaPassword(password)) return false;
    if (!ws().prefs->begin(WIFI_STA_CONFIG_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = ws().prefs->putBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, true);
    size_t ssidWritten = ws().prefs->putString(WIFI_STA_CONFIG_PREF_SSID_KEY, ws().staSsid);
    size_t passwordWritten = ws().prefs->putString(WIFI_STA_CONFIG_PREF_PASSWORD_KEY, ws().staPassword);
    ws().prefs->end();
    if (enabledWritten == 0 || ssidWritten == 0 || (ws().staPasswordSet && passwordWritten == 0)) return false;
    ws().staConfigured = true;
    return true;
}

bool saveWifiStaSsidPreference(const String& ssid)
{
    if (!copyWifiStaSsid(ssid)) return false;
    if (!ws().prefs->begin(WIFI_STA_CONFIG_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = ws().prefs->putBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, true);
    size_t ssidWritten = ws().prefs->putString(WIFI_STA_CONFIG_PREF_SSID_KEY, ws().staSsid);
    ws().prefs->end();
    if (enabledWritten == 0 || ssidWritten == 0) return false;
    ws().staConfigured = true;
    return true;
}

bool saveWifiStaPasswordPreference(const String& password)
{
    if (!copyWifiStaPassword(password)) return false;
    if (!ws().prefs->begin(WIFI_STA_CONFIG_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = ws().prefs->putBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, true);
    size_t passwordWritten = ws().prefs->putString(WIFI_STA_CONFIG_PREF_PASSWORD_KEY, ws().staPassword);
    ws().prefs->end();
    if (enabledWritten == 0 || (ws().staPasswordSet && passwordWritten == 0)) return false;
    return true;
}

void clearWifiStaRuntimeStateWithoutDisconnect()
{
    ws().staSsid[0] = 0;
    ws().staPassword[0] = 0;
    ws().staPasswordSet = false;
    ws().staConfigured = false;
    ws().staConnected = false;
    ws().staTimedOut = false;
    ws().staConnecting = false;
    clearWifiStaLastError();
    ws().staApplyPending = false;
    clearWifiStaHandoff();
}

bool clearWifiStaPreference()
{
    if (!ws().prefs->begin(WIFI_STA_CONFIG_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = ws().prefs->putBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, false);
    ws().prefs->remove(WIFI_STA_CONFIG_PREF_SSID_KEY);
    ws().prefs->remove(WIFI_STA_CONFIG_PREF_PASSWORD_KEY);
    ws().prefs->end();
    if (enabledWritten == 0) return false;
    clearWifiStaRuntimeStateWithoutDisconnect();
    return true;
}

void loadWifiStaPreference()
{
    ws().staSsid[0] = 0;
    ws().staPassword[0] = 0;
    ws().staPasswordSet = false;
    ws().staConfigured = false;
    if (!ws().prefs->begin(WIFI_STA_CONFIG_PREF_NAMESPACE, true)) {
        copyWifiStaSsid(String(WIFI_STA_SSID));
        copyWifiStaPassword(String(WIFI_STA_PASSWORD));
        ws().staConfigured = strlen(ws().staSsid) > 0;
        mus4LogLine("wifi", "STA config load failed, using build defaults");
        return;
    }
    bool hasStaEnabled = ws().prefs->isKey(WIFI_STA_CONFIG_PREF_ENABLED_KEY);
    bool staEnabled = ws().prefs->getBool(WIFI_STA_CONFIG_PREF_ENABLED_KEY, false);
    String ssid = hasStaEnabled && staEnabled ? ws().prefs->getString(WIFI_STA_CONFIG_PREF_SSID_KEY, "") : String(WIFI_STA_SSID);
    String password = hasStaEnabled && staEnabled ? ws().prefs->getString(WIFI_STA_CONFIG_PREF_PASSWORD_KEY, "") : String(WIFI_STA_PASSWORD);
    ws().prefs->end();
    if (hasStaEnabled && !staEnabled) {
        mus4LogLine("wifi", "STA disabled by preference");
        return;
    }
    if (copyWifiStaSsid(ssid) && copyWifiStaPassword(password)) {
        ws().staConfigured = strlen(ws().staSsid) > 0;
    } else {
        ws().staSsid[0] = 0;
        ws().staPassword[0] = 0;
        ws().staPasswordSet = false;
        ws().staConfigured = false;
        mus4LogLine("wifi", "STA config invalid");
    }
}

void printWifiStaStatus(Print& out)
{
    out.printf("WIFI_STA configured=%d connected=%d timed_out=%d connecting=%d ssid=\"%s\" password_set=%d ap_ip=%s sta_ip=%s last_error=\"%s\" last_error_message=\"%s\"\n",
        ws().staConfigured ? 1 : 0,
        ws().staConnected ? 1 : 0,
        ws().staTimedOut ? 1 : 0,
        ws().staConnecting ? 1 : 0,
        ws().staSsid,
        ws().staPasswordSet ? 1 : 0,
        WiFi.softAPIP().toString().c_str(),
        wifiStaIpText().c_str(),
        ws().staLastError,
        ws().staLastErrorMessage);
}

bool processWifiStaConfigCommand(const String& line, Print& out, WifiRuntimeState& /*ws*/)
{
    if (line.equalsIgnoreCase("WIFI_STA_STATUS")) {
        printWifiStaStatus(out);
        return true;
    }
    if (line.startsWith("WIFI_STA_SSID:")) {
        String ssid = line.substring(14);
        ssid.trim();
        if (!saveWifiStaSsidPreference(ssid)) {
            out.println("NACK:WIFI_STA_SSID");
            return true;
        }
        out.printf("WIFI_STA_SSID_SAVED configured=%d\n", ws().staConfigured ? 1 : 0);
        return true;
    }
    if (line.startsWith("WIFI_STA_PASSWORD:")) {
        String password = line.substring(18);
        if (!saveWifiStaPasswordPreference(password)) {
            out.println("NACK:WIFI_STA_PASSWORD");
            return true;
        }
        out.printf("WIFI_STA_PASSWORD_SAVED password_set=%d\n", ws().staPasswordSet ? 1 : 0);
        return true;
    }
    if (line.equalsIgnoreCase("WIFI_STA_APPLY")) {
        if (!ws().staConfigured) {
            out.println("NACK:WIFI_STA_NOT_CONFIGURED");
            return true;
        }
        applyWifiStaCredentials();
        out.printf("WIFI_STA_APPLY_OK ssid=\"%s\"\n", ws().staSsid);
        return true;
    }
    if (line.equalsIgnoreCase("WIFI_STA_CLEAR")) {
        if (!clearWifiStaPreference()) {
            out.println("NACK:WIFI_STA_CLEAR");
            return true;
        }
        out.println("WIFI_STA_CLEARED");
        return true;
    }
    return false;
}
#endif

// ============================================================================
// Section: WifiOta.cpp
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE
// WIFI_CONSOLE_AP_PASSWORD, WIFI_OTA_PORT and WIFI_OTA_WINDOW_MS are defined
// in WifiConsoleTypes.h (included via RuntimeState.h -> WifiConsoleTypes.h).

extern SerialBuf wifiConsoleBuf;
extern ControlData rc_data;
extern ControlData car_output;

extern void ensureWifiOtaStarted();

void forceWifiOtaParkLocked()
{
    rc_data.park = PARK_LOCKED;
    car_output.park = PARK_LOCKED;
    car_output.throttle = 0;
}

void keepDevModeOtaWindowActive(OtaRuntimeState& os, WifiRuntimeState& ws)
{
    if (!ws.devModeEnabled) return;
    ensureWifiOtaStarted();
    os.windowOpen = true;
    os.deadlineMs = millis() + WIFI_OTA_WINDOW_MS;
}

bool shouldEmitSerial1Telemetry(OtaRuntimeState& os)
{
    return !os.inProgress;
}

void openWifiOtaWindow(Print& out, WirelessCommandOrigin origin, OtaRuntimeState& os, WifiRuntimeState& ws)
{
    bool webDevMode = ws.devModeEnabled && origin == WIRELESS_ORIGIN_WEB;
    if (!webDevMode && !ws.consoleAuthenticated) {
        out.println("NACK:AUTH_REQUIRED");
        wifiConsoleBuf.errors++;
        return;
    }
    if (car_output.park != PARK_LOCKED) {
        out.println("NACK:PARK_REQUIRED");
        wifiConsoleBuf.errors++;
        return;
    }
    os.parkGuardActive = true;
    forceWifiOtaParkLocked();
    ensureWifiOtaStarted();
    os.windowOpen = true;
    os.deadlineMs = millis() + WIFI_OTA_WINDOW_MS;
    os.lastProgressPct = 0;
    out.printf("OTA_READY ip=%s port=%u ttl_ms=%lu\n", WiFi.localIP().toString().c_str(), WIFI_OTA_PORT, WIFI_OTA_WINDOW_MS);
    mus4LogLine("ota", webDevMode ? "ready: web_dev" : "ready");
}

void openLocalWifiOtaWindow(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os)
{
    if (!line.substring(11).equals(WIFI_CONSOLE_AP_PASSWORD)) {
        out.println("NACK:AUTH_REQUIRED");
        sb.errors++;
        return;
    }
    os.parkGuardActive = true;
    forceWifiOtaParkLocked();
    ensureWifiOtaStarted();
    os.windowOpen = true;
    os.deadlineMs = millis() + WIFI_OTA_WINDOW_MS;
    os.lastProgressPct = 0;
    out.printf("OTA_READY ip=%s port=%u ttl_ms=%lu\n", WiFi.localIP().toString().c_str(), WIFI_OTA_PORT, WIFI_OTA_WINDOW_MS);
    mus4LogLine("ota", "ready: local");
}

bool processLocalOtaMaintenanceCommand(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os, WifiRuntimeState& ws)
{
    if (isLocalOtaOpenCommand(line)) {
        openLocalWifiOtaWindow(line, out, sb, os);
        return true;
    }
    if (isWirelessOtaStatusCommand(line)) {
        printWifiOtaStatus(out, os, ws);
        return true;
    }
    if (isWirelessOtaCloseCommand(line)) {
        closeWifiOtaWindow("LOCAL", os);
        out.println("OTA_CLOSED");
        return true;
    }
    return false;
}

void updateWifiOta(OtaRuntimeState& os, WifiRuntimeState& ws)
{
    if (ws.devModeEnabled) keepDevModeOtaWindowActive(os, ws);
    if (!os.windowOpen) return;
    if (os.inProgress || os.parkGuardActive) {
        forceWifiOtaParkLocked();
    }
    unsigned long now = millis();
    if (!ws.devModeEnabled && !os.inProgress && (long)(now - os.deadlineMs) >= 0) {
        closeWifiOtaWindow("TIMEOUT", os);
        return;
    }
    ArduinoOTA.handle();
}

void closeWifiOtaWindow(const char* reason, OtaRuntimeState& os)
{
    os.windowOpen = false;
    os.deadlineMs = 0;
    os.inProgress = false;
    os.parkGuardActive = false;
    os.lastProgressPct = 0;
    if (os.started) {
        ArduinoOTA.end();
        os.started = false;
    }
    mus4LogLine("ota", String("closed: ") + reason);
}

unsigned long wifiOtaTtlMs(OtaRuntimeState& os, WifiRuntimeState& ws)
{
    if (!os.windowOpen) return 0;
    if (ws.devModeEnabled) return WIFI_OTA_WINDOW_MS;
    unsigned long now = millis();
    if ((long)(os.deadlineMs - now) <= 0) return 0;
    return os.deadlineMs - now;
}

void printWifiOtaStatus(Print& out, OtaRuntimeState& os, WifiRuntimeState& ws)
{
    out.printf("OTA_STATUS started=%d window=%d in_progress=%d ttl_ms=%lu progress=%u park=%d dev_mode=%d park_guard=%d\n",
        os.started ? 1 : 0,
        os.windowOpen ? 1 : 0,
        os.inProgress ? 1 : 0,
        wifiOtaTtlMs(os, ws),
        os.lastProgressPct,
        car_output.park ? 1 : 0,
        ws.devModeEnabled ? 1 : 0,
        os.parkGuardActive ? 1 : 0);
}
#endif

// ============================================================================
// Section: WebLogBuffer.cpp
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE



// Compact entry for high-rate Serial1 telemetry (e.g. "T0:S0").  Kept in a
// separate ring buffer so that 500 Hz telemetry does not evict general web/
// serial/command logs from the shared 64-slot buffer.
struct Serial1WebLogEntry {
    uint32_t seq;
    unsigned long t;
    char line[16];
};

static const char* canonicalWebLogSource(const char* source)
{
    if (strcmp(source, "serial") == 0 || strcmp(source, "serial1") == 0) {
        return source;
    }
    return "web";
}

static WebLogEntry s_webLogEntries[WIFI_WEB_LOG_CAPACITY];
static Serial1WebLogEntry s_serial1LogEntries[SERIAL1_WEB_LOG_CAPACITY];
static uint32_t s_webLogSeq = 0;
static uint32_t s_webLogDropped = 0;
static uint16_t s_webLogHead = 0;
static uint16_t s_webLogCount = 0;
static uint8_t s_serial1LogHead = 0;
static uint8_t s_serial1LogCount = 0;
static WebLogSocketSink s_webLogSocketSink = nullptr;

static void appendGeneralWebLog(const char* source, const String& line)
{
    const char* src = canonicalWebLogSource(source);
    WebLogEntry& entry = s_webLogEntries[s_webLogHead];
    entry.seq = ++s_webLogSeq;
    entry.t = millis();
    snprintf(entry.source, sizeof(entry.source), "%s", src);
    snprintf(entry.line, sizeof(entry.line), "%s", line.c_str());
    s_webLogHead = (s_webLogHead + 1) % WIFI_WEB_LOG_CAPACITY;
    if (s_webLogCount < WIFI_WEB_LOG_CAPACITY) {
        s_webLogCount++;
    } else {
        s_webLogDropped++;
    }
    if (s_webLogSocketSink) {
        s_webLogSocketSink(entry.seq, entry.t, src, entry.line);
    }
}

static void appendSerial1WebLog(const String& line)
{
    Serial1WebLogEntry& entry = s_serial1LogEntries[s_serial1LogHead];
    entry.seq = ++s_webLogSeq;
    entry.t = millis();
    snprintf(entry.line, sizeof(entry.line), "%s", line.c_str());
    s_serial1LogHead = (s_serial1LogHead + 1) % SERIAL1_WEB_LOG_CAPACITY;
    if (s_serial1LogCount < SERIAL1_WEB_LOG_CAPACITY) {
        s_serial1LogCount++;
    } else {
        s_webLogDropped++;
    }
    if (s_webLogSocketSink) {
        s_webLogSocketSink(entry.seq, entry.t, "serial1", entry.line);
    }
}

void webLogBufferInit()
{
    s_webLogSeq = 0;
    s_webLogDropped = 0;
    s_webLogHead = 0;
    s_webLogCount = 0;
    s_serial1LogHead = 0;
    s_serial1LogCount = 0;
    s_webLogSocketSink = nullptr;
    for (uint16_t i = 0; i < WIFI_WEB_LOG_CAPACITY; i++) {
        s_webLogEntries[i] = WebLogEntry{};
    }
    for (uint8_t i = 0; i < SERIAL1_WEB_LOG_CAPACITY; i++) {
        s_serial1LogEntries[i] = Serial1WebLogEntry{};
    }
}

void webLogBufferSetSocketSink(WebLogSocketSink sink)
{
    s_webLogSocketSink = sink;
}

void appendWebLog(const char* source, const String& line)
{
    if (strcmp(canonicalWebLogSource(source), "serial1") == 0) {
        appendSerial1WebLog(line);
    } else {
        appendGeneralWebLog(source, line);
    }
}

void appendWebLogLines(const char* source, const String& text)
{
    int start = 0;
    while (start < text.length()) {
        int end = text.indexOf('\n', start);
        if (end < 0) end = text.length();
        String line = text.substring(start, end);
        line.trim();
        if (line.length() > 0) appendWebLog(source, line);
        start = end + 1;
    }
}

uint32_t webLogBufferDropped()
{
    return s_webLogDropped;
}

static void appendGeneralEntryJson(String& response, WebLogEntry& entry, bool& first)
{
    if (!first) response += ',';
    first = false;
    response += "{\"seq\":";
    response += entry.seq;
    response += ",\"t\":";
    response += entry.t;
    response += ",\"src\":";
    appendJsonString(response, entry.source);
    response += ",\"line\":";
    appendJsonString(response, entry.line);
    response += '}';
}

static void appendSerial1EntryJson(String& response, Serial1WebLogEntry& entry, bool& first)
{
    if (!first) response += ',';
    first = false;
    response += "{\"seq\":";
    response += entry.seq;
    response += ",\"t\":";
    response += entry.t;
    response += ",\"src\":\"serial1\",\"line\":";
    appendJsonString(response, entry.line);
    response += '}';
}

void writeWebLogsJson(String& response, uint32_t since)
{
    response += "{\"dropped\":";
    response += s_webLogDropped;
    response += ',';
    response += '"';
    response += "entries";
    response += '"';
    response += ':';
    response += '[';

    uint16_t genPos = 0;
    uint8_t s1Pos = 0;
    bool first = true;

    while (genPos < s_webLogCount || s1Pos < s_serial1LogCount) {
        bool useGeneral;
        if (genPos >= s_webLogCount) {
            useGeneral = false;
        } else if (s1Pos >= s_serial1LogCount) {
            useGeneral = true;
        } else {
            uint16_t genIdx = (s_webLogHead + WIFI_WEB_LOG_CAPACITY - s_webLogCount + genPos) % WIFI_WEB_LOG_CAPACITY;
            uint8_t s1Idx = (s_serial1LogHead + SERIAL1_WEB_LOG_CAPACITY - s_serial1LogCount + s1Pos) % SERIAL1_WEB_LOG_CAPACITY;
            useGeneral = s_webLogEntries[genIdx].seq <= s_serial1LogEntries[s1Idx].seq;
        }

        if (useGeneral) {
            uint16_t genIdx = (s_webLogHead + WIFI_WEB_LOG_CAPACITY - s_webLogCount + genPos) % WIFI_WEB_LOG_CAPACITY;
            WebLogEntry& entry = s_webLogEntries[genIdx];
            genPos++;
            if (entry.seq <= since) continue;
            appendGeneralEntryJson(response, entry, first);
        } else {
            uint8_t s1Idx = (s_serial1LogHead + SERIAL1_WEB_LOG_CAPACITY - s_serial1LogCount + s1Pos) % SERIAL1_WEB_LOG_CAPACITY;
            Serial1WebLogEntry& entry = s_serial1LogEntries[s1Idx];
            s1Pos++;
            if (entry.seq <= since) continue;
            appendSerial1EntryJson(response, entry, first);
        }
    }

    response += "]}";
}

#endif

// ============================================================================
// Section: WebTelemetry.cpp
// ============================================================================
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY


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

static void sendWebLogToSocket(uint32_t seq, unsigned long t, const char* source, const char* line)
{
    if (!wifiWebSocketClientConnected || !wifiWebSocketClient) return;
    if (!wifiWebSocketClient->canSend() || wifiWebSocketClient->queueIsFull()) return;
    if (otaRuntime.inProgress) return;
    if (ESP.getFreeHeap() < WIFI_WEB_TELEMETRY_MIN_FREE_HEAP) return;

    wifiWebSocketPayload = "{\"type\":\"log\",\"seq\":";
    wifiWebSocketPayload += seq;
    wifiWebSocketPayload += ",\"t\":";
    wifiWebSocketPayload += t;
    wifiWebSocketPayload += ",\"src\":\"";
    wifiWebSocketPayload += source;
    wifiWebSocketPayload += "\",\"line\":";
    appendJsonString(wifiWebSocketPayload, line);
    wifiWebSocketPayload += '}';
    wifiWebSocketClient->text(wifiWebSocketPayload);
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
    webLogBufferSetSocketSink(sendWebLogToSocket);
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

// ============================================================================
// Section: WebConsoleServer.cpp
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE



// Hardware/framework globals defined in MUS4_FW.ino
extern WebServer wifiWebServer;

// Runtime state aggregates defined in MUS4_FW.ino
extern WifiRuntimeState wifiRuntime;
extern OtaRuntimeState otaRuntime;

// Wi-Fi runtime state aliases (kept in MUS4_FW.ino via reference aliases)
#define ws wifiRuntime
#define os otaRuntime

// Control / sensor globals defined in MUS4_FW.ino (to be migrated later)
extern ControlData car_output;
extern SerialBuf wifiConsoleBuf;

// Web telemetry globals defined in MUS4_FW.ino (to be migrated to WebTelemetry)
extern WebDataPoint wifiWebData[WIFI_WEB_DATA_CAPACITY];
extern uint32_t wifiWebDataSeq;
extern uint16_t wifiWebDataHead;
extern uint16_t wifiWebDataCount;
extern unsigned long lastWifiWebDataSampleMs;

#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
#endif

// Wi-Fi scan cache globals defined in MUS4_FW.ino (to be migrated with WebConsoleServer later)
extern WifiScanEntry wifiScanCache[16];
extern uint8_t wifiScanCacheCount;

// Wi-Fi runtime helpers still in MUS4_FW.ino (to be migrated to WifiManager in slice 4)
extern void startWifiStaHandoff(const String& targetSsid);
extern void clearWifiStaHandoff();
extern void scheduleWifiStaApply();
extern void scheduleWifiApRestart();
extern bool saveDevModePreference(bool enabled);
extern bool saveWifiApPreference(const String& ssid);

// Web telemetry sampler still in MUS4_FW.ino (to be migrated to WebTelemetry in slice 3)
extern void sampleWifiWebData();

// Web-local state (moved from MUS4_FW.ino)
static String wifiWebUpdateErrorMsg;
static size_t wifiWebUpdateReceived = 0;
static unsigned long lastWifiWebUpdateMs = 0;
static uint32_t wifiWebUpdateMaxDtMs = 0;
static uint32_t wifiWebSampleMaxDtMs = 0;
static uint32_t wifiWebHttpMaxDtMs = 0;
static uint32_t wifiWebStatusRequests = 0;
static uint32_t wifiWebLogRequests = 0;
static uint32_t wifiWebDataRequests = 0;
static uint32_t wifiWebCommandRequests = 0;
static uint32_t wifiWebStatusMaxDtMs = 0;
static uint32_t wifiWebLogMaxDtMs = 0;
static uint32_t wifiWebDataMaxDtMs = 0;
static uint32_t wifiWebCommandMaxDtMs = 0;

void printWirelessStatus(Print& out)
{
    out.printf("STATUS mode=%d park=%d throttle=%d steering=%d wifi_frames=%lu wifi_errors=%lu ota_window=%d ota_progress=%u ota_ttl_ms=%lu dev_mode=%d park_guard=%d version=%s build=\"%s %s\" web_port=%u free_heap=%lu min_free_heap=%lu ws_port=%u ws_client=%d ws_dropped=%lu ws_queue_full_skip=%lu ws_heap_skip=%lu ws_frames=%lu ws_max_backlog=%lu ws_connects=%lu ws_disconnects=%lu web_update_dt_max=%lu web_sample_dt_max=%lu web_http_dt_max=%lu web_ws_dt_max=%lu http_status_count=%lu http_log_count=%lu http_data_count=%lu http_cmd_count=%lu http_status_dt_max=%lu http_log_dt_max=%lu http_data_dt_max=%lu http_cmd_dt_max=%lu ap_ssid=\"%s\" ap_ip=%s ap_clients=%u sta_configured=%d sta_connected=%d sta_ssid=\"%s\" sta_ip=%s mdns_host=\"%s\" mdns_url=%s mdns_started=%d web_log_dropped=%lu\n",
        car_output.mode,
        car_output.park ? 1 : 0,
        car_output.throttle,
        car_output.steering,
        wifiConsoleBuf.frames,
        wifiConsoleBuf.errors,
        os.windowOpen ? 1 : 0,
        os.lastProgressPct,
        wifiOtaTtlMs(otaRuntime, wifiRuntime),
        wifiRuntime.devModeEnabled ? 1 : 0,
        otaRuntime.parkGuardActive ? 1 : 0,
        MUS4_FIRMWARE_VERSION,
        MUS4_BUILD_DATE,
        MUS4_BUILD_TIME,
        WIFI_WEB_CONSOLE_PORT,
        (unsigned long)ESP.getFreeHeap(),
        (unsigned long)WIFI_WEB_TELEMETRY_MIN_FREE_HEAP,
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
        WIFI_WEB_SOCKET_PORT,
        wifiWebSocketClientConnected ? 1 : 0,
        wifiWebSocketDroppedPoints,
        wifiWebSocketQueueFullSkips,
        wifiWebSocketHeapSkips,
        wifiWebSocketFramesSent,
        wifiWebSocketMaxBacklog,
        wifiWebSocketConnects,
        wifiWebSocketDisconnects,
#else
        0,
        0,
        0UL,
        0UL,
        0UL,
        0UL,
        0UL,
        0UL,
        0UL,
#endif
        wifiWebUpdateMaxDtMs,
        wifiWebSampleMaxDtMs,
        wifiWebHttpMaxDtMs,
        wifiWebSocketMaxDtMs,
        wifiWebStatusRequests,
        wifiWebLogRequests,
        wifiWebDataRequests,
        wifiWebCommandRequests,
        wifiWebStatusMaxDtMs,
        wifiWebLogMaxDtMs,
        wifiWebDataMaxDtMs,
        wifiWebCommandMaxDtMs,
        getActiveWifiApSsid().c_str(),
        WiFi.softAPIP().toString().c_str(),
        WiFi.softAPgetStationNum(),
        wifiRuntime.staConfigured ? 1 : 0,
        wifiRuntime.staConnected ? 1 : 0,
        wifiRuntime.staSsid,
        wifiStaIpText().c_str(),
        wifiMdnsHostText().c_str(),
        wifiMdnsUrlText().c_str(),
        wifiRuntime.mdnsStarted ? 1 : 0,
        (unsigned long)webLogBufferDropped());
}

static void redirectWifiWebCaptivePortalToRoot()
{
    String url = String("http://") + WiFi.softAPIP().toString() + "/";
    wifiWebServer.sendHeader("Cache-Control", "no-store");
    wifiWebServer.sendHeader("Location", url);
    wifiWebServer.send(302, "text/plain", "");
}

static void handleWifiWebRoot()
{
    wifiWebServer.send_P(200, "text/html", WIFI_WEB_CONSOLE_HTML);
}

static void handleWifiWebCaptivePortal()
{
    redirectWifiWebCaptivePortalToRoot();
}

static void handleWifiWebCaptivePortalRedirectPage()
{
    String host = wifiWebServer.hostHeader();
    if (host.length() == 0 || host.indexOf(WiFi.softAPIP().toString()) >= 0) {
        host = WiFi.softAPIP().toString();
    }
    String url = String("http://") + host + "/";
    String response = String("<!doctype html><html><head><meta charset=\"utf-8\">") +
        "<meta http-equiv=\"refresh\" content=\"0;url=" + url + "\">" +
        "<script>location.replace('" + url + "');</script></head>" +
        "<body><a href=\"" + url + "\">打开 Drifter Console</a></body></html>";
    wifiWebServer.sendHeader("Cache-Control", "no-store");
    wifiWebServer.send(200, "text/html", response);
}

static void handleWifiWebWindowsConnectTest()
{
    handleWifiWebCaptivePortal();
}

static void handleWifiWebWindowsNcsi()
{
    handleWifiWebCaptivePortal();
}

static void handleWifiWebCaptivePortalNotFound()
{
    String uri = wifiWebServer.uri();
    if (uri.startsWith("/api/")) {
        wifiWebServer.sendHeader("Cache-Control", "no-store");
        wifiWebServer.send(404, "application/json", "{\"error\":\"not_found\"}");
        return;
    }
    redirectWifiWebCaptivePortalToRoot();
}

static void recordWifiWebHandlerDt(unsigned long startedMs, uint32_t& maxDtMs)
{
    uint32_t dt = (uint32_t)(millis() - startedMs);
    if (dt > maxDtMs) maxDtMs = dt;
}

static void sendWifiWebApiHeaders()
{
    wifiWebServer.sendHeader("Cache-Control", "no-store");
}

static void handleWifiWebStatus()
{
    unsigned long startedMs = millis();
    wifiWebStatusRequests++;
    String response;
    StringPrint out(response);
    printWirelessStatus(out);
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "text/plain", response);
    recordWifiWebHandlerDt(startedMs, wifiWebStatusMaxDtMs);
}

static void handleWifiWebCommand()
{
    unsigned long startedMs = millis();
    wifiWebCommandRequests++;
    String line = wifiWebServer.arg("plain");
    line.trim();
    if (line.length() == 0) {
        sendWifiWebApiHeaders();
        wifiWebServer.send(400, "text/plain", "NACK:EMPTY\n");
        appendWebLog("web", "> <empty>");
        appendWebLog("cmd", "NACK:EMPTY");
        recordWifiWebHandlerDt(startedMs, wifiWebCommandMaxDtMs);
        return;
    }
    String target = wifiWebServer.arg("target");
    target.trim();
    String response;
    StringPrint out(response);
    if (target.equalsIgnoreCase("serial")) {
        appendWebLog("serial", String("> ") + redactWirelessConsoleLine(line));
        Serial.println(line);
        out.println("ACK:SERIAL");
        appendWebLog("serial", "ACK:SERIAL");
    } else if (target.equalsIgnoreCase("serial1")) {
        appendWebLog("serial1", String("> ") + redactWirelessConsoleLine(line));
        Serial1.println(line);
        out.println("ACK:SERIAL1");
        appendWebLog("serial1", "ACK:SERIAL1");
    } else {
        appendWebLog("web", String("> ") + redactWirelessConsoleLine(line));
        processWirelessConsoleLine(line, out, WIRELESS_ORIGIN_WEB);
        appendWebLogLines("web", response);
    }
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "text/plain", response);
    recordWifiWebHandlerDt(startedMs, wifiWebCommandMaxDtMs);
}

static void handleWifiWebDevMode()
{
    String response = String("{\"enabled\":") + (ws.devModeEnabled ? "true" : "false") + "}";
    wifiWebServer.send(200, "application/json", response);
}

static void handleWifiWebDevModeSet()
{
    String body = wifiWebServer.arg("plain");
    body.trim();
    body.toLowerCase();
    bool enabled;
    if (body == "1" || body == "true" || body == "on") {
        enabled = true;
    } else if (body == "0" || body == "false" || body == "off") {
        enabled = false;
    } else {
        wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_value\"}");
        return;
    }
    if (!saveDevModePreference(enabled)) {
        wifiWebServer.send(500, "application/json", "{\"saved\":false}");
        return;
    }
    String response = String("{\"enabled\":") + (ws.devModeEnabled ? "true" : "false") + ",\"saved\":true}";
    wifiWebServer.send(200, "application/json", response);
}

static String wifiApJson()
{
    String response;
    response.reserve(128);
    // Return the configured base SSID (<prefix>-ESP) so the config dialog
    // can edit the prefix. The live broadcast SSID may differ in DEV mode.
    response += "{\"ssid\":";
    appendJsonString(response, ws.apSsid);
    response += ",\"ip\":";
    appendJsonString(response, WiFi.softAPIP().toString().c_str());
    response += ",\"clients\":";
    response += WiFi.softAPgetStationNum();
    response += "}";
    return response;
}

static void handleWifiWebAp()
{
    wifiWebServer.send(200, "application/json", wifiApJson());
}

static void handleWifiWebApSet()
{
    if (!ws.consoleAuthenticated && !ws.devModeEnabled) {
        wifiWebServer.send(403, "application/json", "{\"error\":\"auth_required\"}");
        return;
    }
    String prefix = wifiWebServer.arg("ssid");
    prefix.trim();
    if (!isValidApSsidPrefix(prefix)) {
        wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_ssid\"}");
        return;
    }
    String fullSsid = prefix + WIFI_AP_SSID_SUFFIX;
    if (!saveWifiApPreference(fullSsid)) {
        wifiWebServer.send(500, "application/json", "{\"saved\":false}");
        return;
    }
    appendWebLog("web", String("wifi ap saved ssid=") + ws.apSsid);
    wifiWebServer.send(200, "application/json", String("{\"saved\":true,\"restart_pending\":true,\"state\":") + wifiApJson() + "}");
    scheduleWifiApRestart();
}

static String wifiStaJson()
{
    String response;
    response.reserve(320);
    response += "{\"configured\":";
    response += ws.staConfigured ? "true" : "false";
    response += ",\"connected\":";
    response += ws.staConnected ? "true" : "false";
    response += ",\"timed_out\":";
    response += ws.staTimedOut ? "true" : "false";
    response += ",\"connecting\":";
    response += ws.staConnecting ? "true" : "false";
    response += ",\"last_error\":";
    appendJsonString(response, ws.staConnected ? "" : ws.staLastError);
    response += ",\"last_error_message\":";
    appendJsonString(response, ws.staConnected ? "" : ws.staLastErrorMessage);
    response += ",\"ssid\":";
    appendJsonString(response, ws.staSsid);
    response += ",\"password_set\":";
    response += ws.staPasswordSet ? "true" : "false";
    response += ",\"password_len\":";
    response += ws.staPasswordSet ? strlen(ws.staPassword) : 0;
    response += ",\"ap_ip\":";
    appendJsonString(response, WiFi.softAPIP().toString().c_str());
    response += ",\"sta_ip\":";
    appendJsonString(response, wifiStaIpText().c_str());
    response += ",\"mdns_host\":";
    appendJsonString(response, wifiMdnsHostText().c_str());
    response += ",\"mdns_url\":";
    appendJsonString(response, wifiMdnsUrlText().c_str());
    response += ",\"mdns_started\":";
    response += ws.mdnsStarted ? "true" : "false";
    response += ",\"handoff_active\":";
    response += ws.staHandoffActive ? "true" : "false";
    response += ",\"handoff_target_ssid\":";
    appendJsonString(response, ws.staHandoffTargetSsid);
    response += ",\"handoff_sta_ip\":";
    appendJsonString(response, ws.staHandoffStaIp[0] ? ws.staHandoffStaIp : wifiStaIpText().c_str());
    response += ",\"handoff_ap_ssid\":";
    appendJsonString(response, ws.staHandoffApSsid[0] ? ws.staHandoffApSsid : getActiveWifiApSsid().c_str());
    response += ",\"handoff_ap_url\":";
    appendJsonString(response, "http://192.168.4.1/");
    response += ",\"handoff_mdns_url\":";
    appendJsonString(response, wifiMdnsUrlText().c_str());
    response += "}";
    return response;
}

static void handleWifiWebSta()
{
    wifiWebServer.send(200, "application/json", wifiStaJson());
}

static void handleWifiWebStaPassword()
{
    if (!ws.consoleAuthenticated && !ws.devModeEnabled) {
        wifiWebServer.send(403, "application/json", "{\"error\":\"auth_required\"}");
        return;
    }
    String response;
    response.reserve(128);
    response += "{\"password_set\":";
    response += ws.staPasswordSet ? "true" : "false";
    response += ",\"password_len\":";
    response += ws.staPasswordSet ? strlen(ws.staPassword) : 0;
    response += ",\"password\":";
    if (ws.staPasswordSet) appendJsonString(response, ws.staPassword);
    else appendJsonString(response, "");
    response += '}';
    wifiWebServer.send(200, "application/json", response);
}

static void startWifiStaScan()
{
    if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return;
    WiFi.scanNetworks(true, true);
}

static void cacheWifiStaScanResults(int count)
{
    wifiScanCacheCount = 0;
    for (int i = 0; i < count && wifiScanCacheCount < 16; i++) {
        String ssid = WiFi.SSID(i);
        ssid.trim();
        int32_t channel = WiFi.channel(i);
        if (ssid.length() == 0 || channel < 1 || channel > 14) continue;
        int32_t rssi = WiFi.RSSI(i);
        int existing = -1;
        for (uint8_t j = 0; j < wifiScanCacheCount; j++) {
            if (ssid.equals(wifiScanCache[j].ssid)) {
                existing = j;
                break;
            }
        }
        if (existing >= 0 && rssi <= wifiScanCache[existing].rssi) continue;
        WifiScanEntry& entry = existing >= 0 ? wifiScanCache[existing] : wifiScanCache[wifiScanCacheCount++];
        ssid.toCharArray(entry.ssid, sizeof(entry.ssid));
        entry.rssi = rssi;
        entry.channel = channel;
        entry.secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }
    for (uint8_t i = 0; i < wifiScanCacheCount; i++) {
        for (uint8_t j = i + 1; j < wifiScanCacheCount; j++) {
            if (wifiScanCache[j].rssi > wifiScanCache[i].rssi) {
                WifiScanEntry tmp = wifiScanCache[i];
                wifiScanCache[i] = wifiScanCache[j];
                wifiScanCache[j] = tmp;
            }
        }
    }
}

static void handleWifiWebStaScan()
{
    int result = WiFi.scanComplete();
    bool scanning = result == WIFI_SCAN_RUNNING;
    if (result >= 0) {
        cacheWifiStaScanResults(result);
        WiFi.scanDelete();
        startWifiStaScan();
        scanning = false;
    } else if (result != WIFI_SCAN_RUNNING) {
        startWifiStaScan();
        scanning = true;
    }
    String response;
    response.reserve(640);
    response += "{\"scanning\":";
    response += scanning ? "true" : "false";
    response += ",\"networks\":[";
    for (uint8_t i = 0; i < wifiScanCacheCount; i++) {
        if (i > 0) response += ',';
        response += "{\"ssid\":";
        appendJsonString(response, wifiScanCache[i].ssid);
        response += ",\"rssi\":";
        response += wifiScanCache[i].rssi;
        response += ",\"channel\":";
        response += wifiScanCache[i].channel;
        response += ",\"secure\":";
        response += wifiScanCache[i].secure ? "true" : "false";
        response += '}';
    }
    response += "]}";
    wifiWebServer.send(200, "application/json", response);
}

static void handleWifiWebStaSet()
{
    if (!ws.consoleAuthenticated && !ws.devModeEnabled) {
        wifiWebServer.send(403, "application/json", "{\"error\":\"auth_required\"}");
        return;
    }
    String ssid = wifiWebServer.arg("ssid");
    String password = wifiWebServer.arg("password");
    String sourceArg = wifiWebServer.arg("source");
    bool keepPassword = wifiWebServer.arg("keep_password") == "1";
    ssid.trim();
    if (ssid.length() == 0 || ssid.length() > WIFI_STA_SSID_MAX_LEN) {
        wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_ssid\"}");
        return;
    }
    bool staHandoffRequested = ws.staConnected && sourceArg == "sta" && !ssid.equals(WiFi.SSID());
    if (keepPassword) {
        if (!ws.staPasswordSet) {
            wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_password\"}");
            return;
        }
        if (!saveWifiStaSsidPreference(ssid)) {
            wifiWebServer.send(500, "application/json", "{\"saved\":false}");
            return;
        }
    } else {
        if (password.length() > 0 && (password.length() < WIFI_STA_PASSWORD_MIN_LEN || password.length() > WIFI_STA_PASSWORD_MAX_LEN)) {
            wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_password\"}");
            return;
        }
        if (!saveWifiStaPreference(ssid, password)) {
            wifiWebServer.send(500, "application/json", "{\"saved\":false}");
            return;
        }
    }
    if (staHandoffRequested) {
        startWifiStaHandoff(ssid);
    } else {
        clearWifiStaHandoff();
    }
    appendWebLog("web", String("wifi sta saved ssid=") + ws.staSsid + " password=<redacted>");
    wifiWebServer.send(200, "application/json", String("{\"saved\":true,\"applied\":true,\"state\":") + wifiStaJson() + "}");
    scheduleWifiStaApply();
}

static void handleWifiWebStaClear()
{
    if (!ws.consoleAuthenticated && !ws.devModeEnabled) {
        wifiWebServer.send(403, "application/json", "{\"error\":\"auth_required\"}");
        return;
    }
    if (!clearWifiStaPreference()) {
        wifiWebServer.send(500, "application/json", "{\"cleared\":false}");
        return;
    }
    appendWebLog("web", "wifi sta cleared");
    wifiWebServer.send(200, "application/json", "{\"cleared\":true}");
}

static void handleWifiWebLog()
{
    unsigned long startedMs = millis();
    wifiWebLogRequests++;
    uint32_t since = wifiWebServer.hasArg("since") ? (uint32_t)wifiWebServer.arg("since").toInt() : 0;
    String response;
    response.reserve(512);
    writeWebLogsJson(response, since);
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "application/json", response);
    recordWifiWebHandlerDt(startedMs, wifiWebLogMaxDtMs);
}

static void appendWifiWebPlotPointJson(String& response, WebDataPoint& point)
{
    response += "{\"seq\":";
    response += point.seq;
    response += ",\"t\":";
    response += point.t;
    response += ",\"dt\":";
    response += point.dtMs;
    response += ",\"thr\":";
    response += point.throttle;
    response += ",\"str\":";
    response += point.steering;
    response += ",\"gz\":";
    response += String(point.gyroZ, 3);
    response += '}';
}

static void appendWifiWebStateJson(String& response, WebDataPoint& point)
{
    response += "{\"seq\":";
    response += point.seq;
    response += ",\"t\":";
    response += point.t;
    response += ",\"dt\":";
    response += point.dtMs;
    response += ",\"thr\":";
    response += point.throttle;
    response += ",\"str\":";
    response += point.steering;
    response += ",\"mode\":";
    response += point.mode;
    response += ",\"park\":";
    response += point.park ? 1 : 0;
    response += ",\"rct\":";
    response += point.rcThrottle;
    response += ",\"rcs\":";
    response += point.rcSteering;
    response += ",\"ch1\":";
    response += point.rcChannels[CH_STEERING];
    response += ",\"ch2\":";
    response += point.rcChannels[CH_THROTTLE];
    response += ",\"ch3\":";
    response += point.rcChannels[CH_PARK];
    response += ",\"ch4\":";
    response += point.rcChannels[CH_MODE];
    response += ",\"ch5\":";
    response += point.rcChannels[CH_DRIFT];
    response += ",\"ch6\":";
    response += point.rcChannels[CH_DRIFT_SCALE];
    response += ",\"pt\":";
    response += point.pilotThrottle;
    response += ",\"ps\":";
    response += point.pilotSteering;
    response += ",\"cur\":";
    response += String(point.currentMa, 2);
    response += ",\"vol\":";
    response += String(point.voltage, 2);
    response += ",\"gz\":";
    response += String(point.gyroZ, 3);
    response += ",\"de\":";
    response += point.driftEnabled ? 1 : 0;
    response += ",\"da\":";
    response += point.driftActive ? 1 : 0;
    response += ",\"dc\":";
    response += String(point.driftCompensation, 2);
    response += ",\"gzf\":";
    response += String(point.gyroZFiltered, 3);
    response += '}';
}

static void handleWifiWebData()
{
    unsigned long startedMs = millis();
    wifiWebDataRequests++;
    uint32_t since = wifiWebServer.hasArg("since") ? (uint32_t)wifiWebServer.arg("since").toInt() : 0;
    String response;
    response.reserve(768);
    response += "{\"points\":[";
    bool first = true;
    for (uint16_t i = 0; i < wifiWebDataCount; i++) {
        uint16_t index = (wifiWebDataHead + WIFI_WEB_DATA_CAPACITY - wifiWebDataCount + i) % WIFI_WEB_DATA_CAPACITY;
        WebDataPoint& point = wifiWebData[index];
        if (point.seq <= since) continue;
        if (!first) response += ',';
        first = false;
        appendWifiWebPlotPointJson(response, point);
    }
    response += "],\"latest\":";
    if (wifiWebDataCount > 0) {
        uint16_t latestIndex = (wifiWebDataHead + WIFI_WEB_DATA_CAPACITY - 1) % WIFI_WEB_DATA_CAPACITY;
        appendWifiWebStateJson(response, wifiWebData[latestIndex]);
    } else {
        response += "null";
    }
    response += '}';
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "application/json", response);
    recordWifiWebHandlerDt(startedMs, wifiWebDataMaxDtMs);
}

static void handleWifiWebUpdateGet()
{
    wifiWebServer.send_P(200, "text/html", WIFI_WEB_UPDATE_HTML);
}

static bool isWifiWebUpdateAuthOk()
{
    if (ws.devModeEnabled) return true;
    if (ws.consoleAuthenticated) return true;
    // Allow one-shot auth via query parameter for scripted uploads
    if (wifiWebServer.hasArg("auth") && wifiWebServer.arg("auth").equals(WIFI_CONSOLE_AP_PASSWORD)) {
        return true;
    }
    return false;
}

static void handleWifiWebUpdateUpload()
{
    HTTPUpload& upload = wifiWebServer.upload();
    if (upload.status == UPLOAD_FILE_START) {
        wifiWebUpdateErrorMsg = "";
        wifiWebUpdateReceived = 0;
        if (!isWifiWebUpdateAuthOk()) {
            wifiWebUpdateErrorMsg = "NACK:AUTH_REQUIRED";
            mus4LogLine("ota", "http update rejected: auth required");
            return;
        }
        if (car_output.park != PARK_LOCKED) {
            wifiWebUpdateErrorMsg = "NACK:PARK_REQUIRED";
            mus4LogLine("ota", "http update rejected: park required");
            return;
        }
        os.parkGuardActive = true;
        forceWifiOtaParkLocked();
        os.inProgress = true;
        os.windowOpen = true;
        os.lastProgressPct = 0;
        if (!Update.begin(upload.totalSize > 0 ? upload.totalSize : UPDATE_SIZE_UNKNOWN)) {
            wifiWebUpdateErrorMsg = "NACK:BEGIN_FAILED:" + String(Update.errorString());
            mus4Logf("ota", "http update begin failed: %s", Update.errorString());
        } else {
            mus4LogLine("ota", "http update begin");
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (wifiWebUpdateErrorMsg.length() > 0) return;
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            wifiWebUpdateErrorMsg = "NACK:WRITE_FAILED";
            mus4Logf("ota", "http update write failed at %u", wifiWebUpdateReceived);
        } else {
            wifiWebUpdateReceived += upload.currentSize;
            if (upload.totalSize > 0) {
                os.lastProgressPct = (uint8_t)((wifiWebUpdateReceived * 100U) / upload.totalSize);
            }
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (wifiWebUpdateErrorMsg.length() > 0) {
            Update.end();
            return;
        }
        if (!Update.end(true)) {
            wifiWebUpdateErrorMsg = "NACK:END_FAILED:" + String(Update.errorString());
            mus4Logf("ota", "http update end failed: %s", Update.errorString());
        } else {
            os.lastProgressPct = 100;
            mus4LogLine("ota", "http update success");
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.end();
        wifiWebUpdateErrorMsg = "NACK:ABORTED";
        os.inProgress = false;
        mus4LogLine("ota", "http update aborted");
    }
}

static void handleWifiWebUpdatePost()
{
    unsigned long startedMs = millis();
    sendWifiWebApiHeaders();
    if (wifiWebUpdateErrorMsg.length() > 0) {
        wifiWebServer.send(500, "text/plain", wifiWebUpdateErrorMsg + "\n");
        recordWifiWebHandlerDt(startedMs, wifiWebHttpMaxDtMs);
        return;
    }
    wifiWebServer.send(200, "text/plain", "ACK:UPDATE_OK\n");
    recordWifiWebHandlerDt(startedMs, wifiWebHttpMaxDtMs);
    delay(100);
    ESP.restart();
}

void setupWebConsoleServer()
{
    wifiWebServer.on("/", HTTP_GET, handleWifiWebRoot);
    wifiWebServer.on("/connecttest.txt", HTTP_GET, handleWifiWebWindowsConnectTest);
    wifiWebServer.on("/ncsi.txt", HTTP_GET, handleWifiWebWindowsNcsi);
    wifiWebServer.on("/redirect", HTTP_GET, handleWifiWebCaptivePortalRedirectPage);
    wifiWebServer.on("/hotspot-detect.html", HTTP_GET, handleWifiWebCaptivePortal);
    wifiWebServer.on("/library/test/success.html", HTTP_GET, handleWifiWebCaptivePortal);
    wifiWebServer.on("/success.txt", HTTP_GET, handleWifiWebCaptivePortal);
    wifiWebServer.on("/generate_204", HTTP_GET, handleWifiWebCaptivePortal);
    wifiWebServer.on("/gen_204", HTTP_GET, handleWifiWebCaptivePortal);
    wifiWebServer.on("/mobile/status.php", HTTP_GET, handleWifiWebCaptivePortal);
    wifiWebServer.on("/connectivity-check.html", HTTP_GET, handleWifiWebCaptivePortal);
    wifiWebServer.on("/api/status", HTTP_GET, handleWifiWebStatus);
    wifiWebServer.on("/api/cmd", HTTP_POST, handleWifiWebCommand);
    wifiWebServer.on("/api/devmode", HTTP_GET, handleWifiWebDevMode);
    wifiWebServer.on("/api/devmode", HTTP_POST, handleWifiWebDevModeSet);
    wifiWebServer.on("/api/wifi-ap", HTTP_GET, handleWifiWebAp);
    wifiWebServer.on("/api/wifi-ap", HTTP_POST, handleWifiWebApSet);
    wifiWebServer.on("/api/wifi-sta", HTTP_GET, handleWifiWebSta);
    wifiWebServer.on("/api/wifi-sta", HTTP_POST, handleWifiWebStaSet);
    wifiWebServer.on("/api/wifi-sta/password", HTTP_GET, handleWifiWebStaPassword);
    wifiWebServer.on("/api/wifi-sta/scan", HTTP_GET, handleWifiWebStaScan);
    wifiWebServer.on("/api/wifi-sta/clear", HTTP_POST, handleWifiWebStaClear);
    wifiWebServer.on("/api/log", HTTP_GET, handleWifiWebLog);
    wifiWebServer.on("/api/data", HTTP_GET, handleWifiWebData);
    wifiWebServer.on("/update", HTTP_GET, handleWifiWebUpdateGet);
    wifiWebServer.on("/update", HTTP_POST, handleWifiWebUpdatePost, handleWifiWebUpdateUpload);
    wifiWebServer.onNotFound(handleWifiWebCaptivePortalNotFound);
    wifiWebServer.begin();
}

void updateWebConsoleServer()
{
    if (!ws.consoleStarted) return;
    unsigned long now = millis();
    if (lastWifiWebUpdateMs != 0) {
        uint32_t dt = (uint32_t)(now - lastWifiWebUpdateMs);
        if (dt > wifiWebUpdateMaxDtMs) wifiWebUpdateMaxDtMs = dt;
    }
    lastWifiWebUpdateMs = now;
    unsigned long stageStart = millis();
    sampleWifiWebData();
    uint32_t stageDt = (uint32_t)(millis() - stageStart);
    if (stageDt > wifiWebSampleMaxDtMs) wifiWebSampleMaxDtMs = stageDt;
    stageStart = millis();
    wifiWebServer.handleClient();
    stageDt = (uint32_t)(millis() - stageStart);
    if (stageDt > wifiWebHttpMaxDtMs) wifiWebHttpMaxDtMs = stageDt;
}

#endif

// ============================================================================
// Section: WifiManager.cpp
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE

#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
#endif
#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
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

