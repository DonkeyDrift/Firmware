//=============================================================
/* 
[Note]
1. Some pin definitions were adjusted for the MUS4-v2.4.2 PCB.
    - CH1_PIN 36 // Receiver PWM input CH1
    - CH2_PIN 39 // Receiver PWM input CH2
    - CH3_PIN 34 // Receiver PWM input CH3
    - CH4_PIN 26 // Receiver PWM input CH4
    - CH5_PIN 27 // Receiver PWM input CH5
    - CH6_PIN 35 // Receiver PWM input CH6
    - CH1_ST 23 // CH1 steering servo
    - CH2_TH 25 // CH2 throttle ESC
    - PWM_1 32 // PWM output channel 1
    - PWM_2 33 // PWM output channel 2

2. Mode selection and parking functions were disabled for receiver testing. [Note]

[Experience]
1. Firmware upload baud rate is 115200.
2. Serial protocol: T:S\n
  T means Throttle
  S means Steering
  Ends with "\n
*/

#include "FirmwareConfig.h"

#include <Wire.h>
#include <FastLED.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_INA219.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#endif
#include <ArduinoOTA.h>
#include <Update.h>
#include <Preferences.h>
#include "driver/mcpwm_cap.h"
#include "BuildInfo.h"
#include "SharedTypes.h"
#include "TUI.h"
#include "WebConsoleAssets.h"
#include "StringPrint.h"
#include "JsonUtil.h"
#include "I2CBusTools.h"
#include "LedStatus.h"
#include "Mus4Log.h"
#include "SteeringCalibration.h"
#include "Sensors.h"
#include "GamepadMode.h"
#include "RcFilter.h"
#include "CommandParser.h"
#include "CommandDispatcher.h"
#include "LocalCommands.h"
#include "SerialLineReader.h"
#include "WifiConsoleTypes.h"
#include "WirelessConsole.h"
#include "WifiStaConfig.h"
#include "WifiIdentity.h"
#include "DriftAssist.h"
#include "SteeringControl.h"
#include "Diagnostics.h"
#include "SerialBufferTypes.h"

#include "Buzzer.h"
// #include "test_runner.h"

TUI tui(Serial);
Buzzer buzzer(BUZZER_PIN);

int lastCarMode = -1;
bool lastParkState = false;

#ifdef ENABLE_GAMEPAD_MODE
  #include <BleGamepad.h>
  BleGamepad bleGamepad("Gamepad MU02", "Espressif", 100);
#endif

Adafruit_MPU6050 mpu;
Adafruit_INA219 ina219;

volatile uint16_t pwm_value[RC_CHANNEL_COUNT] = {0};
volatile unsigned long rise_time[RC_CHANNEL_COUNT] = {0};
volatile unsigned long last_valid_time[RC_CHANNEL_COUNT] = {0};
uint16_t pwm_filter_buf[RC_CHANNEL_COUNT][PWM_FILTER_SIZE] = {{0}};
uint8_t pwm_filter_idx[RC_CHANNEL_COUNT] = {0};
bool pwm_filter_initialized[RC_CHANNEL_COUNT] = {false};
uint16_t pwm_filtered[RC_CHANNEL_COUNT] = {0};
uint16_t aux_stable_pwm[RC_CHANNEL_COUNT] = {0};
uint16_t aux_candidate_pwm[RC_CHANNEL_COUNT] = {0};
uint8_t aux_candidate_count[RC_CHANNEL_COUNT] = {0};
bool aux_stable_initialized[RC_CHANNEL_COUNT] = {false};
uint16_t primary_smooth_pwm[RC_CHANNEL_COUNT] = {0};
bool primary_smooth_initialized[RC_CHANNEL_COUNT] = {false};
bool filterDebugEnabled = false;          // Debug output switch

const int Channels[RC_CHANNEL_COUNT] = {CH1_PIN, CH2_PIN, CH3_PIN, CH4_PIN, CH5_PIN, CH6_PIN};

CRGB leds[NUM_LEDS]; // Define the array of leds

// New global variables
unsigned long lastSensorUpdate = 0;
unsigned long lastRCDataUpdate = 0;
unsigned long lastRCFilterUpdate = 0;
unsigned long lastUIUpdate = 0;
unsigned long lastWaveUpdate = 0;     // Independent waveform refresh timer
const unsigned long WAVE_UPDATE_INTERVAL = 250; // 4Hz refresh rate
bool toggleActive = false;
CRGB toggleColor1, toggleColor2;
unsigned long toggleTime = 0;
unsigned long toggleInterval = 250; // LED toggle interval is 250 ms
bool degradeMode = false;
uint32_t degradeReason = 0;
bool ansiEnabled = true;
bool ansiDetected = false;            // Auto-detected ANSI support state
bool uiInitialized = false;
unsigned long uiIntervalCurrent = UI_UPDATE_INTERVAL;
const unsigned long uiIntervalMin = 100;
const unsigned long uiIntervalMax = 500;
unsigned long lastPerfEval = 0;
unsigned long lastUICycleDuration = 0;
unsigned long sensorTTL = 1000;
unsigned long rcTTL = 100;
unsigned long outputTTL = 100;
SerialBuf serial0Buf = {{0},0,0,0,false};
SerialBuf serial1Buf = {{0},0,0,0,false};
bool processLocalOtaMaintenanceCommand(const String& line, Print& out, SerialBuf& sb);
static bool shouldEmitSerial1Telemetry();
#ifdef ENABLE_WIFI_CONSOLE
static void ensureWifiOtaStarted();
static void closeWifiOtaWindow(const char* reason);
#if __has_include("WirelessSecrets.h")
#include "WirelessSecrets.h"
#endif
#ifndef WIFI_STA_SSID
#define WIFI_STA_SSID ""
#endif
#ifndef WIFI_STA_PASSWORD
#define WIFI_STA_PASSWORD ""
#endif
WiFiServer wifiConsoleServer(WIFI_CONSOLE_PORT);
WiFiClient wifiConsoleClient;
WebServer wifiWebServer(WIFI_WEB_CONSOLE_PORT);
DNSServer wifiCaptiveDnsServer;
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
AsyncWebServer wifiWebSocketServer(WIFI_WEB_SOCKET_PORT);
AsyncWebSocket wifiWebSocket("/");
#endif
SerialBuf wifiConsoleBuf = {{0},0,0,0,false};
WebLogEntry wifiWebLogs[WIFI_WEB_LOG_CAPACITY];
WifiScanEntry wifiScanCache[16];
uint8_t wifiScanCacheCount = 0;
WebDataPoint wifiWebData[WIFI_WEB_DATA_CAPACITY];
bool wifiConsoleStarted = false;
bool wifiConsoleAuthenticated = false;
bool wifiStaConfigured = false;
bool wifiStaConnected = false;
bool wifiStaTimedOut = false;
bool wifiStaConnecting = false;
char wifiStaLastError[24] = {0};
char wifiStaLastErrorMessage[128] = {0};
bool wifiStaApplyPending = false;
bool wifiApRestartPending = false;
bool wifiMdnsStarted = false;
bool wifiOtaStarted = false;
bool wifiOtaWindowOpen = false;
bool wifiOtaInProgress = false;
bool wifiOtaParkGuardActive = false;
bool wifiStaHandoffActive = false;
char wifiStaHandoffTargetSsid[WIFI_STA_SSID_MAX_LEN + 1] = {0};
char wifiStaHandoffStaIp[16] = {0};
char wifiStaHandoffApSsid[WIFI_AP_SSID_MAX_LEN + 1] = {0};
unsigned long wifiStaHandoffStartedMs = 0;
static bool wifiWebUpdateError = false;
static size_t wifiWebUpdateReceived = 0;

bool wifiDevModeEnabled = false;
char wifiApSsid[WIFI_AP_SSID_MAX_LEN + 1] = {0};
char wifiStaSsid[WIFI_STA_SSID_MAX_LEN + 1] = {0};
char wifiStaPassword[WIFI_STA_PASSWORD_MAX_LEN + 1] = {0};
bool wifiStaPasswordSet = false;
Preferences mus4Prefs;
unsigned long lastWifiConsoleStartAttemptMs = 0;
unsigned long wifiStaConnectStartMs = 0;
unsigned long wifiStaApplyDeadlineMs = 0;
unsigned long wifiApRestartDeadlineMs = 0;
unsigned long wifiOtaDeadlineMs = 0;
unsigned long lastWifiWebDataSampleMs = 0;
uint32_t wifiWebLogSeq = 0;
uint32_t wifiWebLogDropped = 0;
uint8_t wifiWebLogHead = 0;
uint8_t wifiWebLogCount = 0;
uint32_t wifiWebDataSeq = 0;
uint16_t wifiWebDataHead = 0;
uint16_t wifiWebDataCount = 0;
unsigned long lastWifiWebUpdateMs = 0;
uint32_t wifiWebUpdateMaxDtMs = 0;
uint32_t wifiWebSampleMaxDtMs = 0;
uint32_t wifiWebHttpMaxDtMs = 0;
uint32_t wifiWebSocketMaxDtMs = 0;
uint32_t wifiWebStatusRequests = 0;
uint32_t wifiWebLogRequests = 0;
uint32_t wifiWebDataRequests = 0;
uint32_t wifiWebCommandRequests = 0;
uint32_t wifiWebStatusMaxDtMs = 0;
uint32_t wifiWebLogMaxDtMs = 0;
uint32_t wifiWebDataMaxDtMs = 0;
uint32_t wifiWebCommandMaxDtMs = 0;
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
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
unsigned long lastWifiWebSocketPushMs = 0;
String wifiWebSocketPayload;
uint8_t wifiWebSocketBinaryPayload[256];
#endif
uint8_t wifiOtaLastProgressPct = 0;
#endif
int lastSeq = -1;                     // Last received sequence number

ControlData esp_now_data = {0, 0, 0, PARK_LOCKED}; // Initialize the structure at declaration
ControlData rc_data = {0, 0, 0, PARK_LOCKED};      // Initialize the structure at declaration
ControlData pilot_data = {0, 0, 0, PARK_LOCKED};   // Initialize the structure at declaration
ControlData car_output = {0, 0, 0, PARK_LOCKED};   // Initialize the structure at declaration

// 300Hz PWM output parameters (for servo and ESC)
// Frequency = 80MHz / (prescale * resolution)
// 300Hz = 80000000 / (prescale * 16384) → prescale ≈ 16
// Pulse-width calculation: count = (pulse_us / period_us) * 2^14
// Period = 1000000/300 = 3333.33µs
const int PWM_PERIOD_US = 3333;  // 300Hz period (µs)
const int PWM_MIN_V = 4915;      // 1000µs @ 300Hz (1000/3333.33×16384 ≈ 4915)
const int PWM_MAX_V = 9830;      // 2000µs @ 300Hz (2000/3333.33×16384 ≈ 9830)
const int MOTOR_MID_V = 7372;    // 1500µs @ 300Hz
const int MOTOR_RANGE_V = 2458; // ±500µs range
extern const int SERVO_MID_V = 7372;    // 1500µs @ 300Hz
extern const int SERVO_RANGE_V = 2458; // ±500µs range
const int MOTOR_OFFSET_V = 1;
const int SERVO_OFFSET_V = -1;

// Waveform data
int throttleWave[WAVE_WIDTH] = {0};
int steeringWave[WAVE_WIDTH] = {0};
int waveIndex = 0;

// Sensor data storage
SensorData ina219Data = {0}, mpu6050Data = {0};
uint8_t g_mpuCandidateAddress = 0;
uint8_t g_mpuWhoAmIValue = 0;
uint32_t g_i2cWorkingSpeed = I2C_SPEED;
uint8_t g_i2cScanAddresses[16] = {0};
uint8_t g_i2cScanCount = 0;
enum EmergencyStopState
{
    EST_IDLE,
    EST_READY,
    EST_BRAKING,
    EST_DONE
};
EmergencyStopState emergencyStopState = EST_IDLE;
unsigned long emergencyStopStartTime = 0;                 // Indicates whether braking is being prepared
const unsigned long EMERGENCY_STOP_READY_DURATION = 500;  // Brake preparation time: 500ms
const unsigned long EMERGENCY_STOP_BRAKE_DURATION = 1500; // Braking duration: 1500ms

// Park Control Variables
unsigned long parkBtnPressStartTime = 0;
bool parkBtnPressed = false;
bool parkActionTaken = false;
const unsigned long PARK_UNLOCK_HOLD_TIME = 1000; // 1s to Unlock
const unsigned long PARK_LOCK_HOLD_TIME = 500;    // 0.5s to Lock

#ifdef ENABLE_WIFI_CONSOLE
static unsigned long wifiOtaTtlMs()
{
    if (!wifiOtaWindowOpen) return 0;
    if (wifiDevModeEnabled) return WIFI_OTA_WINDOW_MS;
    unsigned long now = millis();
    if ((long)(wifiOtaDeadlineMs - now) <= 0) return 0;
    return wifiOtaDeadlineMs - now;
}

static bool shouldEmitSerial1Telemetry()
{
    return !wifiOtaWindowOpen && !wifiOtaInProgress;
}

static void forceWifiOtaParkLocked()
{
    rc_data.park = PARK_LOCKED;
    car_output.park = PARK_LOCKED;
    car_output.throttle = 0;
}

static void keepDevModeOtaWindowActive()
{
    if (!wifiDevModeEnabled) return;
    ensureWifiOtaStarted();
    wifiOtaWindowOpen = true;
    wifiOtaDeadlineMs = millis() + WIFI_OTA_WINDOW_MS;
}

static void loadDevModePreference()
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

static bool saveDevModePreference(bool enabled)
{
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    size_t written = mus4Prefs.putBool(MUS4_PREF_DEV_MODE_KEY, enabled);
    mus4Prefs.end();
    if (written == 0) return false;
    wifiDevModeEnabled = enabled;
    if (wifiDevModeEnabled) {
        keepDevModeOtaWindowActive();
    } else if (wifiOtaWindowOpen && !wifiOtaInProgress) {
        closeWifiOtaWindow("DEV_MODE_OFF");
    }
    mus4Logf("wifi", "dev_mode saved=%d", wifiDevModeEnabled ? 1 : 0);
    return true;
}


static void appendWifiWebLog(const char* source, const String& line)
{
    WebLogEntry& entry = wifiWebLogs[wifiWebLogHead];
    entry.seq = ++wifiWebLogSeq;
    entry.t = millis();
    snprintf(entry.source, sizeof(entry.source), "%s", source);
    snprintf(entry.line, sizeof(entry.line), "%s", line.c_str());
    wifiWebLogHead = (wifiWebLogHead + 1) % WIFI_WEB_LOG_CAPACITY;
    if (wifiWebLogCount < WIFI_WEB_LOG_CAPACITY) {
        wifiWebLogCount++;
    } else {
        wifiWebLogDropped++;
    }
}

static void appendWifiWebLogLines(const char* source, const String& text)
{
    int start = 0;
    while (start < text.length()) {
        int end = text.indexOf('\n', start);
        if (end < 0) end = text.length();
        String line = text.substring(start, end);
        line.trim();
        if (line.length() > 0) appendWifiWebLog(source, line);
        start = end + 1;
    }
}

static void startWifiMdnsIfNeeded()
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

static bool ensureWifiApAvailable();
static bool restartWifiAp();

static void stopWifiMdnsIfNeeded()
{
    if (!wifiMdnsStarted) return;
    MDNS.end();
    wifiMdnsStarted = false;
    mus4LogLine("wifi", "mDNS stopped");
}

static void clearWifiStaHandoff()
{
    wifiStaHandoffActive = false;
    wifiStaHandoffTargetSsid[0] = 0;
    wifiStaHandoffStaIp[0] = 0;
    wifiStaHandoffApSsid[0] = 0;
    wifiStaHandoffStartedMs = 0;
}

static void finishWifiStaHandoff()
{
    if (!wifiStaHandoffActive) return;
    snprintf(wifiStaHandoffStaIp, sizeof(wifiStaHandoffStaIp), "%s", WiFi.localIP().toString().c_str());
    mus4Logf("wifi", "STA handoff ready ssid=%s ip=%s", wifiStaHandoffTargetSsid, wifiStaHandoffStaIp);
}

static void startWifiStaHandoff(const String& targetSsid)
{
    wifiStaHandoffActive = true;
    targetSsid.toCharArray(wifiStaHandoffTargetSsid, sizeof(wifiStaHandoffTargetSsid));
    snprintf(wifiStaHandoffApSsid, sizeof(wifiStaHandoffApSsid), "%s", wifiApSsid);
    wifiStaHandoffStaIp[0] = 0;
    wifiStaHandoffStartedMs = millis();
    ensureWifiApAvailable();
    mus4Logf("wifi", "STA handoff started target=%s", wifiStaHandoffTargetSsid);
}

static void disconnectWifiStaOnly()
{
    esp_wifi_disconnect();
}

static void clearWifiStaRuntimeStateWithoutDisconnect()
{
    wifiStaSsid[0] = 0;
    wifiStaPassword[0] = 0;
    wifiStaPasswordSet = false;
    wifiStaConfigured = false;
    wifiStaConnected = false;
    wifiStaTimedOut = false;
    wifiStaConnecting = false;
    clearWifiStaLastError();
    wifiStaApplyPending = false;
    clearWifiStaHandoff();
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

static void scheduleWifiStaApply()
{
    wifiStaApplyPending = true;
    wifiStaApplyDeadlineMs = millis() + WIFI_STA_APPLY_DELAY_MS;
}

static void scheduleWifiApRestart()
{
    wifiApRestartPending = true;
    wifiApRestartDeadlineMs = millis() + WIFI_STA_APPLY_DELAY_MS;
}

static bool configureWifiSoftApNetwork()
{
    IPAddress apIp(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    return WiFi.softAPConfig(apIp, apIp, subnet);
}

static bool startWifiConsoleServices(const char* logPrefix)
{
    wifiCaptiveDnsServer.start(53, "*", WiFi.softAPIP());
    wifiConsoleServer.begin();
    wifiConsoleServer.setNoDelay(true);
    wifiWebServer.begin();
    wifiConsoleStarted = true;
    mus4Logf("wifi", "%s ssid=%s IP: %s", logPrefix, wifiApSsid, WiFi.softAPIP().toString().c_str());
    return true;
}

static bool startWifiApServices(const char* logPrefix)
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

static bool ensureWifiApAvailable()
{
    wifiApRestartPending = false;
    if (WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
        return startWifiApServices("AP ensured");
    }
    return startWifiConsoleServices("AP ensured");
}

static bool restartWifiAp()
{
    wifiApRestartPending = false;
    wifiCaptiveDnsServer.stop();
    WiFi.softAPdisconnect(true);
    delay(100);
    WiFi.mode(WIFI_AP_STA);
    return startWifiApServices("AP restarted");
}

static void loadWifiApPreference()
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

static bool saveWifiApPreference(const String& ssid)
{
    String trimmed = ssid;
    trimmed.trim();
    if (!copyWifiApSsid(trimmed)) return false;
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    size_t ssidWritten = mus4Prefs.putString(MUS4_PREF_AP_SSID_KEY, wifiApSsid);
    mus4Prefs.end();
    return ssidWritten > 0;
}

static bool saveWifiStaPreference(const String& ssid, const String& password)
{
    if (!copyWifiStaSsid(ssid) || !copyWifiStaPassword(password)) return false;
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = mus4Prefs.putBool(MUS4_PREF_STA_ENABLED_KEY, true);
    size_t ssidWritten = mus4Prefs.putString(MUS4_PREF_STA_SSID_KEY, wifiStaSsid);
    size_t passwordWritten = mus4Prefs.putString(MUS4_PREF_STA_PASSWORD_KEY, wifiStaPassword);
    mus4Prefs.end();
    if (enabledWritten == 0 || ssidWritten == 0 || (wifiStaPasswordSet && passwordWritten == 0)) return false;
    wifiStaConfigured = true;
    return true;
}

bool saveWifiStaSsidPreference(const String& ssid)
{
    if (!copyWifiStaSsid(ssid)) return false;
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = mus4Prefs.putBool(MUS4_PREF_STA_ENABLED_KEY, true);
    size_t ssidWritten = mus4Prefs.putString(MUS4_PREF_STA_SSID_KEY, wifiStaSsid);
    mus4Prefs.end();
    if (enabledWritten == 0 || ssidWritten == 0) return false;
    wifiStaConfigured = true;
    return true;
}

bool saveWifiStaPasswordPreference(const String& password)
{
    if (!copyWifiStaPassword(password)) return false;
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = mus4Prefs.putBool(MUS4_PREF_STA_ENABLED_KEY, true);
    size_t passwordWritten = mus4Prefs.putString(MUS4_PREF_STA_PASSWORD_KEY, wifiStaPassword);
    mus4Prefs.end();
    if (enabledWritten == 0 || (wifiStaPasswordSet && passwordWritten == 0)) return false;
    return true;
}

bool clearWifiStaPreference()
{
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = mus4Prefs.putBool(MUS4_PREF_STA_ENABLED_KEY, false);
    mus4Prefs.remove(MUS4_PREF_STA_SSID_KEY);
    mus4Prefs.remove(MUS4_PREF_STA_PASSWORD_KEY);
    mus4Prefs.end();
    if (enabledWritten == 0) return false;
    clearWifiStaRuntimeStateWithoutDisconnect();
    return true;
}

static void loadWifiStaPreference()
{
    wifiStaSsid[0] = 0;
    wifiStaPassword[0] = 0;
    wifiStaPasswordSet = false;
    wifiStaConfigured = false;
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, true)) {
        copyWifiStaSsid(String(WIFI_STA_SSID));
        copyWifiStaPassword(String(WIFI_STA_PASSWORD));
        wifiStaConfigured = strlen(wifiStaSsid) > 0;
        mus4LogLine("wifi", "STA config load failed, using build defaults");
        return;
    }
    bool hasStaEnabled = mus4Prefs.isKey(MUS4_PREF_STA_ENABLED_KEY);
    bool staEnabled = mus4Prefs.getBool(MUS4_PREF_STA_ENABLED_KEY, false);
    String ssid = hasStaEnabled && staEnabled ? mus4Prefs.getString(MUS4_PREF_STA_SSID_KEY, "") : String(WIFI_STA_SSID);
    String password = hasStaEnabled && staEnabled ? mus4Prefs.getString(MUS4_PREF_STA_PASSWORD_KEY, "") : String(WIFI_STA_PASSWORD);
    mus4Prefs.end();
    if (hasStaEnabled && !staEnabled) {
        mus4LogLine("wifi", "STA disabled by preference");
        return;
    }
    if (copyWifiStaSsid(ssid) && copyWifiStaPassword(password)) {
        wifiStaConfigured = strlen(wifiStaSsid) > 0;
    } else {
        wifiStaSsid[0] = 0;
        wifiStaPassword[0] = 0;
        wifiStaPasswordSet = false;
        wifiStaConfigured = false;
        mus4LogLine("wifi", "STA config invalid");
    }
}

#endif

#ifdef ENABLE_WIFI_CONSOLE
static void sampleWifiWebData()
{
    unsigned long now = millis();
    if (now - lastWifiWebDataSampleMs < WIFI_WEB_DATA_INTERVAL_MS) return;
    lastWifiWebDataSampleMs = now;
    WebDataPoint& point = wifiWebData[wifiWebDataHead];
    point.seq = ++wifiWebDataSeq;
    point.t = now;
    point.dtMs = wifiWebDataCount == 0 ? WIFI_WEB_DATA_INTERVAL_MS : (uint16_t)(now - wifiWebData[(wifiWebDataHead + WIFI_WEB_DATA_CAPACITY - 1) % WIFI_WEB_DATA_CAPACITY].t);
    point.throttle = car_output.throttle;
    point.steering = car_output.steering;
    point.mode = car_output.mode;
    point.park = car_output.park;
    point.rcThrottle = rc_data.throttle;
    point.rcSteering = rc_data.steering;
    for (uint8_t i = 0; i < RC_CHANNEL_COUNT; i++) {
        point.rcChannels[i] = pwm_filtered[i];
    }
    point.pilotThrottle = pilot_data.throttle;
    point.pilotSteering = pilot_data.steering;
    point.currentMa = ina219Data.current_mA;
    point.voltage = ina219Data.loadVoltage;
    point.gyroZ = mpu6050Data.gyroZ;
    point.driftEnabled = drift_assist_enabled;
    point.driftActive = drift_assist_active;
    point.driftCompensation = drift_compensation;
    point.gyroZFiltered = gyro_z_filtered;
    wifiWebDataHead = (wifiWebDataHead + 1) % WIFI_WEB_DATA_CAPACITY;
    if (wifiWebDataCount < WIFI_WEB_DATA_CAPACITY) wifiWebDataCount++;
}

static void printWifiOtaStatus(Print& out)
{
    out.printf("OTA_STATUS started=%d window=%d in_progress=%d ttl_ms=%lu progress=%u park=%d dev_mode=%d park_guard=%d\n",
        wifiOtaStarted ? 1 : 0,
        wifiOtaWindowOpen ? 1 : 0,
        wifiOtaInProgress ? 1 : 0,
        wifiOtaTtlMs(),
        wifiOtaLastProgressPct,
        car_output.park ? 1 : 0,
        wifiDevModeEnabled ? 1 : 0,
        wifiOtaParkGuardActive ? 1 : 0);
}

static void printWirelessStatus(Print& out)
{
    out.printf("STATUS mode=%d park=%d throttle=%d steering=%d wifi_frames=%lu wifi_errors=%lu ota_window=%d ota_progress=%u ota_ttl_ms=%lu dev_mode=%d park_guard=%d version=%s build=\"%s %s\" web_port=%u free_heap=%lu min_free_heap=%lu ws_port=%u ws_client=%d ws_dropped=%lu ws_queue_full_skip=%lu ws_heap_skip=%lu ws_frames=%lu ws_max_backlog=%lu ws_connects=%lu ws_disconnects=%lu web_update_dt_max=%lu web_sample_dt_max=%lu web_http_dt_max=%lu web_ws_dt_max=%lu http_status_count=%lu http_log_count=%lu http_data_count=%lu http_cmd_count=%lu http_status_dt_max=%lu http_log_dt_max=%lu http_data_dt_max=%lu http_cmd_dt_max=%lu ap_ssid=\"%s\" ap_ip=%s ap_clients=%u sta_configured=%d sta_connected=%d sta_ssid=\"%s\" sta_ip=%s mdns_host=\"%s\" mdns_url=%s mdns_started=%d\n",
        car_output.mode,
        car_output.park ? 1 : 0,
        car_output.throttle,
        car_output.steering,
        wifiConsoleBuf.frames,
        wifiConsoleBuf.errors,
        wifiOtaWindowOpen ? 1 : 0,
        wifiOtaLastProgressPct,
        wifiOtaTtlMs(),
        wifiDevModeEnabled ? 1 : 0,
        wifiOtaParkGuardActive ? 1 : 0,
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
        wifiApSsid,
        WiFi.softAPIP().toString().c_str(),
        WiFi.softAPgetStationNum(),
        wifiStaConfigured ? 1 : 0,
        wifiStaConnected ? 1 : 0,
        wifiStaSsid,
        wifiStaIpText().c_str(),
        wifiMdnsHostText().c_str(),
        wifiMdnsUrlText().c_str(),
        wifiMdnsStarted ? 1 : 0);
}

static void closeWifiOtaWindow(const char* reason)
{
    wifiOtaWindowOpen = false;
    wifiOtaDeadlineMs = 0;
    wifiOtaInProgress = false;
    wifiOtaParkGuardActive = false;
    wifiOtaLastProgressPct = 0;
    if (wifiOtaStarted) {
        ArduinoOTA.end();
        wifiOtaStarted = false;
    }
    mus4LogLine("ota", String("closed: ") + reason);
}

static void setupWifiOtaCallbacks()
{
    ArduinoOTA.setHostname(WIFI_OTA_HOSTNAME);
    ArduinoOTA.setPassword(WIFI_OTA_PASSWORD);
    ArduinoOTA.setPort(WIFI_OTA_PORT);
    ArduinoOTA.onStart([]() {
        wifiOtaInProgress = true;
        wifiOtaParkGuardActive = true;
        forceWifiOtaParkLocked();
        wifiOtaLastProgressPct = 0;
        mus4LogLine("ota", "start");
    });
    ArduinoOTA.onEnd([]() {
        wifiOtaInProgress = false;
        if (wifiDevModeEnabled) {
            wifiOtaParkGuardActive = false;
            keepDevModeOtaWindowActive();
        } else {
            wifiOtaWindowOpen = false;
            wifiOtaParkGuardActive = false;
            wifiOtaDeadlineMs = 0;
        }
        mus4LogLine("ota", "end");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        if (total > 0) wifiOtaLastProgressPct = (uint8_t)((progress * 100U) / total);
    });
    ArduinoOTA.onError([](ota_error_t error) {
        wifiOtaInProgress = false;
        if (wifiDevModeEnabled) {
            wifiOtaParkGuardActive = false;
            keepDevModeOtaWindowActive();
        } else {
            wifiOtaWindowOpen = false;
            wifiOtaParkGuardActive = false;
            wifiOtaDeadlineMs = 0;
        }
        mus4Logf("ota", "error: %u", error);
    });
}

static void ensureWifiOtaStarted()
{
    if (wifiOtaStarted) return;
    setupWifiOtaCallbacks();
    ArduinoOTA.begin();
    wifiOtaStarted = true;
}

static void openWifiOtaWindow(Print& out, WirelessCommandOrigin origin)
{
    bool webDevMode = wifiDevModeEnabled && origin == WIRELESS_ORIGIN_WEB;
    if (!webDevMode && !wifiConsoleAuthenticated) {
        out.println("NACK:AUTH_REQUIRED");
        wifiConsoleBuf.errors++;
        return;
    }
    if (car_output.park != PARK_LOCKED) {
        out.println("NACK:PARK_REQUIRED");
        wifiConsoleBuf.errors++;
        return;
    }
    wifiOtaParkGuardActive = true;
    forceWifiOtaParkLocked();
    ensureWifiOtaStarted();
    wifiOtaWindowOpen = true;
    wifiOtaDeadlineMs = millis() + WIFI_OTA_WINDOW_MS;
    wifiOtaLastProgressPct = 0;
    out.printf("OTA_READY ip=%s port=%u ttl_ms=%lu\n", WiFi.localIP().toString().c_str(), WIFI_OTA_PORT, WIFI_OTA_WINDOW_MS);
    mus4LogLine("ota", webDevMode ? "ready: web_dev" : "ready");
}

static void openLocalWifiOtaWindow(const String& line, Print& out, SerialBuf& sb)
{
    if (!line.substring(11).equals(WIFI_CONSOLE_AP_PASSWORD)) {
        out.println("NACK:AUTH_REQUIRED");
        sb.errors++;
        return;
    }
    wifiOtaParkGuardActive = true;
    forceWifiOtaParkLocked();
    ensureWifiOtaStarted();
    wifiOtaWindowOpen = true;
    wifiOtaDeadlineMs = millis() + WIFI_OTA_WINDOW_MS;
    wifiOtaLastProgressPct = 0;
    out.printf("OTA_READY ip=%s port=%u ttl_ms=%lu\n", WiFi.localIP().toString().c_str(), WIFI_OTA_PORT, WIFI_OTA_WINDOW_MS);
    mus4LogLine("ota", "ready: local");
}

bool processLocalOtaMaintenanceCommand(const String& line, Print& out, SerialBuf& sb)
{
    if (isLocalOtaOpenCommand(line)) {
        openLocalWifiOtaWindow(line, out, sb);
        return true;
    }
    if (isWirelessOtaStatusCommand(line)) {
        printWifiOtaStatus(out);
        return true;
    }
    if (isWirelessOtaCloseCommand(line)) {
        closeWifiOtaWindow("LOCAL");
        out.println("OTA_CLOSED");
        return true;
    }
    return false;
}

static void updateWifiOta()
{
    if (wifiDevModeEnabled) keepDevModeOtaWindowActive();
    if (!wifiOtaWindowOpen) return;
    if (wifiOtaInProgress || wifiOtaParkGuardActive) {
        forceWifiOtaParkLocked();
    }
    unsigned long now = millis();
    if (!wifiDevModeEnabled && !wifiOtaInProgress && (long)(now - wifiOtaDeadlineMs) >= 0) {
        closeWifiOtaWindow("TIMEOUT");
        return;
    }
    ArduinoOTA.handle();
}

static void processWirelessConsoleLine(const String& line, Print& out, WirelessCommandOrigin origin)
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
        wifiConsoleAuthenticated = line.substring(5).equals(WIFI_CONSOLE_AP_PASSWORD);
        out.println(wifiConsoleAuthenticated ? "AUTH_OK" : "AUTH_FAIL");
        return;
    }
    if (!isWirelessCommandAllowed(line, origin)) {
        bool webDevMode = wifiDevModeEnabled && origin == WIRELESS_ORIGIN_WEB;
        if (isParkLockedWirelessCommand(line) && car_output.park != PARK_LOCKED && (wifiConsoleAuthenticated || webDevMode)) {
            out.println("NACK:PARK_REQUIRED");
        } else {
            out.println("NACK:UNAUTHORIZED");
        }
        wifiConsoleBuf.errors++;
        return;
    }
    if (isWirelessOtaOpenCommand(line)) {
        openWifiOtaWindow(out, origin);
        return;
    }
    if (isWirelessOtaStatusCommand(line)) {
        printWifiOtaStatus(out);
        return;
    }
    if (isWirelessOtaCloseCommand(line)) {
        closeWifiOtaWindow("USER");
        out.println("OTA_CLOSED");
        return;
    }
    if (processWifiStaConfigCommand(line, out)) {
        return;
    }
    dispatchCommandLine(line, out, wifiConsoleBuf);
}





static void handleWifiWebRoot()
{
    wifiWebServer.send_P(200, "text/html", WIFI_WEB_CONSOLE_HTML);
}

static void redirectWifiWebCaptivePortalToRoot()
{
    String url = String("http://") + WiFi.softAPIP().toString() + "/";
    wifiWebServer.sendHeader("Cache-Control", "no-store");
    wifiWebServer.sendHeader("Location", url);
    wifiWebServer.send(302, "text/plain", "");
}

static void handleWifiWebCaptivePortal()
{
    redirectWifiWebCaptivePortalToRoot();
}

static void handleWifiWebCaptivePortalRedirectPage()
{
    String url = String("http://") + WiFi.softAPIP().toString() + "/";
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
        appendWifiWebLog("web", "> <empty>");
        appendWifiWebLog("cmd", "NACK:EMPTY");
        recordWifiWebHandlerDt(startedMs, wifiWebCommandMaxDtMs);
        return;
    }
    String response;
    StringPrint out(response);
    appendWifiWebLog("web", String("> ") + redactWirelessConsoleLine(line));
    processWirelessConsoleLine(line, out, WIRELESS_ORIGIN_WEB);
    appendWifiWebLogLines("cmd", response);
    sendWifiWebApiHeaders();
    wifiWebServer.send(200, "text/plain", response);
    recordWifiWebHandlerDt(startedMs, wifiWebCommandMaxDtMs);
}

static void handleWifiWebDevMode()
{
    String response = String("{\"enabled\":") + (wifiDevModeEnabled ? "true" : "false") + "}";
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
    String response = String("{\"enabled\":") + (wifiDevModeEnabled ? "true" : "false") + ",\"saved\":true}";
    wifiWebServer.send(200, "application/json", response);
}

static String wifiApJson()
{
    String response;
    response.reserve(128);
    response += "{\"ssid\":";
    appendJsonString(response, wifiApSsid);
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
    if (!wifiConsoleAuthenticated && !wifiDevModeEnabled) {
        wifiWebServer.send(403, "application/json", "{\"error\":\"auth_required\"}");
        return;
    }
    String ssid = wifiWebServer.arg("ssid");
    ssid.trim();
    if (ssid.length() == 0 || ssid.length() > WIFI_AP_SSID_MAX_LEN || !isMdnsSafeHostname(ssid)) {
        wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_ssid\"}");
        return;
    }
    if (!saveWifiApPreference(ssid)) {
        wifiWebServer.send(500, "application/json", "{\"saved\":false}");
        return;
    }
    appendWifiWebLog("web", String("wifi ap saved ssid=") + wifiApSsid);
    wifiWebServer.send(200, "application/json", String("{\"saved\":true,\"restart_pending\":true,\"state\":") + wifiApJson() + "}");
    scheduleWifiApRestart();
}

static String wifiStaJson()
{
    String response;
    response.reserve(320);
    response += "{\"configured\":";
    response += wifiStaConfigured ? "true" : "false";
    response += ",\"connected\":";
    response += wifiStaConnected ? "true" : "false";
    response += ",\"timed_out\":";
    response += wifiStaTimedOut ? "true" : "false";
    response += ",\"connecting\":";
    response += wifiStaConnecting ? "true" : "false";
    response += ",\"last_error\":";
    appendJsonString(response, wifiStaConnected ? "" : wifiStaLastError);
    response += ",\"last_error_message\":";
    appendJsonString(response, wifiStaConnected ? "" : wifiStaLastErrorMessage);
    response += ",\"ssid\":";
    appendJsonString(response, wifiStaSsid);
    response += ",\"password_set\":";
    response += wifiStaPasswordSet ? "true" : "false";
    response += ",\"password_len\":";
    response += wifiStaPasswordSet ? strlen(wifiStaPassword) : 0;
    response += ",\"ap_ip\":";
    appendJsonString(response, WiFi.softAPIP().toString().c_str());
    response += ",\"sta_ip\":";
    appendJsonString(response, wifiStaIpText().c_str());
    response += ",\"mdns_host\":";
    appendJsonString(response, wifiMdnsHostText().c_str());
    response += ",\"mdns_url\":";
    appendJsonString(response, wifiMdnsUrlText().c_str());
    response += ",\"mdns_started\":";
    response += wifiMdnsStarted ? "true" : "false";
    response += ",\"handoff_active\":";
    response += wifiStaHandoffActive ? "true" : "false";
    response += ",\"handoff_target_ssid\":";
    appendJsonString(response, wifiStaHandoffTargetSsid);
    response += ",\"handoff_sta_ip\":";
    appendJsonString(response, wifiStaHandoffStaIp[0] ? wifiStaHandoffStaIp : wifiStaIpText().c_str());
    response += ",\"handoff_ap_ssid\":";
    appendJsonString(response, wifiStaHandoffApSsid[0] ? wifiStaHandoffApSsid : wifiApSsid);
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
    if (!wifiConsoleAuthenticated && !wifiDevModeEnabled) {
        wifiWebServer.send(403, "application/json", "{\"error\":\"auth_required\"}");
        return;
    }
    String response;
    response.reserve(128);
    response += "{\"password_set\":";
    response += wifiStaPasswordSet ? "true" : "false";
    response += ",\"password_len\":";
    response += wifiStaPasswordSet ? strlen(wifiStaPassword) : 0;
    response += ",\"password\":";
    if (wifiStaPasswordSet) appendJsonString(response, wifiStaPassword);
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
    if (!wifiConsoleAuthenticated && !wifiDevModeEnabled) {
        wifiWebServer.send(403, "application/json", "{\"error\":\"auth_required\"}");
        return;
    }
    String ssid = wifiWebServer.arg("ssid");
    String password = wifiWebServer.arg("password");
    String source = wifiWebServer.arg("source");
    bool keepPassword = wifiWebServer.arg("keep_password") == "1";
    ssid.trim();
    if (ssid.length() == 0 || ssid.length() > WIFI_STA_SSID_MAX_LEN) {
        wifiWebServer.send(400, "application/json", "{\"error\":\"invalid_ssid\"}");
        return;
    }
    bool staHandoffRequested = wifiStaConnected && source == "sta" && !ssid.equals(WiFi.SSID());
    if (keepPassword) {
        if (!wifiStaPasswordSet) {
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
    appendWifiWebLog("web", String("wifi sta saved ssid=") + wifiStaSsid + " password=<redacted>");
    wifiWebServer.send(200, "application/json", String("{\"saved\":true,\"applied\":true,\"state\":") + wifiStaJson() + "}");
    scheduleWifiStaApply();
}

static void handleWifiWebStaClear()
{
    if (!wifiConsoleAuthenticated && !wifiDevModeEnabled) {
        wifiWebServer.send(403, "application/json", "{\"error\":\"auth_required\"}");
        return;
    }
    if (!clearWifiStaPreference()) {
        wifiWebServer.send(500, "application/json", "{\"cleared\":false}");
        return;
    }
    appendWifiWebLog("web", "wifi sta cleared");
    wifiWebServer.send(200, "application/json", "{\"cleared\":true}");
}

static void handleWifiWebLog()
{
    unsigned long startedMs = millis();
    wifiWebLogRequests++;
    uint32_t since = wifiWebServer.hasArg("since") ? (uint32_t)wifiWebServer.arg("since").toInt() : 0;
    String response;
    response.reserve(512);
    response += "{\"dropped\":";
    response += wifiWebLogDropped;
    response += ",\"entries\":[";
    bool first = true;
    for (uint8_t i = 0; i < wifiWebLogCount; i++) {
        uint8_t index = (wifiWebLogHead + WIFI_WEB_LOG_CAPACITY - wifiWebLogCount + i) % WIFI_WEB_LOG_CAPACITY;
        WebLogEntry& entry = wifiWebLogs[index];
        if (entry.seq <= since) continue;
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
    response += "]}";
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

static void handleWifiWebUpdateUpload()
{
    HTTPUpload& upload = wifiWebServer.upload();
    if (upload.status == UPLOAD_FILE_START) {
        wifiWebUpdateError = false;
        wifiWebUpdateReceived = 0;
        bool webDevMode = wifiDevModeEnabled;
        if (!webDevMode && !wifiConsoleAuthenticated) {
            wifiWebUpdateError = true;
            mus4LogLine("ota", "http update rejected: auth required");
            return;
        }
        if (car_output.park != PARK_LOCKED) {
            wifiWebUpdateError = true;
            mus4LogLine("ota", "http update rejected: park required");
            return;
        }
        wifiOtaParkGuardActive = true;
        forceWifiOtaParkLocked();
        wifiOtaInProgress = true;
        wifiOtaWindowOpen = true;
        wifiOtaLastProgressPct = 0;
        if (!Update.begin(upload.totalSize > 0 ? upload.totalSize : UPDATE_SIZE_UNKNOWN)) {
            wifiWebUpdateError = true;
            mus4Logf("ota", "http update begin failed: %s", Update.errorString());
        } else {
            mus4LogLine("ota", "http update begin");
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (wifiWebUpdateError) return;
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            wifiWebUpdateError = true;
            mus4Logf("ota", "http update write failed at %u", wifiWebUpdateReceived);
        } else {
            wifiWebUpdateReceived += upload.currentSize;
            if (upload.totalSize > 0) {
                wifiOtaLastProgressPct = (uint8_t)((wifiWebUpdateReceived * 100U) / upload.totalSize);
            }
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (wifiWebUpdateError) {
            Update.end();
            return;
        }
        if (!Update.end(true)) {
            wifiWebUpdateError = true;
            mus4Logf("ota", "http update end failed: %s", Update.errorString());
        } else {
            wifiOtaLastProgressPct = 100;
            mus4LogLine("ota", "http update success");
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.end();
        wifiWebUpdateError = true;
        wifiOtaInProgress = false;
        mus4LogLine("ota", "http update aborted");
    }
}

static void handleWifiWebUpdatePost()
{
    unsigned long startedMs = millis();
    sendWifiWebApiHeaders();
    if (wifiWebUpdateError) {
        wifiWebServer.send(500, "text/plain", "NACK:UPDATE_FAILED\n");
        recordWifiWebHandlerDt(startedMs, wifiWebHttpMaxDtMs);
        return;
    }
    wifiWebServer.send(200, "text/plain", "ACK:UPDATE_OK\n");
    recordWifiWebHandlerDt(startedMs, wifiWebHttpMaxDtMs);
    delay(100);
    ESP.restart();
}

#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
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
    if (wifiOtaInProgress) return;
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

static void setupWifiWebSocket()
{
    wifiWebSocketPayload.reserve(1536);
    wifiWebSocket.onEvent(handleWifiWebSocketEvent);
    wifiWebSocketServer.addHandler(&wifiWebSocket);
    wifiWebSocketServer.begin();
    mus4Logf("web", "ws telemetry port=%u", WIFI_WEB_SOCKET_PORT);
}

static void updateWifiWebSocket()
{
    wifiWebSocket.cleanupClients();
    pushWifiWebSocketData();
}
#endif

static void setupWifiWebConsole()
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
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
    setupWifiWebSocket();
#endif
}

static void updateWifiWebConsole()
{
    if (!wifiConsoleStarted) return;
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
    if (wifiApRestartPending && (long)(millis() - wifiApRestartDeadlineMs) >= 0) {
        restartWifiAp();
    }
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
    stageStart = millis();
    updateWifiWebSocket();
    stageDt = (uint32_t)(millis() - stageStart);
    if (stageDt > wifiWebSocketMaxDtMs) wifiWebSocketMaxDtMs = stageDt;
#endif
}

static void setupWifiConsole()
{
    mus4SetWebLogSink(appendWifiWebLog);
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

static void updateWifiSta()
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

static void updateWifiConsole()
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
            appendWifiWebLog("tcp", "client connected");
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
            appendWifiWebLog("tcp", String("> ") + redactWirelessConsoleLine(line));
            processWirelessConsoleLine(line, out, WIRELESS_ORIGIN_TCP);
            wifiConsoleClient.print(response);
            appendWifiWebLogLines("cmd", response);
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
#else
bool processLocalOtaMaintenanceCommand(const String& line, Print& out, SerialBuf& sb)
{
    return false;
}

static bool shouldEmitSerial1Telemetry()
{
    return true;
}
#endif


static void IRAM_ATTR acceptRcPulse(int channel, uint32_t width, unsigned long now)
{
    static uint16_t candidate_pwm[RC_CHANNEL_COUNT] = {0};
    static uint16_t large_change_count[RC_CHANNEL_COUNT] = {0};
    static uint16_t last_large_pwm[RC_CHANNEL_COUNT] = {0};

    if (width < RC_PWM_MIN || width > RC_PWM_MAX) return;

    uint16_t pulse = (uint16_t)width;
    uint16_t prev = pwm_value[channel];
    int diff = abs((int)pulse - (int)prev);

    if (diff <= 120) {
        pwm_value[channel] = pulse;
        last_valid_time[channel] = now;
    } else if (diff <= 200) {
        if (abs((int)pulse - (int)candidate_pwm[channel]) < 80) {
            pwm_value[channel] = pulse;
            last_valid_time[channel] = now;
        }
        candidate_pwm[channel] = pulse;
    } else {
        if (abs((int)pulse - (int)last_large_pwm[channel]) < 100) {
            large_change_count[channel]++;
            if (large_change_count[channel] >= 2) {
                pwm_value[channel] = pulse;
                last_valid_time[channel] = now;
                large_change_count[channel] = 0;
            }
        } else {
            large_change_count[channel] = 0;
        }
        last_large_pwm[channel] = pulse;
    }
}

void IRAM_ATTR handle_interrupt(int channel)
{
    static int pin_state[RC_CHANNEL_COUNT] = {0};
    static unsigned long last_edge_time[RC_CHANNEL_COUNT] = {0};
    static unsigned long last_rise_time[RC_CHANNEL_COUNT] = {0};

    unsigned long now = micros();
    if (now - last_edge_time[channel] < 100) return;
    last_edge_time[channel] = now;

    pin_state[channel] = digitalRead(Channels[channel]);
    if (pin_state[channel] == HIGH)
    {
        last_rise_time[channel] = now;
    }
    else
    {
        acceptRcPulse(channel, now - last_rise_time[channel], now);
    }
}

void IRAM_ATTR CH1_interrupt() { handle_interrupt(CH_STEERING); } // interrupt handler
void IRAM_ATTR CH2_interrupt() { handle_interrupt(CH_THROTTLE); }
void IRAM_ATTR CH3_interrupt() { handle_interrupt(CH_PARK); }
void IRAM_ATTR CH4_interrupt() { handle_interrupt(CH_MODE); }
void IRAM_ATTR CH5_interrupt() { handle_interrupt(CH_DRIFT); }
void IRAM_ATTR CH6_interrupt() { handle_interrupt(CH_DRIFT_SCALE); }

void (*isr_functions[RC_CHANNEL_COUNT])() = {CH1_interrupt, CH2_interrupt, CH3_interrupt, CH4_interrupt, CH5_interrupt, CH6_interrupt}; // array of function pointers

#if ENABLE_RC_MCPWM_CAPTURE
static mcpwm_cap_timer_handle_t rcMcpwmCaptureTimer = nullptr;
static mcpwm_cap_channel_handle_t rcModeCaptureChannel = nullptr;
static volatile uint32_t rcModeLastRiseTick = 0;
static volatile bool rcModeHasRiseTick = false;
static bool rcMcpwmCaptureActive = false;

static bool IRAM_ATTR onRcModeCapture(mcpwm_cap_channel_handle_t channel, const mcpwm_capture_event_data_t *edata, void *user_data)
{
    if (edata->cap_edge == MCPWM_CAP_EDGE_POS) {
        rcModeLastRiseTick = edata->cap_value;
        rcModeHasRiseTick = true;
    } else if (edata->cap_edge == MCPWM_CAP_EDGE_NEG && rcModeHasRiseTick) {
        uint32_t width = edata->cap_value - rcModeLastRiseTick;
        acceptRcPulse(CH_MODE, width, micros());
    }
    return false;
}

static bool setupRcMcpwmCapture()
{
    mcpwm_capture_timer_config_t timerConfig = {};
    timerConfig.group_id = RC_MCPWM_CAPTURE_GROUP_ID;
    timerConfig.clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT;
    timerConfig.resolution_hz = RC_MCPWM_CAPTURE_RESOLUTION_HZ;

    esp_err_t err = mcpwm_new_capture_timer(&timerConfig, &rcMcpwmCaptureTimer);
    if (err != ESP_OK) {
        mus4Logf("rc", "MCPWM timer init failed: %d", err);
        return false;
    }

    mcpwm_capture_channel_config_t channelConfig = {};
    channelConfig.gpio_num = CH4_PIN;
    channelConfig.prescale = 1;
    channelConfig.flags.pos_edge = true;
    channelConfig.flags.neg_edge = true;
    channelConfig.flags.pull_down = true;

    err = mcpwm_new_capture_channel(rcMcpwmCaptureTimer, &channelConfig, &rcModeCaptureChannel);
    if (err != ESP_OK) {
        mus4Logf("rc", "MCPWM CH4 channel init failed: %d", err);
        return false;
    }

    mcpwm_capture_event_callbacks_t callbacks = {};
    callbacks.on_cap = onRcModeCapture;
    err = mcpwm_capture_channel_register_event_callbacks(rcModeCaptureChannel, &callbacks, nullptr);
    if (err != ESP_OK) {
        mus4Logf("rc", "MCPWM CH4 callback init failed: %d", err);
        return false;
    }

    err = mcpwm_capture_channel_enable(rcModeCaptureChannel);
    if (err != ESP_OK) {
        mus4Logf("rc", "MCPWM CH4 channel enable failed: %d", err);
        return false;
    }

    err = mcpwm_capture_timer_enable(rcMcpwmCaptureTimer);
    if (err != ESP_OK) {
        mus4Logf("rc", "MCPWM timer enable failed: %d", err);
        return false;
    }

    err = mcpwm_capture_timer_start(rcMcpwmCaptureTimer);
    if (err != ESP_OK) {
        mus4Logf("rc", "MCPWM timer start failed: %d", err);
        return false;
    }

    mus4LogLine("rc", "MCPWM capture enabled for CH4");
    return true;
}
#endif

int User_throttle = 0;  // User throttle value from the RC transmitter
int User_steering = 0;  // User steering value from the RC transmitter
int Pilot_throttle = 0; // Throttle value from the host computer
int Pilot_steering = 0; // Steering value from the host computer

// RC calibration defaults moved to top of file (must precede function definitions for Arduino preprocessor compatibility)

int carOutputModeLast = -1;
unsigned long counter;

void emergencyStop()
{
    // If the park signal has been released, reset the state machine
    if (car_output.park == 0 && emergencyStopState == EST_DONE)
    {
        emergencyStopState = EST_IDLE;
        mus4LogLine("tui", "Emergency Stop FSM reset: Park unlocked");
        return;
    }

    switch (emergencyStopState)
    {
    // case default:
    case EST_IDLE:
        if (car_output.throttle > 0)
        {
            mus4LogLine("tui", "Start Emergency stop");
            car_output.throttle = 15;
            emergencyStopState = EST_READY;
            emergencyStopStartTime = millis();
        }
        else
        {
            emergencyStopState = EST_DONE;
        }

        break;

    case EST_READY:
        if (millis() - emergencyStopStartTime >= EMERGENCY_STOP_READY_DURATION)
        {
            car_output.throttle = -100;
            emergencyStopState = EST_BRAKING;
            emergencyStopStartTime = millis();
            mus4LogLine("tui", "Emergency STOP ready");
        }
        break;

    case EST_BRAKING:
        if (millis() - emergencyStopStartTime >= EMERGENCY_STOP_BRAKE_DURATION)
        {
            emergencyStopState = EST_DONE;
            mus4LogLine("tui", "Emergency STOP done");
        }
        break;

    case EST_DONE:
        // Braking is complete; reset throttle to zero
        car_output.throttle = 0;
        break;
    }
}

int adj(int v, int s) // v: value, s: step
{
    v = v + s;
    if (v > 4095)
        v = 4095;
    if (v < 0)
        v = 0;
    return v;
}

// Old TUI functions removed. Using TUI class.
// See TUI.h/cpp for implementation.


void park_change()
{
    // PWM > 1500 considered Pressed (Button value 2000)
    // PWM < 1500 considered Released (Button value 1000)
    bool isPressed = (pwm_filtered[CH_PARK] > 1500);

    if (isPressed)
    {
        if (!parkBtnPressed)
        {
            // Rising Edge: Start Timer
            parkBtnPressed = true;
            parkBtnPressStartTime = millis();
            parkActionTaken = false;
        }
        else
        {
            // Button Held
            if (!parkActionTaken)
            {
                unsigned long duration = millis() - parkBtnPressStartTime;

                if (rc_data.park)
                { // Currently Locked (Park Mode)
                    // Unlock Logic: Hold for 1s
                    if (duration >= PARK_UNLOCK_HOLD_TIME)
                    {
                        rc_data.park = false; // Unlock
                        emergencyStopState = EST_IDLE; // Reset Emergency Stop FSM
                        parkActionTaken = true;
                        mus4LogLine("tui", "System Unlocked: Park Mode Exited");
                        buzzer.playParkUnlockSound();
                    }
                }
                else
                { // Currently Unlocked (Drive Mode)
                    // Lock Logic: Hold for 0.5s
                    if (duration >= PARK_LOCK_HOLD_TIME)
                    {
                        rc_data.park = true; // Lock
                        parkActionTaken = true;
                        mus4LogLine("tui", "System Locked: Park Mode Entered");
                        buzzer.playParkLockSound();
                    }
                }
            }
        }
    }
    else
    {
        // Button Released
        parkBtnPressed = false;
        parkActionTaken = false;
    }

    car_output.park = rc_data.park;
}

void mode_change(bool modeValid) // Switch driving mode according to the RC mode value
{
    if (!modeValid) {
        return;
    }

    rc_data.mode = pwm_filtered[CH_MODE];
    if (rc_data.mode <= MODE_PWM_MANUAL_MAX)
    {
        car_output.mode = CAR_MODE_MANUAL; // 0: RC manual mode
    }
    else if (rc_data.mode >= MODE_PWM_FULL_AUTO_MIN)
    {
        car_output.mode = CAR_MODE_FULL_AUTO; // 2: autonomous driving mode
    }
    else
    {
        car_output.mode = CAR_MODE_SEMI_AUTO; // 1: Pilot steering with manual throttle
    }

    if (car_output.mode != lastCarMode)
    {
        buzzer.playModeSound(car_output.mode);
        lastCarMode = car_output.mode;
    }
}

void setup()
{
    pinMode(UART_SEL, OUTPUT);
    // digitalWrite(UART_SEL, HIGH);
    digitalWrite(UART_SEL, LOW);

    Serial.begin(BAUD_RATE_0);                                  // TypeC
    Serial1.begin(BAUD_RATE_1, SERIAL_8N1, RX_1_PIN, TX_1_PIN); // RS232: rx = 16, tx = 17
    mus4Logf("boot", "firmware=%s version=%s build=\"%s %s\"",
        MUS4_FIRMWARE_NAME,
        MUS4_FIRMWARE_VERSION,
        MUS4_BUILD_DATE,
        MUS4_BUILD_TIME);
    mus4LogLine("boot", "ESP32 Receiver Serial Ready!");
    Serial1.println("ESP32 Receiver Serial1 Ready!");

#ifdef ENABLE_BOOT_STEERING_SELF_TEST
    run_steering_tests(); // Run unit tests for steering signal processing
#endif

    #ifdef ENABLE_GAMEPAD_MODE
      bleGamepad.begin();
    #endif
    #ifdef ENABLE_WIFI_CONSOLE
      loadDevModePreference();
      loadWifiApPreference();
      loadWifiStaPreference();
      loadSteeringCalibration();
      setupWifiConsole();
      keepDevModeOtaWindowActive();
    #endif

    g_i2cWorkingSpeed = I2C_SPEED;
    Wire.begin(SDA_PIN, SCL_PIN, g_i2cWorkingSpeed); // SDA = 21, SCL = 22
    delay(100);
    scanI2CBus();
    setup_ina219();
    setup_mpu6050();
    delay(100);

#if ENABLE_RC_MCPWM_CAPTURE
    rcMcpwmCaptureActive = setupRcMcpwmCapture();
#endif
    // Set the RC receiver pins as inputs and attach the interrupts
    for (int i = 0; i < RC_CHANNEL_COUNT; i++)
    {
#if ENABLE_RC_MCPWM_CAPTURE
        if (i == CH_MODE && rcMcpwmCaptureActive) continue;
#endif
        if (Channels[i] == 26) {
            // GPIO 26 supports the internal pull-down resistor
            pinMode(Channels[i], INPUT_PULLDOWN);
        } else {
            // Keep GPIO27 as a normal input; GPIO34/35/36/39 are input-only and have no internal pull-up/down
            pinMode(Channels[i], INPUT);
        }
        attachInterrupt(digitalPinToInterrupt(Channels[i]), isr_functions[i], CHANGE);
    }

    ledcAttachChannel(STEERING_PIN, 300, 14, CH_STEERING);
    ledcAttachChannel(THROTTLE_PIN, 300, 14, CH_THROTTLE);

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);

    // Replace the previous direct color-setting method
    setLEDColor(CRGB::Blue); // Set the initial color with the new function

    // Initialize Park State (Default Locked)
    rc_data.park = PARK_LOCKED; 
    car_output.park = PARK_LOCKED;
    emergencyStopState = EST_IDLE;
    mus4LogLine("tui", "System Locked: Park Mode Active");

    delay(1000);
    uiInitialized = false;
}

void loop()
{
    unsigned long now = millis();
    if (millis() - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL)
    {
        read_ina219();
        read_mpu6050();
        lastSensorUpdate = millis();
    }

    updateSteerCalibration();

    readSerialBuf(Serial, serial0Buf);
    readSerialBuf(Serial1, serial1Buf);
    #ifdef ENABLE_WIFI_CONSOLE
      updateWifiConsole();
      updateWifiWebConsole();
      updateWifiSta();
      updateWifiOta();
    #endif

    // RC signal readout: check timeout and validity, then apply moving-average filtering (with update interval control)
    unsigned long nowUs = micros();
    uint16_t pwmSnapshot[RC_CHANNEL_COUNT];
    unsigned long lastValidSnapshot[RC_CHANNEL_COUNT];
    noInterrupts();
    for (int i = 0; i < RC_CHANNEL_COUNT; i++) {
        pwmSnapshot[i] = pwm_value[i];
        lastValidSnapshot[i] = last_valid_time[i];
    }
    interrupts();
    bool steeringValid = (nowUs - lastValidSnapshot[CH_STEERING]) < RC_SIGNAL_TIMEOUT;
    bool throttleValid = (nowUs - lastValidSnapshot[CH_THROTTLE]) < RC_SIGNAL_TIMEOUT;
    bool parkValid = (nowUs - lastValidSnapshot[CH_PARK]) < RC_SIGNAL_TIMEOUT;
    bool modeValid = (nowUs - lastValidSnapshot[CH_MODE]) < RC_SIGNAL_TIMEOUT;
    bool driftValid = (nowUs - lastValidSnapshot[CH_DRIFT]) < RC_SIGNAL_TIMEOUT;
    bool driftScaleValid = (nowUs - lastValidSnapshot[CH_DRIFT_SCALE]) < RC_SIGNAL_TIMEOUT;

    if (millis() - lastRCFilterUpdate >= RC_FILTER_UPDATE_INTERVAL) {
        // Improved filtering: sliding-window median filter (Size=5)
        auto filterPWM = [&](int ch, uint16_t raw, bool valid) -> uint16_t {
            if (!valid) {
                if (isAuxiliaryRcChannel(ch)) return stabilizeAuxiliaryPWM(ch, pwm_filtered[ch], false);
                if (isPrimaryRcChannel(ch)) return smoothPrimaryPWM(ch, pwm_filtered[ch], false);
                return 1500;
            }

            // Boundary protection: check whether the PWM is within a reasonable range (800-2200us)
            // If out of range, treat it as noise and discard it (do not update the buffer; return the previous filtered value)
            if (raw < RC_PWM_MIN || raw > RC_PWM_MAX) {
                if (isAuxiliaryRcChannel(ch)) return stabilizeAuxiliaryPWM(ch, pwm_filtered[ch], false);
                if (isPrimaryRcChannel(ch)) return smoothPrimaryPWM(ch, pwm_filtered[ch], false);
                return pwm_filtered[ch];
            }

            if (!pwm_filter_initialized[ch]) {
                for (int i = 0; i < PWM_FILTER_SIZE; i++) pwm_filter_buf[ch][i] = raw;
                pwm_filter_idx[ch] = 0;
                pwm_filter_initialized[ch] = true;
            }

            uint8_t idx = pwm_filter_idx[ch];
            pwm_filter_buf[ch][idx] = raw;
            pwm_filter_idx[ch] = (idx + 1) % PWM_FILTER_SIZE;

            // Pure median filtering: ensure the output is the middle value after sorting the window
            uint16_t median = medianFilter(pwm_filter_buf[ch], PWM_FILTER_SIZE);
            uint16_t filtered = isAuxiliaryRcChannel(ch) ? stabilizeAuxiliaryPWM(ch, median, true) : smoothPrimaryPWM(ch, median, true);

            // Debug output
            if (filterDebugEnabled && ch == CH_THROTTLE) {
                 mus4Logf("filter", "F_DBG: ch=%d, raw=%d, med=%d, out=%d", ch, raw, median, filtered);
            }

            return filtered;
        };

        // Apply filtering to all channels
        for (int i = 0; i < RC_CHANNEL_COUNT; i++) {
            bool valid = (nowUs - lastValidSnapshot[i]) < RC_SIGNAL_TIMEOUT;
            pwm_filtered[i] = filterPWM(i, pwmSnapshot[i], valid);
        }
        lastRCFilterUpdate = millis();
    }

    // Update rc_data when the signal is valid; otherwise keep defaults (neutral position)
    if (steeringValid) {
        rc_data.steering = pwm_filtered[CH_STEERING];
    } else {
        rc_data.steering = RC_STEERING_MID; // Use the midpoint after timeout
    }
    if (throttleValid) {
        rc_data.throttle = pwm_filtered[CH_THROTTLE];
    } else {
        rc_data.throttle = RC_THROTTLE_MID; // Use the midpoint after timeout
    }

    // Apply similar handling to the Park, Mode, and Drift channels
    if (!parkValid && !aux_stable_initialized[CH_PARK]) pwm_filtered[CH_PARK] = 1500;
    if (!driftValid && !aux_stable_initialized[CH_DRIFT]) pwm_filtered[CH_DRIFT] = 1000;
    if (!driftScaleValid && !aux_stable_initialized[CH_DRIFT_SCALE]) pwm_filtered[CH_DRIFT_SCALE] = 1500;

    park_change();
    #ifdef ENABLE_WIFI_CONSOLE
    if (wifiOtaParkGuardActive || wifiOtaInProgress) forceWifiOtaParkLocked();
    #endif
    mode_change(modeValid);
    update_drift_assist_control(driftValid, driftScaleValid);

    if (car_output.mode == CAR_MODE_FULL_AUTO)
    {
        // Controlled by Pilot
        if (car_output.park == 1)
        {
            // car_output.throttle = 0;
            // emergencyStop();
            if (carOutputModeLast != CAR_MODE_FULL_AUTO || toggleActive == false)
            {
                setLEDToggle(CRGB::Blue, CRGB::Red);
                carOutputModeLast = CAR_MODE_FULL_AUTO;
            }
            if (!toggleActive)
            {
                setLEDToggle(CRGB::Blue, CRGB::Red);
            }
        }
        else
        {
            setLEDColor(CRGB::Blue); // set LED to Red
            car_output.throttle = pilot_data.throttle;
        }
        car_output.steering = pilot_data.steering;

        #ifdef ENABLE_GAMEPAD_MODE
            sendGamepadPacket();
        #endif
    }
    else if (car_output.mode == CAR_MODE_SEMI_AUTO)
    {
        // Controlled by both RC and Pilot
        if (car_output.park == 1)
        {
            // car_output.throttle = 0;
            // emergencyStop();
            if (carOutputModeLast != CAR_MODE_SEMI_AUTO || toggleActive == false)
            {
                setLEDToggle(CRGB::Yellow, CRGB::Red);
                carOutputModeLast = CAR_MODE_SEMI_AUTO;
            }
        }
        else
        {
            setLEDColor(CRGB::Yellow); // set LED to blue
            car_output.throttle = map(rc_data.throttle, RC_THROTTLE_MIN, RC_THROTTLE_MAX, -100, 100);
        }
        car_output.steering = pilot_data.steering;
    }
    else
    {
        // Controlled by RC Controller (car_output.mode = CAR_MODE_MANUAL)
        if (car_output.park == 1)
        {
            // car_output.throttle = 0;
            // emergencyStop();
            if (carOutputModeLast != CAR_MODE_MANUAL || toggleActive == false)
            {
                setLEDToggle(CRGB::Green, CRGB::Red);
                carOutputModeLast = CAR_MODE_MANUAL;
            }
        }
        else
        {
            setLEDColor(CRGB::Green); // set LED to blue

            // RC => CAR
            car_output.throttle = map(rc_data.throttle, RC_THROTTLE_MIN, RC_THROTTLE_MAX, -100, 100);
        }
        if (steer_cal_enabled) {
            car_output.steering = mapSteeringCalibrated(rc_data.steering);
        } else {
            car_output.steering = map(rc_data.steering, RC_STEERING_MIN, RC_STEERING_MAX, -100, 100);
        }
        // Drift Assist: add counter-steer compensation only in manual mode
        car_output.steering = apply_drift_assist(car_output.steering);
    }

    if (mus4LogTarget == MUS4_LOG_TARGET_SERIAL) {
        tui.setRC(pwm_filtered[CH_STEERING], pwm_filtered[CH_THROTTLE], pwm_filtered[CH_PARK], pwm_filtered[CH_MODE], pwm_filtered[CH_DRIFT], pwm_filtered[CH_DRIFT_SCALE]);
        tui.setOutput(car_output.throttle, car_output.steering, car_output.mode, car_output.park);

        SensorData combined = ina219Data;
        if (mpu6050Data.valid) {
            combined.accelX = mpu6050Data.accelX;
            combined.accelY = mpu6050Data.accelY;
            combined.accelZ = mpu6050Data.accelZ;
            combined.gyroX = mpu6050Data.gyroX;
            combined.gyroY = mpu6050Data.gyroY;
            combined.gyroZ = mpu6050Data.gyroZ;
            combined.temperature = mpu6050Data.temperature;
        }
        tui.setSensors(combined);

        tui.setRefreshRate(uiIntervalCurrent);
        tui.setAnsiEnabled(ansiEnabled);
        tui.setWaveformEnabled(false);
        tui.update(millis());
        lastUICycleDuration = tui.getLastRenderDuration();
    } else {
        lastUICycleDuration = 0;
    }

    if (millis() - lastRCDataUpdate >= RC_DATA_UPDATE_INTERVAL)
    {
        if (shouldEmitSerial1Telemetry()) {
            Serial1.printf("T%d:S%d\n", car_output.throttle, car_output.steering); // RC => Type-C
        }
        lastRCDataUpdate = millis();
    }

#ifdef DEBUG // Print the values for debugging
    // Read the RC receiver values
    for (int i = 0; i < RC_CHANNEL_COUNT; i++)
    {
        Serial.print(" CH");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(pwm_value[i]);
        if (i == 3)
            Serial.println(" ");
    }

#endif

    int pwm_steering = map(car_output.steering, -100, 100, SERVO_MID_V - SERVO_RANGE_V, SERVO_MID_V + SERVO_RANGE_V);
    int pwm_throttle = map(car_output.throttle, -100, 100, MOTOR_MID_V - MOTOR_RANGE_V, MOTOR_MID_V + MOTOR_RANGE_V);

    pwm_steering = min(max(pwm_steering, PWM_MIN_V), PWM_MAX_V);
    pwm_throttle = min(max(pwm_throttle, PWM_MIN_V), PWM_MAX_V);

    ledcWriteChannel(CH_STEERING, pwm_steering);
    ledcWriteChannel(CH_THROTTLE, pwm_throttle);

    counter += 1;

    scanLEDToggle();
    if (now - lastPerfEval >= 1000)
    {
        evalDegrade();
        if (lastUICycleDuration > 150) uiIntervalCurrent = min(uiIntervalCurrent + 50, uiIntervalMax);
        else uiIntervalCurrent = (uiIntervalCurrent > uiIntervalMin ? uiIntervalCurrent - 20 : uiIntervalMin);
        lastPerfEval = now;
    }
    delay(4);
}
