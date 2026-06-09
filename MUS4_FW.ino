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

#define ENABLE_WIFI_CONSOLE
#ifdef ENABLE_WIFI_CONSOLE
#define ENABLE_WIFI_WEBSOCKET_TELEMETRY
#endif
// #define ENABLE_DIAGNOSTIC_COMMANDS
// #define ENABLE_BOOT_STEERING_SELF_TEST

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

// RC Receiver Calibration Defaults (PWM pulse width in microseconds)
#define RC_THROTTLE_MIN 888
#define RC_THROTTLE_MID 1493
#define RC_THROTTLE_MAX 2149
#define RC_STEERING_MIN 872
#define RC_STEERING_MID 1488
#define RC_STEERING_MAX 2113

#define BUZZER_PIN 2

#include "Buzzer.h"
// #include "test_runner.h"

TUI tui(Serial);
Buzzer buzzer(BUZZER_PIN);

int lastCarMode = -1;
bool lastParkState = false;

#ifndef ENABLE_WIFI_CONSOLE
#define ENABLE_GAMEPAD_MODE
#endif

#define MUS4_LOG_TARGET_SERIAL 0
#define MUS4_LOG_TARGET_WEB 1
#ifndef MUS4_LOG_TARGET
#ifdef ENABLE_WIFI_CONSOLE
#define MUS4_LOG_TARGET MUS4_LOG_TARGET_WEB
#else
#define MUS4_LOG_TARGET MUS4_LOG_TARGET_SERIAL
#endif
#endif
uint8_t mus4LogTarget = MUS4_LOG_TARGET;
#ifdef ENABLE_GAMEPAD_MODE
  #include <BleGamepad.h>
  BleGamepad bleGamepad("Gamepad MU02", "Espressif", 100);
#endif

Adafruit_MPU6050 mpu;
Adafruit_INA219 ina219;

// #define DEBUG // Uncomment to enable debugging output

#define CH1_PIN 36 // Receiver PWM input CH1
#define CH2_PIN 39 // Receiver PWM input CH2
#define CH3_PIN 34 // Receiver PWM input CH3
#define CH4_PIN 26 // Receiver PWM input CH4
#define CH5_PIN 27 // Receiver PWM input CH5
#define CH6_PIN 35 // Receiver PWM input CH6

#define STEERING_PIN 23 // CH1 steering servo
#define THROTTLE_PIN 25 // CH2 throttle ESC

#define PWM_1 32 // PWM output channel 1
#define PWM_2 33 // PWM output channel 2

#define LED_PIN 5
#define NUM_LEDS 1
#define BRIGHTNESS 64
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

#define BAUD_RATE_0 115200
#define RX_1_PIN 16
#define TX_1_PIN 17
// #define RX_1_PIN 19
// #define TX_1_PIN 18      // MU02 cannot connect; use pins 16 and 17 consistently.
#define BAUD_RATE_1 115200
#define UART_SEL 12

#define SDA_PIN 21
#define SCL_PIN 22
#define I2C_SPEED 400000L

#define RC_CHANNEL_COUNT 6
#define CH_STEERING 0    // index of pwm_value[]
#define CH_THROTTLE 1    // index of pwm_value[]
#define CH_PARK 2        // index of pwm_value[]
#define CH_MODE 3        // index of pwm_value[]
#define CH_DRIFT 4       // index of pwm_value[]
#define CH_DRIFT_SCALE 5 // index of pwm_value[]

#define CAR_MODE_MANUAL 0    // 0: RC manual mode
#define CAR_MODE_SEMI_AUTO 1 // 1: Pilot steering with manual throttle
#define CAR_MODE_FULL_AUTO 2 // 2: autonomous driving mode
#define MODE_PWM_MANUAL_MAX 1250
#define MODE_PWM_FULL_AUTO_MIN 1750

#define PARK_LOCKED true     // Locked state
#define PARK_UNLOCKED false  // Unlocked state

volatile uint16_t pwm_value[RC_CHANNEL_COUNT] = {0};
volatile unsigned long rise_time[RC_CHANNEL_COUNT] = {0};
volatile unsigned long last_valid_time[RC_CHANNEL_COUNT] = {0};
#define RC_SIGNAL_TIMEOUT 1000000UL  // RC signal timeout (µs)
#define RC_PWM_MIN 800   // Minimum valid PWM (µs)
#define RC_PWM_MAX 2200  // Maximum valid PWM (µs)
#define ENABLE_RC_MCPWM_CAPTURE 0
#define RC_MCPWM_CAPTURE_RESOLUTION_HZ 1000000
#define RC_MCPWM_CAPTURE_GROUP_ID 0

#define PWM_FILTER_SIZE 5  // Sliding-window median filter size (5-7)
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

bool parseAndValidateCommand(String cmd, int* throttle, int* steering);
static void mus4LogLine(const char* source, const String& line);
static void mus4Logf(const char* source, const char* fmt, ...);
static void setMus4LogTargetWeb();

CRGB leds[NUM_LEDS]; // Define the array of leds

// Terminal control macros
#define CLEAR_SCREEN "\033[2J"         // Clear screen
#define CURSOR_HOME "\033[H"           // Move cursor to home position
#define CURSOR_UP(n) "\033[" #n "A"    // Move cursor up n rows
#define CURSOR_DOWN(n) "\033[" #n "B"  // Move cursor down n rows
#define CURSOR_RIGHT(n) "\033[" #n "C" // Move cursor right n columns
#define CURSOR_LEFT(n) "\033[" #n "D"  // Move cursor left n columns
#define SAVE_CURSOR "\033[s"           // Save cursor position
#define RESTORE_CURSOR "\033[u"        // Restore cursor position
#define HIDE_CURSOR "\033[?25l"        // Hide cursor
#define SHOW_CURSOR "\033[?25h"        // Show cursor

// Color macros
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"

// Output control parameters
#define SENSOR_UPDATE_INTERVAL 2     // Sensor data update interval (ms) - ~60Hz
#define RC_DATA_UPDATE_INTERVAL 2    // RC data update interval (ms) - ~60Hz
#define RC_FILTER_UPDATE_INTERVAL 2   // RC filter update interval (ms) - ~125Hz, balances response and stability
#define UI_UPDATE_INTERVAL 2         // UI update interval (ms) - smooth 60Hz experience

// Waveform parameters
#define WAVE_WIDTH 20                 // Waveform width (reduced for performance)
#define WAVE_HEIGHT 6                 // Waveform height (reduced for performance)

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
struct SerialBuf { char buf[256]; uint16_t len; uint32_t frames; uint32_t errors; bool overflow; };
SerialBuf serial0Buf = {{0},0,0,0,false};
SerialBuf serial1Buf = {{0},0,0,0,false};
static bool processLocalOtaMaintenanceCommand(const String& line, Print& out, SerialBuf& sb);
static bool shouldEmitSerial1Telemetry();
#ifdef ENABLE_WIFI_CONSOLE
enum WirelessCommandOrigin { WIRELESS_ORIGIN_TCP, WIRELESS_ORIGIN_WEB };
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
const char* WIFI_CONSOLE_AP_DEFAULT_SSID = "MUS4-DEBUG";
const char* WIFI_CONSOLE_AP_PASSWORD = "mus4-debug";
const uint16_t WIFI_CONSOLE_PORT = 2323;
const uint16_t WIFI_WEB_CONSOLE_PORT = 80;
const uint32_t WIFI_WEB_TELEMETRY_MIN_FREE_HEAP = 60000;
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
const uint16_t WIFI_WEB_SOCKET_PORT = 81;
const unsigned long WIFI_WEB_SOCKET_PUSH_INTERVAL_MS = 16;
const uint8_t WIFI_WEB_SOCKET_MAX_POINTS_PER_FRAME = 8;
const uint8_t WIFI_WEB_SOCKET_MAX_REPLAY_POINTS = 32;
const uint16_t WIFI_WEB_SOCKET_KEEPALIVE_SECONDS = 60;
#endif
const uint8_t WIFI_CONSOLE_CHANNEL = 6;
const uint8_t WIFI_CONSOLE_MAX_CLIENTS = 1;
const unsigned long WIFI_CONSOLE_RETRY_INTERVAL_MS = 5000;
const unsigned long WIFI_STA_CONNECT_TIMEOUT_MS = 15000;
const unsigned long WIFI_STA_APPLY_DELAY_MS = 800;
const char* WIFI_OTA_HOSTNAME = "mus4-ota";
const char* WIFI_OTA_PASSWORD = "mus4-debug";
const uint16_t WIFI_OTA_PORT = 3232;
const unsigned long WIFI_OTA_WINDOW_MS = 120000UL;
const char* MUS4_PREF_NAMESPACE = "mus4";
const char* MUS4_PREF_DEV_MODE_KEY = "dev_mode";
const char* MUS4_PREF_AP_SSID_KEY = "ap_ssid";
const char* MUS4_PREF_STA_ENABLED_KEY = "sta_en";
const char* MUS4_PREF_STA_SSID_KEY = "sta_ssid";
const char* MUS4_PREF_STA_PASSWORD_KEY = "sta_pass";
const char* MUS4_PREF_STEER_MIN_KEY = "str_min";
const char* MUS4_PREF_STEER_MID_KEY = "str_mid";
const char* MUS4_PREF_STEER_MAX_KEY = "str_max";
const char* MUS4_PREF_STEER_CAL_EN_KEY = "str_cal";
const uint8_t WIFI_AP_SSID_MAX_LEN = 32;
const uint8_t WIFI_STA_SSID_MAX_LEN = 32;
const uint8_t WIFI_STA_PASSWORD_MAX_LEN = 63;
const uint8_t WIFI_STA_PASSWORD_MIN_LEN = 8;
const uint8_t WIFI_WEB_LOG_CAPACITY = 64;
const uint16_t WIFI_WEB_DATA_CAPACITY = 256;
const unsigned long WIFI_WEB_DATA_INTERVAL_MS = 16;
WiFiServer wifiConsoleServer(WIFI_CONSOLE_PORT);
WiFiClient wifiConsoleClient;
WebServer wifiWebServer(WIFI_WEB_CONSOLE_PORT);
DNSServer wifiCaptiveDnsServer;
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
AsyncWebServer wifiWebSocketServer(WIFI_WEB_SOCKET_PORT);
AsyncWebSocket wifiWebSocket("/");
#endif
SerialBuf wifiConsoleBuf = {{0},0,0,0,false};
struct WebLogEntry { uint32_t seq; unsigned long t; char source[8]; char line[160]; };
struct WifiScanEntry { char ssid[WIFI_STA_SSID_MAX_LEN + 1]; int32_t rssi; int32_t channel; bool secure; };
struct WebDataPoint {
    uint32_t seq;
    unsigned long t;
    uint16_t dtMs;
    int throttle;
    int steering;
    int mode;
    bool park;
    int rcThrottle;
    int rcSteering;
    int rcChannels[RC_CHANNEL_COUNT];
    int pilotThrottle;
    int pilotSteering;
    float currentMa;
    float voltage;
    float gyroZ;
    bool driftEnabled;
    bool driftActive;
    float driftCompensation;
    float gyroZFiltered;
};
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

struct SteeringCalibration {
    int16_t min_pwm;
    int16_t mid_pwm;
    int16_t max_pwm;
};

enum SteerCalState {
    STEER_CAL_IDLE,
    STEER_CAL_CENTER,
    STEER_CAL_MINMAX,
    STEER_CAL_DONE
};

static SteeringCalibration steer_cal;
static bool steer_cal_enabled = false;
static SteerCalState steer_cal_state = STEER_CAL_IDLE;
static unsigned long steer_cal_stage_start_ms = 0;
static int16_t steer_cal_temp_min = 0;
static int16_t steer_cal_temp_max = 0;

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
static void cursorDownN(int n){ if(ansiEnabled) Serial.printf("\033[%dB", n); }
static void cursorUpN(int n){ if(ansiEnabled) Serial.printf("\033[%dA", n); }
static void cursorRightN(int n){ if(ansiEnabled) Serial.printf("\033[%dC", n); }
static void cursorLeftN(int n){ if(ansiEnabled) Serial.printf("\033[%dD", n); }
int lastModePrinted = -1;
bool lastParkPrinted = true;
int lastCh1 = -1, lastCh2 = -1, lastCh3 = -1, lastCh4 = -1;
int lastOutTh = -1000, lastOutSt = -1000;
unsigned long lastSensorsPrint = 0;
String lastINAStr = "";
String lastMPUStr = "";
int lastWaveTh[WAVE_WIDTH] = {0};     // Cache the previous waveform frame for dirty rectangles
int lastWaveSt[WAVE_WIDTH] = {0};     // Cache the previous waveform frame for dirty rectangles
bool forceRedraw = false;             // Force redraw flag
int lastSeq = -1;                     // Last received sequence number

// Optimized insertion-sort median filter (O(n^2), but very fast and stable for n=5)
static uint16_t medianFilter(uint16_t* buf, int size) {
    uint16_t temp[8]; // Supports up to 8 elements
    // Copy data
    for (int i = 0; i < size; i++) temp[i] = buf[i];
    
    // Insertion sort
    for (int i = 1; i < size; i++) {
        uint16_t key = temp[i];
        int j = i - 1;
        while (j >= 0 && temp[j] > key) {
            temp[j + 1] = temp[j];
            j = j - 1;
        }
        temp[j + 1] = key;
    }
    
    // Return the median value
    return temp[size / 2];
}

static bool isAuxiliaryRcChannel(int ch)
{
    return ch == CH_PARK || ch == CH_MODE || ch == CH_DRIFT || ch == CH_DRIFT_SCALE;
}

static bool isPrimaryRcChannel(int ch)
{
    return ch == CH_STEERING || ch == CH_THROTTLE;
}

static uint16_t smoothPrimaryPWM(int ch, uint16_t value, bool valid)
{
    if (!isPrimaryRcChannel(ch)) return value;
    if (!valid) return primary_smooth_initialized[ch] ? primary_smooth_pwm[ch] : value;
    if (!primary_smooth_initialized[ch]) {
        primary_smooth_pwm[ch] = value;
        primary_smooth_initialized[ch] = true;
        return value;
    }

    int diff = (int)value - (int)primary_smooth_pwm[ch];
    int absDiff = abs(diff);
    if (absDiff <= 6) return primary_smooth_pwm[ch];
    if (absDiff >= 80) {
        primary_smooth_pwm[ch] = value;
        return value;
    }

    primary_smooth_pwm[ch] = primary_smooth_pwm[ch] + (diff * 35) / 100;
    return primary_smooth_pwm[ch];
}

static uint16_t stabilizeAuxiliaryPWM(int ch, uint16_t value, bool valid)
{
    if (!isAuxiliaryRcChannel(ch)) return value;
    if (!valid) return aux_stable_initialized[ch] ? aux_stable_pwm[ch] : value;
    if (!aux_stable_initialized[ch]) {
        aux_stable_pwm[ch] = value;
        aux_candidate_pwm[ch] = value;
        aux_candidate_count[ch] = 0;
        aux_stable_initialized[ch] = true;
        return value;
    }

    int diff = abs((int)value - (int)aux_stable_pwm[ch]);
    if (diff <= 80) {
        aux_stable_pwm[ch] = value;
        aux_candidate_pwm[ch] = value;
        aux_candidate_count[ch] = 0;
        return value;
    }

    if (abs((int)value - (int)aux_candidate_pwm[ch]) <= 80) {
        if (aux_candidate_count[ch] < 255) aux_candidate_count[ch]++;
    } else {
        aux_candidate_pwm[ch] = value;
        aux_candidate_count[ch] = 1;
    }

    if (aux_candidate_count[ch] >= 3) {
        aux_stable_pwm[ch] = value;
        aux_candidate_count[ch] = 0;
        return value;
    }

    return aux_stable_pwm[ch];
}

#ifdef ENABLE_DIAGNOSTIC_COMMANDS
static bool runFilterTests()
{
    mus4LogLine("test", "Running Filter Tests...");
    bool passed = true;
    
    // Simulated buffer
    uint16_t testBuf[PWM_FILTER_SIZE];
    for(int i=0; i<PWM_FILTER_SIZE; i++) testBuf[i] = 1500;
    
    // Test 1: steady-state test
    uint16_t out = medianFilter(testBuf, PWM_FILTER_SIZE);
    if (out != 1500) { mus4Logf("test", "Filter Test 1 Failed: Expected 1500, got %d", out); passed = false; }
    
    // Test 2: single-sample spike suppression (2000us jump)
    testBuf[2] = 2000; // Middle sample jumps
    out = medianFilter(testBuf, PWM_FILTER_SIZE);
    if (out != 1500) { mus4Logf("test", "Filter Test 2 Failed: Spike not suppressed, got %d", out); passed = false; }
    testBuf[2] = 1500; // Restore
    
    // Test 3: double spike (two consecutive outliers should still be suppressed by a 5-sample window)
    testBuf[1] = 2000;
    testBuf[2] = 2000;
    out = medianFilter(testBuf, PWM_FILTER_SIZE);
    if (out != 1500) { mus4Logf("test", "Filter Test 3 Failed: Double spike not suppressed, got %d", out); passed = false; }
    
    // Test 4: step response (majority changes to the new value)
    testBuf[0] = 1600;
    testBuf[1] = 1600;
    testBuf[2] = 1600; // 3/5 changed to 1600
    out = medianFilter(testBuf, PWM_FILTER_SIZE);
    if (out != 1600) { mus4Logf("test", "Filter Test 4 Failed: Step response failed, got %d", out); passed = false; }

    primary_smooth_initialized[CH_STEERING] = false;
    uint16_t smooth = smoothPrimaryPWM(CH_STEERING, 1500, true);
    smooth = smoothPrimaryPWM(CH_STEERING, 1504, true);
    if (smooth != 1500) { mus4Logf("test", "Filter Test 5 Failed: deadband got %d", smooth); passed = false; }

    smooth = smoothPrimaryPWM(CH_STEERING, 1540, true);
    if (smooth <= 1500 || smooth >= 1540) { mus4Logf("test", "Filter Test 6 Failed: smoothing got %d", smooth); passed = false; }

    smooth = smoothPrimaryPWM(CH_STEERING, 1650, true);
    if (smooth != 1650) { mus4Logf("test", "Filter Test 7 Failed: passthrough got %d", smooth); passed = false; }
    primary_smooth_initialized[CH_STEERING] = false;
    primary_smooth_pwm[CH_STEERING] = 0;

    if (passed) mus4LogLine("test", "Filter Tests Passed!");
    return passed;
}

static int testsTotal = 0;
static int testsPassed = 0;
static bool runUnitTests()
{
    testsTotal = 0; testsPassed = 0;
    int t,s,seq;
    // Basic format
    testsTotal++; if (processLine(String("0:0"), &t,&s,&seq) && t==0 && s==0 && seq==-1) testsPassed++;
    testsTotal++; if (!processLine(String("200:0"), &t,&s,&seq)) testsPassed++;
    // Checksum format
    char payload1[] = "10:-10";
    uint8_t cs1 = calcChecksum(payload1, sizeof(payload1)-1);
    char line1[32]; snprintf(line1, sizeof(line1), "%s*%02X", payload1, cs1);
    testsTotal++; if (processLine(String(line1), &t,&s,&seq) && t==10 && s==-10 && seq==-1) testsPassed++;
    // Seq format
    testsTotal++; if (processLine(String("50:50:100"), &t,&s,&seq) && t==50 && s==50 && seq==100) testsPassed++;
    // Seq + Checksum
    char payload2[] = "20:-20:255";
    uint8_t cs2 = calcChecksum(payload2, sizeof(payload2)-1);
    char line2[32]; snprintf(line2, sizeof(line2), "%s*%02X", payload2, cs2);
    testsTotal++; if (processLine(String(line2), &t,&s,&seq) && t==20 && s==-20 && seq==255) testsPassed++;
    
    return testsPassed*100/testsTotal >= 85;
}
static bool runBenchmarks()
{
    unsigned long ts = millis();
    unsigned long loops = 0;
    unsigned long durStart = millis();
    while (millis() - durStart < 200)
    {
        tui.forceRedraw();
        tui.render();
        loops++;
    }
    unsigned long t1 = millis() - ts;
    unsigned long score = loops;
    mus4Logf("bench", "BENCH: loops=%lu duration=%lums", score, t1);
    return score > 1;
}
#endif
struct struct_message
{
    int throttle; // Throttle value
    int steering; // Steering value
    int mode;     // Driving mode: 0 RC manual, 1 Pilot steering with manual throttle, 2 autonomous driving
    bool park;    // Park state: 0 parked, 1 started
};

struct struct_message esp_now_data = {0, 0, 0, PARK_LOCKED}; // Initialize the structure at declaration
struct struct_message rc_data = {0, 0, 0, PARK_LOCKED};      // Initialize the structure at declaration
struct struct_message pilot_data = {0, 0, 0, PARK_LOCKED};   // Initialize the structure at declaration
struct struct_message car_output = {0, 0, 0, PARK_LOCKED};   // Initialize the structure at declaration

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
const int SERVO_MID_V = 7372;    // 1500µs @ 300Hz
const int SERVO_RANGE_V = 2458; // ±500µs range
const int MOTOR_OFFSET_V = 1;
const int SERVO_OFFSET_V = -1;

#ifdef ENABLE_DIAGNOSTIC_COMMANDS
static bool runStress()
{
    uint32_t errs0 = serial0Buf.errors;
    for (int i=0;i<50;i++)
    {
        int tt,ss,seq;
        processLine(String("999:999"), &tt,&ss,&seq);
    }
    mus4Logf("stress", "STRESS: errors_delta=%lu", serial0Buf.errors-errs0);
    return true;
}

static bool runRegression()
{
    int v = map(-100, -100, 100, SERVO_MID_V - SERVO_RANGE_V, SERVO_MID_V + SERVO_RANGE_V);
    int v2 = map(100, -100, 100, SERVO_MID_V - SERVO_RANGE_V, SERVO_MID_V + SERVO_RANGE_V);
    bool ok = (v <= v2);
    mus4Logf("regress", "REGRESS: ok=%d", ok?1:0);
    return ok;
}
#endif

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

// --- Steering Signal Processing Constants & Globals ---
const int PWM_VALID_MIN = 800; // Increased from 500 to reject noise
const int PWM_VALID_MAX = 2200; // Decreased from 2500 to reject noise
const int MA_WINDOW_SIZE = 10;
const int MAX_ERROR_COUNT = 3;

// PID Parameters
struct PIDConfig {
    float Kp = 0.8; // 0.6  
    float Ki = 0.05;
    float Kd = 0.2;
    float integral_limit = 50.0;
    float deadband = 2.0;
};

struct PIDState {
    float integral = 0;
    float prev_error = 0;
    float current_smooth_output = 0;
};

PIDConfig pid_config;
PIDState pid_state;

int steering_history[MA_WINDOW_SIZE] = {0};
int steering_index = 0;
int last_valid_steering_pwm = 1488; // Default to center
int steering_error_count = 0;
int valid_signal_count = 0; // New: Counter for valid signals to exit safe mode
bool safe_mode_active = false;
bool is_history_initialized = false;

// --- Drift Assist Constants & Globals ---
#define DRIFT_ASSIST_ENABLED     1        // Globally enable Drift Assist at compile time
#define DRIFT_ASSIST_GAIN        25.0f    // Counter-steer gain (gyroZ rad/s -> compensation ±100)
#define DRIFT_ASSIST_THRESHOLD   1.2f     // Drift trigger threshold in rad/s; no intervention below this value
#define DRIFT_ASSIST_MAX_COMP    70       // Maximum compensation angle (±70), prevents excessive counter-steer
#define DRIFT_ASSIST_SMOOTH      0.25f    // First-order smoothing factor for compensation, avoids output jitter
#define DRIFT_ASSIST_DECAY       0.85f    // Compensation decay factor when not triggered

bool drift_assist_enabled = false;   // Whether the user has enabled assist
bool drift_assist_active = false;    // Whether assist is currently intervening
float drift_compensation = 0.0f;     // Current compensation value (final smoothed value)
float gyro_z_filtered = 0.0f;        // Filtered gyroZ value
float drift_assist_scale = 1.0f;     // CH6 Drift Assist strength ratio
// ------------------------------------------------------

// Modified setLEDColor function
void setLEDColor(CRGB targetColor)
{
    if (toggleActive)
    {
        toggleActive = false; // Disable toggle mode
        toggleTime = 0;       // Reset toggle time
    }
    if (leds[0] != targetColor)
    {
        leds[0] = targetColor;
        FastLED.show(); // Update display only when the color differs
    }
}

// Added setLEDToggle function
void setLEDToggle(CRGB color1, CRGB color2)
{
    toggleColor1 = color1;
    toggleColor2 = color2;
    toggleActive = true;
    toggleTime = 0; // Ensure the first toggle runs immediately
}

void scanLEDToggle()
{
    if (toggleActive && (millis() >= toggleTime))
    {
        CRGB currentColor = leds[0];
        CRGB nextColor = (currentColor == toggleColor1) ? toggleColor2 : toggleColor1;
        leds[0] = nextColor;
        FastLED.show();
        toggleTime = millis() + toggleInterval;
    }
}

static void notifyDegrade()
{
    mus4LogLine("system", "DEGRADED MODE ACTIVE");
}

static void evalDegrade()
{
    degradeReason = 0;
    if (!ina219Data.valid) degradeReason |= 0x01;
    if (!mpu6050Data.valid) degradeReason |= 0x02;
    if (lastUICycleDuration > 150) degradeReason |= 0x04;
    if (degradeReason != 0 && !degradeMode)
    {
        degradeMode = true;
        notifyDegrade();
    }
    if (degradeReason == 0 && degradeMode)
    {
        degradeMode = false;
    }
}

static uint8_t parseHex2(const char* s)
{
    auto hv = [](char c)->uint8_t{ if(c>='0'&&c<='9')return c-'0'; if(c>='a'&&c<='f')return 10+(c-'a'); if(c>='A'&&c<='F')return 10+(c-'A'); return 0; };
    return (hv(s[0])<<4)|hv(s[1]);
}

static uint8_t calcChecksum(const char* s, int n)
{
    uint32_t sum = 0;
    for (int i=0;i<n;i++) sum += (uint8_t)s[i];
    return (uint8_t)(sum & 0xFF);
}

static bool processLine(const String& line, int* throttle, int* steering, int* seq)
{
    // Handle local commands
    if (line.equalsIgnoreCase("NOANSI")) { ansiEnabled = false; tui.setAnsiEnabled(false); tui.forceRedraw(); return false; }
    if (line.equalsIgnoreCase("ANSI")) { ansiEnabled = true; tui.setAnsiEnabled(true); tui.forceRedraw(); return false; }
    if (line.equalsIgnoreCase("FILTER_DEBUG")) {
        filterDebugEnabled = !filterDebugEnabled;
        mus4Logf("filter", "Filter Debug: %s", filterDebugEnabled ? "ON" : "OFF");
        return false;
    }
#ifdef ENABLE_DIAGNOSTIC_COMMANDS
    if (line.equalsIgnoreCase("FILTER_TEST")) {
        runFilterTests();
        return false;
    }
#endif

    *seq = -1;
    int star = line.lastIndexOf('*');
    if (star > 0)
    {
        String payload = line.substring(0, star);
        String cs = line.substring(star+1);
        if (cs.length()>=2)
        {
            char cs0 = cs.charAt(0);
            char cs1 = cs.charAt(1);
            char tmp[3]; tmp[0]=cs0; tmp[1]=cs1; tmp[2]=0;
            uint8_t want = parseHex2(tmp);
            int plen = payload.length();
            char buf[260]; int blen = plen; if (blen>259) blen=259;
            payload.toCharArray(buf, blen+1);
            uint8_t got = calcChecksum(buf, blen);
            if (want != got) return false;
            
            // Try to parse SEQ: T:S:SEQ
            int col2 = payload.lastIndexOf(':');
            int col1 = payload.indexOf(':');
            if (col2 > col1 && col1 > 0) {
                 String seqStr = payload.substring(col2+1);
                 *seq = seqStr.toInt();
                 return parseAndValidateCommand(payload.substring(0, col2), throttle, steering);
            }
            return parseAndValidateCommand(payload, throttle, steering);
        }
    }
    
    // No checksum; try to parse T:S:SEQ
    int col2 = line.lastIndexOf(':');
    int col1 = line.indexOf(':');
    if (col2 > col1 && col1 > 0) {
            String seqStr = line.substring(col2+1);
            *seq = seqStr.toInt();
            return parseAndValidateCommand(line.substring(0, col2), throttle, steering);
    }
    
    return parseAndValidateCommand(line, throttle, steering);
}

#ifdef ENABLE_WIFI_CONSOLE
static bool processWifiStaConfigCommand(const String& line, Print& out);
static String wifiStaIpText();
#else
static bool processWifiStaConfigCommand(const String& line, Print& out) { return false; }
#endif

#define PROCESS_COMMAND_LINE(line, out, sb) do { \
    if (processLocalOtaMaintenanceCommand((line), (out), (sb))) { \
    } else if (processWifiStaConfigCommand((line), (out))) { \
    } else if ((line).equalsIgnoreCase("LOG_WEB")) { \
        setMus4LogTargetWeb(); \
        mus4LogLine("log", mus4LogTarget == MUS4_LOG_TARGET_WEB ? "target=web" : "target=serial wifi_disabled"); \
        (out).println("ACK:LOG_WEB"); \
    } else if ((line).equalsIgnoreCase("LOG_SERIAL")) { \
        mus4LogTarget = MUS4_LOG_TARGET_SERIAL; \
        mus4LogLine("log", "target=serial"); \
        (out).println("ACK:LOG_SERIAL"); \
    } else if ((line).equalsIgnoreCase("STEER_CAL")) { \
        startSteerCalibration(out); \
    } else if ((line).equalsIgnoreCase("CAL_SAVE")) { \
        if (steer_cal_state == STEER_CAL_DONE) { \
            if (steer_cal.min_pwm < steer_cal.mid_pwm && steer_cal.mid_pwm < steer_cal.max_pwm \
                && (steer_cal.mid_pwm - steer_cal.min_pwm) > 100 && (steer_cal.max_pwm - steer_cal.mid_pwm) > 100) { \
                if (saveSteeringCalibration()) { \
                    steer_cal_state = STEER_CAL_IDLE; \
                    (out).println("ACK:CAL_SAVED"); \
                } else { \
                    (out).println("NACK:CAL_SAVE_FAILED"); \
                } \
            } else { \
                (out).println("NACK:CAL_INVALID_RANGE"); \
            } \
        } else { \
            (out).println("NACK:CAL_NOT_DONE"); \
        } \
    } else if ((line).equalsIgnoreCase("CAL_RETRY")) { \
        if (steer_cal_state == STEER_CAL_DONE) { \
            steer_cal_state = STEER_CAL_CENTER; \
            steer_cal_stage_start_ms = millis(); \
            steer_cal_temp_min = 32767; \
            steer_cal_temp_max = -32768; \
            tui.log("[CAL] Retrying center capture..."); \
            (out).println("ACK:CAL_RETRY"); \
        } else { \
            (out).println("NACK:CAL_NOT_DONE"); \
        } \
    } else if ((line).equalsIgnoreCase("CAL_ABORT")) { \
        steer_cal_state = STEER_CAL_IDLE; \
        loadSteeringCalibration(); \
        (out).println("ACK:CAL_ABORTED"); \
    } else if ((line).equalsIgnoreCase("CAL_RESET")) { \
        resetSteeringCalibration(); \
        steer_cal_state = STEER_CAL_IDLE; \
        (out).println("ACK:CAL_RESET"); \
    } else if ((line).equalsIgnoreCase("CAL_STATUS")) { \
        printCalStatus(out); \
    } else { \
        int t, s, seq; \
        bool ok = processLine((line), &t, &s, &seq); \
        if (ok) { \
            pilot_data.throttle = t; \
            pilot_data.steering = s; \
            lastSeq = seq; \
            if (seq >= 0) (out).printf("ACK:%d\n", seq); \
            else (out).println("ACK"); \
            (sb).frames++; \
        } else { \
            if (seq >= 0) (out).printf("NACK:%d\n", seq); \
            else (out).println("NACK"); \
            (sb).errors++; \
        } \
    } \
} while (false)

static void readSerialBuf(HardwareSerial& ser, SerialBuf& sb)
{
    while (ser.available())
    {
        int c = ser.read();
        if (c < 0) break;
        if (c == '\r') continue;
        if (c == '\n')
        {
            sb.buf[sb.len] = 0;
            PROCESS_COMMAND_LINE(String(sb.buf), ser, sb);
            sb.len = 0;
            sb.overflow = false;
        }
        else
        {
            if (sb.len < sizeof(sb.buf)-1)
            {
                sb.buf[sb.len++] = (char)c;
            }
            else
            {
                sb.len = 0;
                sb.overflow = true;
            }
        }
    }
}

#ifdef ENABLE_WIFI_CONSOLE
static bool isWirelessControlCommand(const String& line)
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

static bool isWirelessOtaOpenCommand(const String& line)
{
    return line.equalsIgnoreCase("ENABLE_OTA");
}

static bool isLocalOtaOpenCommand(const String& line)
{
    return line.startsWith("ENABLE_OTA:");
}

static bool isWirelessOtaStatusCommand(const String& line)
{
    return line.equalsIgnoreCase("OTA_STATUS");
}

static bool isWirelessOtaCloseCommand(const String& line)
{
    return line.equalsIgnoreCase("DISABLE_OTA");
}

static bool isWifiStaConfigCommand(const String& line)
{
    return line.startsWith("WIFI_STA_SSID:") ||
        line.startsWith("WIFI_STA_PASSWORD:") ||
        line.equalsIgnoreCase("WIFI_STA_APPLY") ||
        line.equalsIgnoreCase("WIFI_STA_CLEAR");
}

static bool isParkLockedWirelessCommand(const String& line)
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

static bool isWirelessCommandAllowed(const String& line, WirelessCommandOrigin origin)
{
    bool webDevMode = wifiDevModeEnabled && origin == WIRELESS_ORIGIN_WEB;
    if (line.equalsIgnoreCase("PING") || line.equalsIgnoreCase("STATUS") || line.equalsIgnoreCase("WIFI_STA_STATUS")) return true;
    if (line.startsWith("AUTH:")) return true;
    if (isWirelessOtaOpenCommand(line)) return (webDevMode || wifiConsoleAuthenticated) && car_output.park == PARK_LOCKED;
    if (isWirelessOtaStatusCommand(line) || isWirelessOtaCloseCommand(line)) return webDevMode || wifiConsoleAuthenticated;
    if (!wifiConsoleAuthenticated && !webDevMode) return false;
    if (isParkLockedWirelessCommand(line)) return car_output.park == PARK_LOCKED;
    if (line.equalsIgnoreCase("ANSI") || line.equalsIgnoreCase("NOANSI") || line.equalsIgnoreCase("FILTER_DEBUG") || line.equalsIgnoreCase("LOG_WEB") || line.equalsIgnoreCase("LOG_SERIAL") || isWifiStaConfigCommand(line)) return true;
    return isWirelessControlCommand(line);
}

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

static void loadSteeringCalibration()
{
    steer_cal.min_pwm = RC_STEERING_MIN;
    steer_cal.mid_pwm = RC_STEERING_MID;
    steer_cal.max_pwm = RC_STEERING_MAX;
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, true)) {
        steer_cal_enabled = false;
        return;
    }
    steer_cal_enabled = mus4Prefs.getBool(MUS4_PREF_STEER_CAL_EN_KEY, false);
    if (steer_cal_enabled) {
        steer_cal.min_pwm = (int16_t)mus4Prefs.getShort(MUS4_PREF_STEER_MIN_KEY, RC_STEERING_MIN);
        steer_cal.mid_pwm = (int16_t)mus4Prefs.getShort(MUS4_PREF_STEER_MID_KEY, RC_STEERING_MID);
        steer_cal.max_pwm = (int16_t)mus4Prefs.getShort(MUS4_PREF_STEER_MAX_KEY, RC_STEERING_MAX);
    }
    mus4Prefs.end();
    mus4Logf("cal", "steer_cal enabled=%d min=%d mid=%d max=%d",
             steer_cal_enabled ? 1 : 0, steer_cal.min_pwm, steer_cal.mid_pwm, steer_cal.max_pwm);
}

static bool saveSteeringCalibration()
{
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    mus4Prefs.putShort(MUS4_PREF_STEER_MIN_KEY, steer_cal.min_pwm);
    mus4Prefs.putShort(MUS4_PREF_STEER_MID_KEY, steer_cal.mid_pwm);
    mus4Prefs.putShort(MUS4_PREF_STEER_MAX_KEY, steer_cal.max_pwm);
    mus4Prefs.putBool(MUS4_PREF_STEER_CAL_EN_KEY, true);
    mus4Prefs.end();
    steer_cal_enabled = true;
    mus4Logf("cal", "saved min=%d mid=%d max=%d", steer_cal.min_pwm, steer_cal.mid_pwm, steer_cal.max_pwm);
    return true;
}

static void resetSteeringCalibration()
{
    steer_cal.min_pwm = RC_STEERING_MIN;
    steer_cal.mid_pwm = RC_STEERING_MID;
    steer_cal.max_pwm = RC_STEERING_MAX;
    steer_cal_enabled = false;
    if (mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) {
        mus4Prefs.remove(MUS4_PREF_STEER_MIN_KEY);
        mus4Prefs.remove(MUS4_PREF_STEER_MID_KEY);
        mus4Prefs.remove(MUS4_PREF_STEER_MAX_KEY);
        mus4Prefs.remove(MUS4_PREF_STEER_CAL_EN_KEY);
        mus4Prefs.end();
    }
    mus4LogLine("cal", "reset to defaults");
}

static int mapSteeringCalibrated(int16_t pwm)
{
    if (pwm < steer_cal.mid_pwm) {
        return map(pwm, steer_cal.min_pwm, steer_cal.mid_pwm, -100, 0);
    } else {
        return map(pwm, steer_cal.mid_pwm, steer_cal.max_pwm, 0, 100);
    }
}

static void printCalStatus(Print& out)
{
    out.printf("CAL_STATUS enabled=%d min=%d mid=%d max=%d state=%d\n",
               steer_cal_enabled ? 1 : 0,
               steer_cal.min_pwm, steer_cal.mid_pwm, steer_cal.max_pwm,
               (int)steer_cal_state);
}

static bool startSteerCalibration(Print& out)
{
    if (car_output.park != PARK_LOCKED) {
        out.println("NACK:PARK_REQUIRED");
        return false;
    }
    steer_cal_state = STEER_CAL_CENTER;
    steer_cal_stage_start_ms = millis();
    steer_cal_temp_min = 32767;
    steer_cal_temp_max = -32768;
    tui.log("[CAL] Keep steering centered, auto-capture in 3s...");
    mus4LogLine("cal", "center stage started");
    return true;
}

static void updateSteerCalibration()
{
    if (steer_cal_state == STEER_CAL_IDLE) return;

    unsigned long now = millis();
    unsigned long elapsed = now - steer_cal_stage_start_ms;

    if (steer_cal_state == STEER_CAL_CENTER) {
        if (elapsed < 3000) return;
        steer_cal.mid_pwm = (int16_t)pwm_filtered[CH_STEERING];
        char buf[64];
        snprintf(buf, sizeof(buf), "[CAL] Center captured: %d", steer_cal.mid_pwm);
        tui.log(buf);
        mus4Logf("cal", "center=%d", steer_cal.mid_pwm);
        steer_cal_state = STEER_CAL_MINMAX;
        steer_cal_stage_start_ms = now;
        steer_cal_temp_min = 32767;
        steer_cal_temp_max = -32768;
        tui.log("[CAL] Swing stick full left/right within 5s...");
    } else if (steer_cal_state == STEER_CAL_MINMAX) {
        int16_t current = (int16_t)pwm_filtered[CH_STEERING];
        if (current < steer_cal_temp_min) steer_cal_temp_min = current;
        if (current > steer_cal_temp_max) steer_cal_temp_max = current;
        if (elapsed < 5000) return;
        steer_cal.min_pwm = steer_cal_temp_min;
        steer_cal.max_pwm = steer_cal_temp_max;
        char buf[96];
        snprintf(buf, sizeof(buf), "[CAL] Range captured: min=%d max=%d", steer_cal.min_pwm, steer_cal.max_pwm);
        tui.log(buf);
        mus4Logf("cal", "range min=%d max=%d", steer_cal.min_pwm, steer_cal.max_pwm);
        steer_cal_state = STEER_CAL_DONE;
        snprintf(buf, sizeof(buf), "[CAL] Result: mid=%d min=%d max=%d", steer_cal.mid_pwm, steer_cal.min_pwm, steer_cal.max_pwm);
        tui.log(buf);
        tui.log("[CAL] Send CAL_SAVE / CAL_RETRY / CAL_ABORT");
    }
}

static void appendJsonString(String& out, const char* text)
{
    out += '"';
    while (*text) {
        char c = *text++;
        if (c == '\\' || c == '"') {
            out += '\\';
            out += c;
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else if ((uint8_t)c >= 32) {
            out += c;
        }
    }
    out += '"';
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

static String redactWirelessConsoleLine(const String& line)
{
    if (line.startsWith("AUTH:")) return "AUTH:<redacted>";
    if (line.startsWith("WIFI_STA_PASSWORD:")) return "WIFI_STA_PASSWORD:<redacted>";
    return line;
}

static bool isMdnsSafeHostnameChar(char c)
{
    return (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') ||
        c == '-';
}

static bool isMdnsSafeHostname(const String& value)
{
    if (value.length() == 0 || value.length() > WIFI_AP_SSID_MAX_LEN) return false;
    if (value[0] == '-' || value[value.length() - 1] == '-') return false;
    for (uint8_t i = 0; i < value.length(); i++) {
        if (!isMdnsSafeHostnameChar(value[i])) return false;
    }
    return true;
}

static bool copyWifiApSsid(const String& ssid)
{
    if (!isMdnsSafeHostname(ssid)) return false;
    ssid.toCharArray(wifiApSsid, sizeof(wifiApSsid));
    return true;
}

static bool copyWifiStaSsid(const String& ssid)
{
    if (ssid.length() == 0 || ssid.length() > WIFI_STA_SSID_MAX_LEN) return false;
    ssid.toCharArray(wifiStaSsid, sizeof(wifiStaSsid));
    return true;
}

static bool copyWifiStaPassword(const String& password)
{
    if (password.length() > 0 && (password.length() < WIFI_STA_PASSWORD_MIN_LEN || password.length() > WIFI_STA_PASSWORD_MAX_LEN)) return false;
    password.toCharArray(wifiStaPassword, sizeof(wifiStaPassword));
    wifiStaPasswordSet = password.length() > 0;
    return true;
}

static String wifiMdnsHostText()
{
    String host = String(wifiApSsid);
    host.toLowerCase();
    return host;
}

static String wifiMdnsUrlText()
{
    return String("http://") + wifiMdnsHostText() + ".local/";
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

static void clearWifiStaLastError()
{
    wifiStaLastError[0] = 0;
    wifiStaLastErrorMessage[0] = 0;
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

static void setWifiStaLastError(const char* code, const char* message, bool timedOut)
{
    // 保留本轮连接的首个失败原因，避免后续瞬态状态覆盖更有诊断价值的根因。
    if (wifiStaLastError[0] != 0) return;
    snprintf(wifiStaLastError, sizeof(wifiStaLastError), "%s", code);
    snprintf(wifiStaLastErrorMessage, sizeof(wifiStaLastErrorMessage), "%s", message);
    wifiStaTimedOut = timedOut;
    wifiStaConnecting = false;
    wifiStaConnected = false;
    mus4Logf("wifi", "STA failed: %s", code);
}

static void applyWifiStaCredentials()
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

static void printWifiStaStatus(Print& out)
{
    out.printf("WIFI_STA configured=%d connected=%d timed_out=%d connecting=%d ssid=\"%s\" password_set=%d ap_ip=%s sta_ip=%s last_error=\"%s\" last_error_message=\"%s\"\n",
        wifiStaConfigured ? 1 : 0,
        wifiStaConnected ? 1 : 0,
        wifiStaTimedOut ? 1 : 0,
        wifiStaConnecting ? 1 : 0,
        wifiStaSsid,
        wifiStaPasswordSet ? 1 : 0,
        WiFi.softAPIP().toString().c_str(),
        wifiStaIpText().c_str(),
        wifiStaLastError,
        wifiStaLastErrorMessage);
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

static bool saveWifiStaSsidPreference(const String& ssid)
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

static bool saveWifiStaPasswordPreference(const String& password)
{
    if (!copyWifiStaPassword(password)) return false;
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    size_t enabledWritten = mus4Prefs.putBool(MUS4_PREF_STA_ENABLED_KEY, true);
    size_t passwordWritten = mus4Prefs.putString(MUS4_PREF_STA_PASSWORD_KEY, wifiStaPassword);
    mus4Prefs.end();
    if (enabledWritten == 0 || (wifiStaPasswordSet && passwordWritten == 0)) return false;
    return true;
}

static bool clearWifiStaPreference()
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

static bool processWifiStaConfigCommand(const String& line, Print& out)
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
        out.printf("WIFI_STA_SSID_SAVED configured=%d\n", wifiStaConfigured ? 1 : 0);
        return true;
    }
    if (line.startsWith("WIFI_STA_PASSWORD:")) {
        String password = line.substring(18);
        if (!saveWifiStaPasswordPreference(password)) {
            out.println("NACK:WIFI_STA_PASSWORD");
            return true;
        }
        out.printf("WIFI_STA_PASSWORD_SAVED password_set=%d\n", wifiStaPasswordSet ? 1 : 0);
        return true;
    }
    if (line.equalsIgnoreCase("WIFI_STA_APPLY")) {
        if (!wifiStaConfigured) {
            out.println("NACK:WIFI_STA_NOT_CONFIGURED");
            return true;
        }
        applyWifiStaCredentials();
        out.printf("WIFI_STA_APPLY_OK ssid=\"%s\"\n", wifiStaSsid);
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

static void setMus4LogTargetWeb()
{
#if defined(ENABLE_WIFI_CONSOLE)
    mus4LogTarget = MUS4_LOG_TARGET_WEB;
#else
    mus4LogTarget = MUS4_LOG_TARGET_SERIAL;
#endif
}

static void mus4LogLine(const char* source, const String& line)
{
#if defined(ENABLE_WIFI_CONSOLE)
    if (mus4LogTarget == MUS4_LOG_TARGET_WEB) {
        appendWifiWebLog(source, line);
        return;
    }
#endif
    Serial.println("[" + String(source) + "] " + line);
}

static void mus4Logf(const char* source, const char* fmt, ...)
{
    char buf[192];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    mus4LogLine(source, String(buf));
}

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

static String wifiStaIpText()
{
    return wifiStaConnected ? WiFi.localIP().toString() : String("0.0.0.0");
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

static bool processLocalOtaMaintenanceCommand(const String& line, Print& out, SerialBuf& sb)
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
    PROCESS_COMMAND_LINE(line, out, wifiConsoleBuf);
}

class StringPrint : public Print {
public:
    explicit StringPrint(String& target) : _target(target) {}
    size_t write(uint8_t value) override
    {
        _target += (char)value;
        return 1;
    }
    size_t write(const uint8_t* buffer, size_t size) override
    {
        for (size_t i = 0; i < size; i++) _target += (char)buffer[i];
        return size;
    }
private:
    String& _target;
};

static const char WIFI_WEB_CONSOLE_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>DonkeyDrift Console</title>
<style>
body{font-family:system-ui,sans-serif;margin:12px;background:#101318;color:#e8edf2}h1{margin:0;font-size:22px}.headerRow{display:flex;align-items:flex-end;gap:12px;flex-wrap:wrap;margin:0 0 10px}.version{color:#8fa1b5;font-size:12px;text-transform:uppercase;letter-spacing:.08em;display:inline-block;transform:translateY(-1px)}.toggleSwitch{position:relative;display:inline-flex;align-items:center;gap:8px;cursor:pointer}.toggleSwitch input{opacity:0;width:0;height:0;position:absolute}.slider{position:relative;width:44px;height:24px;background:#475569;border-radius:999px;transition:.25s}.slider:before{content:"";position:absolute;height:18px;width:18px;left:3px;bottom:3px;background:#fff;border-radius:50%;transition:.25s}.toggleSwitch input:checked+.slider{background:#5cc8ff}.toggleSwitch input:checked+.slider:before{transform:translateX(20px)}.toggleLabel{color:#8fa1b5;font-size:12px;text-transform:uppercase;letter-spacing:.08em}.devHint{position:relative}.devHint:hover:after{content:'开发模式会持久化；Web Console 免 AUTH，但仍保留 Park Locked 安全限制。OTA 传输期间会默认 Park Locked。';position:absolute;right:0;top:26px;width:280px;background:#111820;border:1px solid #5cc8ff;border-radius:10px;padding:8px 10px;color:#dbeafe;font-size:12px;font-weight:600;line-height:1.45;text-transform:none;letter-spacing:0;white-space:normal;z-index:6}.otaLink{margin-left:auto;text-decoration:none}.otaButton{background:#5cc8ff;color:#061019;border-color:#5cc8ff;font-weight:800;font-size:11px;padding:0 10px;min-width:0;height:24px;border-radius:999px;line-height:1}.otaButton:hover{background:#8bdcff}.grid{display:grid;grid-template-columns:1fr;gap:10px}.panel{background:#171c24;border:1px solid #2b3441;border-radius:8px;padding:10px}#status{white-space:pre-wrap;color:#b7c6d8;font-size:13px;margin-top:10px}.fold{margin-top:10px}.foldHead{width:100%;display:flex;align-items:center;gap:6px;text-align:left;background:#111820;border:1px solid #2b3441;color:#dbeafe}.foldHead:hover{background:#17202b}.foldIcon{display:inline-block;width:16px;color:#93c5fd}.fold:not(.open) .foldBody{display:none}.statusTable{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:4px 12px}.statusRow{display:grid;grid-template-columns:minmax(100px,auto) 1fr;gap:10px;align-items:start}.statusRow b{color:#8fa1b5;font-weight:700}.statusRow span{font:13px Consolas,monospace;color:#e8edf2;overflow-wrap:anywhere}.stateGrid{display:grid;gap:10px;align-items:stretch;grid-template-columns:minmax(96px,.30fr) minmax(160px,.56fr) minmax(260px,1.30fr) minmax(112px,.30fr) minmax(220px,.80fr);grid-template-areas:"mode park drift voltage network"}#modeCard{grid-area:mode}#parkCard{grid-area:park}#driftCard{grid-area:drift}#voltageCard{grid-area:voltage}#networkCard{grid-area:network}.stateCard{position:relative;overflow:hidden;border:1px solid #344154;border-radius:10px;padding:12px;background:linear-gradient(135deg,#1c2430,#121821);box-shadow:0 0 0 rgba(0,0,0,0);transition:.25s}.stateHead{color:#8fa1b5;font-size:12px;text-transform:uppercase;letter-spacing:.08em}.stateValue{font-size:24px;font-weight:800;margin-top:4px;white-space:normal;overflow:visible;text-overflow:clip;word-break:normal;overflow-wrap:normal;line-height:1.08}.copyValue{cursor:pointer;position:relative}.copyValue:hover{color:#5cc8ff}.copyValue:hover:after{content:'点击复制 IP';position:absolute;left:72px;top:-26px;background:#111820;border:1px solid #5cc8ff;border-radius:8px;padding:4px 8px;color:#dbeafe;font-size:12px;font-weight:600;white-space:nowrap;pointer-events:none;z-index:4}.stateSub{color:#b7c6d8;font-size:12px;margin-top:3px;white-space:normal;overflow:visible;text-overflow:clip;word-break:normal;overflow-wrap:normal;line-height:1.25}.stateDot{position:absolute;right:12px;top:12px;width:10px;height:10px;border-radius:50%;background:#667}.gear{position:absolute;right:10px;top:32px;width:30px;height:30px;min-width:0;padding:0;border-radius:50%;font-size:16px;line-height:1;z-index:6}.netTabs{position:absolute;right:28px;top:8px;display:flex;gap:4px}.netTabs button{min-width:0;padding:2px 8px;border-radius:999px;font-size:11px;line-height:1.2}.netTabs button.active{background:#5cc8ff;color:#061019;font-weight:800}.stateMeta{display:grid;grid-template-columns:1fr;gap:3px;margin-top:8px;font-size:12px}.stateMeta b{color:#8fa1b5;font-size:11px;letter-spacing:.08em;text-transform:uppercase}.stateMeta span{font-size:15px;font-weight:700;white-space:normal;overflow:visible;text-overflow:clip;word-break:normal;overflow-wrap:normal;line-height:1.2}#networkCard .stateValue,#networkCard .stateMeta span,#driftCard .stateSub{overflow-wrap:anywhere}.mode0{border-color:#39d98a}.mode1{border-color:#ffcc66}.mode2{border-color:#5cc8ff}.mode0 .stateDot{background:#39d98a}.mode1 .stateDot{background:#ffcc66}.mode2 .stateDot{background:#5cc8ff}.parkLocked{border-color:#ff6b6b;animation:pulse 1.2s infinite}.parkUnlocked{border-color:#39d98a}.parkLocked .stateDot{background:#ff6b6b}.parkUnlocked .stateDot{background:#39d98a}.driftOff{border-color:#475569}.driftArmed{border-color:#ffcc66}.driftActive{border-color:#d96bff;animation:pulse 1s infinite}.driftBar{height:6px;background:#273142;border-radius:999px;margin-top:10px;position:relative}.driftBar i{position:absolute;top:-3px;width:4px;height:12px;background:#d96bff;border-radius:2px;left:50%;transition:left .2s}.driftActive:before{content:"";position:absolute;inset:0;background:linear-gradient(90deg,transparent,rgba(217,107,255,.16),transparent);animation:scan 1.4s infinite}.rcGrid{display:grid;grid-template-columns:repeat(6,minmax(72px,1fr));gap:6px;margin-top:10px}.rcCell{background:#0d1219;border:1px solid #2b3441;border-radius:8px;padding:8px;text-align:center}.rcCell b{display:block;color:#8fa1b5;font-size:11px}.rcCell span{font:700 18px Consolas,monospace}.rcCell.modeCh{border-color:#ffcc66}.row{display:flex;gap:6px;flex-wrap:wrap;align-items:center}button,input{font-size:15px;border-radius:6px;border:1px solid #3b4655;background:#222b36;color:#eef;padding:8px}button{cursor:pointer}button:hover{background:#2d3948}input{flex:0 1 180px;min-width:120px;max-width:220px}.formRow{display:flex;gap:8px;align-items:center;margin-top:8px}.formRow label{width:42px;color:#8fa1b5;font-size:13px;font-weight:700;text-align:right}.inputWithAction{position:relative;display:flex;gap:6px;align-items:center;flex:1}.inputWithAction input{flex:1;min-width:0;max-width:none}.iconButton{min-width:0;width:38px;padding:8px}.scanPopover{display:none;position:absolute;left:0;right:0;top:42px;max-height:210px;overflow:auto;background:#111820;border:1px solid #5cc8ff;border-radius:10px;padding:8px;z-index:12}.scanPopover.show{display:block}.scanRow{display:flex;justify-content:space-between;gap:8px;width:100%;text-align:left;margin-top:4px}.scanMeta{color:#8fa1b5;font-size:12px;white-space:nowrap}.modal{position:fixed;inset:0;display:none;align-items:center;justify-content:center;background:rgba(5,7,10,.72);z-index:10}.modal.show{display:flex}.toast{position:fixed;right:18px;bottom:18px;background:#111820;border:1px solid #39d98a;border-radius:12px;padding:12px 14px;box-shadow:0 12px 40px rgba(0,0,0,.35);color:#e8edf2;opacity:0;transform:translateY(12px);transition:.25s;pointer-events:none;z-index:20}.toast.show{opacity:1;transform:translateY(0)}.helpFab{position:fixed;right:18px;bottom:18px;width:46px;height:46px;min-width:0;padding:0;border-radius:50%;background:#5cc8ff;color:#061019;border-color:#5cc8ff;font-size:24px;font-weight:900;line-height:1;z-index:17;box-shadow:0 12px 32px rgba(0,0,0,.35)}.helpFab:hover{background:#8bdcff}.helpOverlay{position:fixed;inset:0;display:none;background:rgba(5,7,10,.45);z-index:18}.helpOverlay.show{display:block}.helpModal{position:fixed;right:18px;bottom:74px;width:min(340px,calc(100vw - 36px));display:none;background:linear-gradient(135deg,#1c2430,#121821);border:1px solid #5cc8ff;border-radius:14px;padding:14px;box-shadow:0 18px 60px rgba(0,0,0,.45);z-index:19}.helpModal.show{display:block}.helpHead{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:8px}.helpClose{min-width:0;width:30px;height:30px;padding:0;border-radius:50%;font-size:18px;line-height:1}.helpList{margin:0;padding-left:18px;color:#dbeafe;font-size:13px;line-height:1.55}.dialog{width:min(420px,calc(100vw - 28px));background:linear-gradient(135deg,#1c2430,#121821);border:1px solid #ffcc66;border-radius:14px;padding:18px;box-shadow:0 18px 60px rgba(0,0,0,.45)}.dialog h2{margin:0 0 8px;font-size:20px}.dialog p{color:#b7c6d8;font-size:14px;line-height:1.5}.dialogActions{display:flex;gap:8px;justify-content:flex-end;margin-top:14px}.chartControls{display:flex;gap:6px;flex-wrap:wrap;align-items:center;margin-top:8px}.chartTools{margin-left:auto;display:flex;gap:6px;flex-wrap:wrap}#serialPanel{display:flex;flex-direction:column}.log{height:calc(5 * 1.35em + 16px);overflow:auto;background:#05070a;color:#d7ffe0;font:13px/1.35 Consolas,monospace;padding:8px;border-radius:6px;white-space:pre-wrap}#serialPanel .log{flex:0 1 auto;min-height:calc(5 * 1.35em + 16px);max-height:calc(20 * 1.35em + 16px)}.muted{color:#8fa1b5;font-size:12px}canvas{width:100%;height:auto;aspect-ratio:38/13;background:#070a0f;border-radius:6px;border:1px solid #2b3441}.legend{display:flex;gap:18px;align-items:flex-start;flex-wrap:wrap}.legend span{display:inline-block;font-size:12px}.legend b{display:block;color:inherit;font:700 13px Consolas,monospace;margin-top:2px}#chartPanel:fullscreen{background:#101318;padding:12px;display:flex;flex-direction:column}#chartPanel:fullscreen canvas{width:min(100%,calc((100vh - 118px) * 38 / 13));height:auto;max-height:calc(100vh - 118px);aspect-ratio:38/13}.c1{color:#39d98a}.c2{color:#5cc8ff}.c3{color:#ffcc66}.c4{color:#ff6b6b}.c5{color:#d96bff}.c6{color:#f472b6}.c7{color:#a3e635}.c8{color:#fb923c}@keyframes pulse{50%{box-shadow:0 0 18px rgba(255,107,107,.35);transform:translateY(-1px)}}@keyframes scan{from{transform:translateX(-100%)}to{transform:translateX(100%)}}@media(max-width:900px){.statusTable{grid-template-columns:repeat(2,minmax(0,1fr))}}@media(max-width:860px){.stateGrid{grid-template-columns:minmax(96px,.30fr) minmax(160px,.56fr) minmax(260px,1.30fr);grid-template-areas:"mode park drift" "voltage network network"}}@media(max-width:620px){.stateGrid{grid-template-columns:84px 154px 100px;grid-template-areas:"mode park voltage" "drift drift drift" "network network network"}#modeCard,#parkCard,#voltageCard{padding:10px 8px}#modeCard .stateValue,#parkCard .stateValue,#voltageCard .stateValue,#driftCard .stateValue,#networkCard .stateValue{font-size:18px}#modeCard .stateSub,#parkCard .stateSub,#driftCard .stateSub{font-size:11px}#voltageCard .stateMeta span,#networkCard .stateMeta span{font-size:13px}#modeCard .stateHead,#parkCard .stateHead,#voltageCard .stateHead,#driftCard .stateHead,#networkCard .stateHead{font-size:11px}.rcGrid{grid-template-columns:repeat(3,minmax(72px,1fr))}}@media(max-width:560px){.statusTable{grid-template-columns:1fr}}@media(min-width:900px){.grid{grid-template-columns:2fr 1fr}.wide{grid-column:1/-1}#diagnosticsPanel{grid-column:1/-1}#serialPanel .log{height:calc(20 * 1.35em + 16px)}}
</style>
<style>
.langFab{position:fixed;right:18px;bottom:74px;width:46px;height:46px;min-width:0;padding:0;border-radius:50%;background:#2563eb;color:#eef;border-color:#5cc8ff;font-size:23px;font-weight:900;line-height:1;z-index:17;box-shadow:0 12px 32px rgba(0,0,0,.35)}.langFab:hover{background:#3b82f6}.langMenu{position:fixed;right:72px;bottom:74px;display:none;min-width:132px;background:#111820;border:1px solid #5cc8ff;border-radius:12px;padding:6px;z-index:17;box-shadow:0 12px 32px rgba(0,0,0,.35)}.langMenu.show{display:block}.langMenu button{display:block;width:100%;min-width:0;text-align:left;margin:2px 0;padding:7px 10px}.langMenu button.active{background:#5cc8ff;color:#061019;font-weight:800}
</style>
</head>
<body>
<div class="headerRow"><h1 data-i18n="app.title">DonkeyDrift Console</h1><span class="version" id="versionLabel">--</span><a href="/update" target="_blank" class="otaLink"><button class="otaButton" data-i18n="button.ota">OTA</button></a><label class="toggleSwitch" id="devModeToggle" style="gap:4px"><span class="toggleLabel devHint">DEV <b id="devModeSwitchText">OFF</b></span><input type="checkbox" id="devModeCheck" onchange="toggleDevModeFromSwitch()"><span class="slider"></span></label></div>
<div class="grid">
<section class="panel wide">
<div class="stateGrid">
<div id="modeCard" class="stateCard"><div class="stateHead" data-i18n="state.mode">Mode</div><div class="stateValue" id="modeValue">--</div><div class="stateSub" id="modeSub">waiting</div><span class="stateDot"></span></div>
<div id="parkCard" class="stateCard"><div class="stateHead" data-i18n="state.park">Park</div><div class="stateValue" id="parkValue">--</div><div class="stateSub" id="parkSub">waiting</div><span class="stateDot"></span></div>
<div id="driftCard" class="stateCard"><div class="stateHead" data-i18n="state.drift">Drift</div><div class="stateValue" id="driftValue">--</div><div class="stateSub" id="driftSub">waiting</div><div class="driftBar"><i id="driftNeedle"></i></div><span class="stateDot"></span></div>
<div id="voltageCard" class="stateCard"><div class="stateHead" data-i18n="state.voltage">Voltage</div><div class="stateValue" id="voltageValue">--</div><div class="stateMeta"><b data-i18n="state.remain">REMAIN</b><span id="voltageSub">battery</span></div><span class="stateDot"></span></div>
<div id="networkCard" class="stateCard"><div class="netTabs"><button id="networkApTab" type="button" onclick="setNetworkTab('ap')">AP</button><button id="networkStaTab" type="button" onclick="setNetworkTab('sta')">STA</button></div><button class="gear" onclick="event.stopPropagation();openNetworkSettings()">⚙</button><div class="stateHead" data-i18n="state.network">Network</div><div class="stateValue copyValue" id="networkValue" onclick="copyNetworkIp()">--</div><div class="stateMeta"><b data-i18n="state.ssid">SSID</b><span id="networkSsidValue">--</span></div><span class="stateDot"></span></div>
</div>
</section>
<section class="panel" id="chartPanel">
<canvas id="chart" width="760" height="260"></canvas>
<div class="legend"><span class="c1">Throttle<b id="thrMeta">--</b></span><span class="c2">Steering<b id="strMeta">--</b></span><span class="c4">GyroZ<b id="gzMeta">--</b></span></div>
<div class="chartControls"><button onclick="toggleChart()" id="chartBtn" data-i18n="button.pause">暂停</button><button onclick="clearChart()" data-i18n="button.clear">清空</button><button onclick="toggleChartFullscreen()" id="chartFullscreenBtn" data-i18n="button.fullscreen">全屏</button><div class="chartTools"><button onclick="ts()" data-i18n="button.tubStart">Tub Start</button><button onclick="te()" data-i18n="button.tubStop">Tub Stop</button><button onclick="td()" data-i18n="button.tubJson">Tub JSON</button></div></div>
</section>
<section class="panel" id="serialPanel">
<div class="row"><input id="cmd"><button onclick="sendCmd()" data-i18n="button.send">发送</button><button onclick="clearLog()" data-i18n="button.clear">清空</button><button onclick="togglePause()" id="pauseBtn" data-i18n="button.pause">暂停</button></div>
<div id="log" class="log"></div>
</section>
<section class="panel wide" id="diagnosticsPanel">
<div id="rcFold" class="fold"><button class="foldHead" onclick="toggleFold('rcFold')" aria-expanded="false"><span class="foldIcon">▸</span><span data-i18n="panel.rcChannels">RC Channels</span></button><div class="foldBody"><div class="rcGrid"><div class="rcCell"><b>CH1 Steering</b><span id="ch1Value">----</span></div><div class="rcCell"><b>CH2 Throttle</b><span id="ch2Value">----</span></div><div class="rcCell"><b>CH3 Park</b><span id="ch3Value">----</span></div><div class="rcCell modeCh"><b>CH4 Mode</b><span id="ch4Value">----</span></div><div class="rcCell"><b>CH5 Drift</b><span id="ch5Value">----</span></div><div class="rcCell"><b>CH6 Scale</b><span id="ch6Value">----</span></div></div></div></div>
<div id="statusFold" class="fold"><button class="foldHead" onclick="toggleFold('statusFold')" aria-expanded="false"><span class="foldIcon">▸</span><span data-i18n="panel.statusDetails">STATUS Details</span></button><div class="foldBody"><div id="status">loading...</div></div></div>
</section>
</div>
<div id="devModeModal" class="modal"><div class="dialog"><h2 data-i18n="dev.title">开启开发模式？</h2><p data-i18n="dev.body">开发模式会持久化，并允许 Web Console 免认证保持 OTA 监听。不会放宽控制命令；实际 OTA 传输期间固件会默认 Park Locked。</p><div class="dialogActions"><button onclick="closeDevModeModal(false)" data-i18n="button.cancel">取消</button><button onclick="closeDevModeModal(true)" data-i18n="button.confirmDev">确认开启</button></div></div></div>
<div id="wifiApModal" class="modal"><div class="dialog"><h2 data-i18n="wifi.apTitle">AP SSID 配置</h2><div class="formRow"><label for="apSsid">SSID</label><div class="inputWithAction"><input id="apSsid" placeholder="AP SSID" data-i18n-placeholder="wifi.apPlaceholder"></div></div><p id="apNotice" data-i18n="wifi.apNotice">保存后会重启 AP，当前浏览器连接会短暂断开。请连接新的 SSID 后刷新页面。</p><div class="dialogActions"><button onclick="closeWifiApModal()" data-i18n="button.cancel">取消</button><button id="apSaveBtn" onclick="saveWifiAp()" data-i18n="wifi.saveRestartAp">保存并重启 AP</button></div></div></div>
<div id="wifiStaModal" class="modal"><div class="dialog"><h2 data-i18n="wifi.staTitle">STA Wi-Fi 配置</h2><div class="formRow"><label for="staSsid">SSID</label><div class="inputWithAction"><input id="staSsid" placeholder="STA SSID" data-i18n-placeholder="wifi.staPlaceholder"><button id="staSsidSearchBtn" class="iconButton" type="button" onclick="openWifiScanPopover(event)">⌕</button><div id="wifiScanPopover" class="scanPopover"><div id="wifiScanStatus" class="muted" data-i18n="wifi.scanning">扫描中...</div><div id="wifiScanList"></div></div></div></div><div class="formRow"><label for="staPassword" data-i18n="wifi.passwordLabel">密码</label><div class="inputWithAction"><input id="staPassword" type="password" placeholder="Wi-Fi 密码，留空表示开放网络" data-i18n-placeholder="wifi.passwordPlaceholder"><button id="staPasswordEye" class="iconButton" type="button" onclick="toggleStaPasswordVisibility()">👁</button></div></div><p id="staNotice" data-i18n="wifi.staNotice">注意只能连接2.4G WiFi</p><div class="dialogActions"><button onclick="closeWifiStaModal()" data-i18n="button.cancel">取消</button><button onclick="clearWifiSta()" data-i18n="button.clear">清除</button><button onclick="saveWifiSta()" data-i18n="button.connect">连接</button></div></div></div>
<div id="wifiStaFailureModal" class="modal"><div class="dialog"><h2 data-i18n="wifi.failureTitle">STA 连接失败</h2><p id="wifiStaFailureText">连接失败。</p><div class="dialogActions"><button onclick="closeWifiStaFailureModal()" data-i18n="button.ok">知道了</button><button onclick="openWifiStaModal();closeWifiStaFailureModal()" data-i18n="button.reconfigure">重新配置</button></div></div></div>
<div id="wifiStaHandoffModal" class="modal"><div class="dialog"><h2 data-i18n="wifi.handoffTitle">STA 切换提示</h2><p id="wifiStaHandoffText">设备正在切换 Wi-Fi。</p><div class="dialogActions"><button onclick="copyHandoffIp()" data-i18n="button.copyIp">复制 IP</button><button onclick="openHandoffUrl()" data-i18n="button.openNewUrl">打开新地址</button><button onclick="closeWifiStaHandoffModal()" data-i18n="button.ok">知道了</button></div></div></div>
<button id="langFab" class="langFab" onclick="toggleLanguageMenu(event)" aria-label="语言" data-i18n-aria="language.title">🌐</button>
<div id="langMenu" class="langMenu"><button type="button" data-lang="zh" onclick="setLanguage('zh')">中文</button><button type="button" data-lang="en" onclick="setLanguage('en')">English</button></div>
<button id="helpFab" class="helpFab" onclick="openHelpModal()" aria-label="功能说明" data-i18n-aria="help.title">?</button>
<div id="helpOverlay" class="helpOverlay" onclick="closeHelpModal()"></div>
<div id="helpModal" class="helpModal" role="dialog" aria-modal="true" aria-labelledby="helpTitle"><div class="helpHead"><h2 id="helpTitle" data-i18n="help.title">功能说明</h2><button class="helpClose" onclick="closeHelpModal()" aria-label="关闭功能说明" data-i18n-aria="help.close">×</button></div><ul class="helpList"><li data-i18n="help.statusCards">状态卡片：查看模式、Park、OTA、连接状态</li><li data-i18n="help.network">Network：查看 AP/STA IP，配置 Wi-Fi</li><li data-i18n="help.diagnostics">Diagnostics：运行测试、回归、维护命令</li><li data-i18n="help.serialLog">Serial Log：查看设备日志和命令反馈</li><li data-i18n="help.tubJson">Tub JSON：记录并下载遥测样本</li><li data-i18n="help.otaDev">OTA / DEV：固件更新与开发模式开关</li></ul></div>
<div id="toast" class="toast"></div>
<script>
const log=document.getElementById('log'),cmd=document.getElementById('cmd'),statusBox=document.getElementById('status'),devModeCheck=document.getElementById('devModeCheck'),devModeSwitchText=document.getElementById('devModeSwitchText'),devModeModal=document.getElementById('devModeModal'),versionLabel=document.getElementById('versionLabel'),apSsid=document.getElementById('apSsid'),apNotice=document.getElementById('apNotice'),apSaveBtn=document.getElementById('apSaveBtn'),wifiApModal=document.getElementById('wifiApModal'),staSsid=document.getElementById('staSsid'),staPassword=document.getElementById('staPassword'),staPasswordEye=document.getElementById('staPasswordEye'),staNotice=document.getElementById('staNotice'),wifiScanPopover=document.getElementById('wifiScanPopover'),wifiScanStatus=document.getElementById('wifiScanStatus'),wifiScanList=document.getElementById('wifiScanList'),wifiStaModal=document.getElementById('wifiStaModal'),wifiStaFailureModal=document.getElementById('wifiStaFailureModal'),wifiStaFailureText=document.getElementById('wifiStaFailureText'),wifiStaHandoffModal=document.getElementById('wifiStaHandoffModal'),wifiStaHandoffText=document.getElementById('wifiStaHandoffText'),toast=document.getElementById('toast'),networkCard=document.getElementById('networkCard'),networkApTab=document.getElementById('networkApTab'),networkStaTab=document.getElementById('networkStaTab'),networkValue=document.getElementById('networkValue'),networkSsidValue=document.getElementById('networkSsidValue'),voltageCard=document.getElementById('voltageCard'),voltageValue=document.getElementById('voltageValue'),voltageSub=document.getElementById('voltageSub'),chartPanel=document.getElementById('chartPanel'),canvas=document.getElementById('chart'),ctx=canvas.getContext('2d'),thrMeta=document.getElementById('thrMeta'),strMeta=document.getElementById('strMeta'),gzMeta=document.getElementById('gzMeta'),modeCard=document.getElementById('modeCard'),modeValue=document.getElementById('modeValue'),modeSub=document.getElementById('modeSub'),parkCard=document.getElementById('parkCard'),parkValue=document.getElementById('parkValue'),parkSub=document.getElementById('parkSub'),driftCard=document.getElementById('driftCard'),driftValue=document.getElementById('driftValue'),driftSub=document.getElementById('driftSub'),driftNeedle=document.getElementById('driftNeedle'),chValues=[1,2,3,4,5,6].map(n=>document.getElementById('ch'+n+'Value'));
const langFab=document.getElementById('langFab'),langMenu=document.getElementById('langMenu');
const LANG_STORAGE_KEY='mus4.ui.lang';
const I18N={zh:{'app.title':'DonkeyDrift Console','language.title':'语言','button.ota':'OTA','button.pause':'暂停','button.resume':'继续','button.draw':'绘制','button.clear':'清空','button.fullscreen':'全屏','button.split':'分屏','button.send':'发送','button.tubStart':'Tub Start','button.tubStop':'Tub Stop','button.tubJson':'Tub JSON','button.cancel':'取消','button.confirmDev':'确认开启','button.connect':'连接','button.ok':'知道了','button.reconfigure':'重新配置','button.copyIp':'复制 IP','button.openNewUrl':'打开新地址','wifi.saveRestartAp':'保存并重启 AP','state.mode':'Mode','state.park':'Park','state.drift':'Drift','state.voltage':'Voltage','state.network':'Network','state.remain':'REMAIN','state.ssid':'SSID','panel.rcChannels':'RC Channels','panel.statusDetails':'STATUS Details','dev.title':'开启开发模式？','dev.body':'开发模式会持久化，并允许 Web Console 免认证保持 OTA 监听。不会放宽控制命令；实际 OTA 传输期间固件会默认 Park Locked。','wifi.apTitle':'AP SSID 配置','wifi.apPlaceholder':'AP SSID','wifi.apNotice':'保存后会重启 AP，当前浏览器连接会短暂断开。请连接新的 SSID 后刷新页面。','wifi.staTitle':'STA Wi-Fi 配置','wifi.staPlaceholder':'STA SSID','wifi.passwordLabel':'密码','wifi.passwordPlaceholder':'Wi-Fi 密码，留空表示开放网络','wifi.scanning':'扫描中...','wifi.staNotice':'注意只能连接2.4G WiFi','wifi.hidePassword':'隐藏密码','wifi.showPassword':'显示密码','wifi.failureTitle':'STA 连接失败','wifi.handoffTitle':'STA 切换提示','help.title':'功能说明','help.close':'关闭功能说明','help.statusCards':'状态卡片：查看模式、Park、OTA、连接状态','help.network':'Network：查看 AP/STA IP，配置 Wi-Fi','help.diagnostics':'Diagnostics：运行测试、回归、维护命令','help.serialLog':'Serial Log：查看设备日志和命令反馈','help.tubJson':'Tub JSON：记录并下载遥测样本','help.otaDev':'OTA / DEV：固件更新与开发模式开关','mode.manual':'Manual input','mode.assist':'Pilot steering','mode.auto':'Pilot control','mode.unknown':'unknown','park.guarded':'output guarded','park.enabled':'drive enabled','battery':'battery','voltage.disconnected':'未连接','toast.copyFailed':'复制失败，请手动选择 IP','toast.copiedIp':'已复制 IP：','toast.newIpUnavailable':'新 IP 暂不可用，请先连接设备 AP 查看','toast.newUrlUnavailable':'新地址暂不可用，请连接设备 AP 查看','error.parkRequired':'当前操作需要 Park Locked。请将 CH3/Park 切到锁定状态后重试。','error.authRequired':'当前操作需要授权。请先 AUTH，或开启 DEV MODE 后重试。'},en:{'app.title':'DonkeyDrift Console','language.title':'Language','button.ota':'OTA','button.pause':'Pause','button.resume':'Resume','button.draw':'Draw','button.clear':'Clear','button.fullscreen':'Fullscreen','button.split':'Split','button.send':'Send','button.tubStart':'Tub Start','button.tubStop':'Tub Stop','button.tubJson':'Tub JSON','button.cancel':'Cancel','button.confirmDev':'Enable','button.connect':'Connect','button.ok':'OK','button.reconfigure':'Reconfigure','button.copyIp':'Copy IP','button.openNewUrl':'Open new URL','wifi.saveRestartAp':'Save and restart AP','state.mode':'Mode','state.park':'Park','state.drift':'Drift','state.voltage':'Voltage','state.network':'Network','state.remain':'REMAIN','state.ssid':'SSID','panel.rcChannels':'RC Channels','panel.statusDetails':'STATUS Details','dev.title':'Enable dev mode?','dev.body':'Dev mode is persistent and lets Web Console keep OTA listening without AUTH. It does not loosen control commands; firmware still defaults to Park Locked during OTA transfer.','wifi.apTitle':'AP SSID settings','wifi.apPlaceholder':'AP SSID','wifi.apNotice':'Saving restarts AP and briefly disconnects this browser. Connect to the new SSID and refresh.','wifi.staTitle':'STA Wi-Fi settings','wifi.staPlaceholder':'STA SSID','wifi.passwordLabel':'Password','wifi.passwordPlaceholder':'Wi-Fi password, leave blank for open network','wifi.scanning':'Scanning...','wifi.staNotice':'Only 2.4 GHz Wi-Fi is supported','wifi.hidePassword':'Hide password','wifi.showPassword':'Show password','wifi.failureTitle':'STA connection failed','wifi.handoffTitle':'STA handoff notice','help.title':'Help','help.close':'Close help','help.statusCards':'Status Cards: view mode, Park, OTA, and connection status','help.network':'Network: view AP/STA IP and configure Wi-Fi','help.diagnostics':'Diagnostics: run tests, regression, and maintenance commands','help.serialLog':'Serial Log: view device logs and command feedback','help.tubJson':'Tub JSON: record and download telemetry samples','help.otaDev':'OTA / DEV: firmware update and development mode switches','mode.manual':'Manual input','mode.assist':'Pilot steering','mode.auto':'Pilot control','mode.unknown':'unknown','park.guarded':'output guarded','park.enabled':'drive enabled','battery':'battery','voltage.disconnected':'Not connected','toast.copyFailed':'Copy failed, select the IP manually','toast.copiedIp':'Copied IP: ','toast.newIpUnavailable':'New IP is not available yet; connect to device AP first','toast.newUrlUnavailable':'New URL is not available yet; connect to device AP first','error.parkRequired':'This action requires Park Locked. Switch CH3/Park to locked and retry.','error.authRequired':'This action requires authorization. Send AUTH first, or enable DEV MODE and retry.'}};
let uiLang=readStoredLanguage();
let lastLogSeq=0,lastDataSeq=0,pointHead=0,pointCount=0,logPaused=false,chartPaused=false,wifiScanTimer=0,wifiScanBusy=false,apSaving=false,staPasswordPlaceholder=false,staPasswordDirty=false,staPasswordVisible=false,staSavedPassword='',staSavedPasswordKnown=false,dataPolling=false,points=new Array(256),scrollOffset=0,lastFrameTime=performance.now(),lastDrawTime=0,smoothedDt=16,dataWs=null,dataWsConnected=false,dataWsReconnectDelay=500,dataWsReconnectTimer=0,dataTransport='poll',screenSaverActive=false,screenSaverStartTime=0,parkLockedAt=0,ch1Samples=[],networkTab='auto',networkTabPinned=false,networkCopyIp='',toastTimer=0,tubRecording=false,tubSamples=[],tubStartedMs=0,tubStoppedMs=0,tubLastSeq=0,gridCanvas=document.createElement('canvas'),gridCtx=gridCanvas.getContext('2d'),gridReady=false,saverTime=0;const TUB_MAX_SAMPLES=12000;const TUB_SCHEMA='mus4.web_data_point.tub.v1';
function normalizeLanguage(lang){return lang==='en'?'en':'zh'}
function readStoredLanguage(){try{return normalizeLanguage(localStorage.getItem(LANG_STORAGE_KEY))}catch(e){return 'zh'}}
function writeStoredLanguage(lang){try{localStorage.setItem(LANG_STORAGE_KEY,lang)}catch(e){}}
function t(key){return (I18N[uiLang]&&I18N[uiLang][key])||I18N.zh[key]||key}
function refreshDynamicLabels(){document.getElementById('pauseBtn').textContent=logPaused?t('button.resume'):t('button.pause');document.getElementById('chartBtn').textContent=chartPaused?t('button.draw'):t('button.pause');document.getElementById('chartFullscreenBtn').textContent=document.fullscreenElement===chartPanel?t('button.split'):t('button.fullscreen');updateStaPasswordEye()}
function applyLanguage(lang){uiLang=normalizeLanguage(lang);document.documentElement.lang=uiLang;document.querySelectorAll('[data-i18n]').forEach(e=>{const v=t(e.dataset.i18n);if(v)e.textContent=v});document.querySelectorAll('[data-i18n-placeholder]').forEach(e=>{const v=t(e.dataset.i18nPlaceholder);if(v)e.placeholder=v});document.querySelectorAll('[data-i18n-aria]').forEach(e=>{const v=t(e.dataset.i18nAria);if(v)e.setAttribute('aria-label',v)});langMenu.querySelectorAll('button[data-lang]').forEach(b=>b.classList.toggle('active',b.dataset.lang===uiLang));refreshDynamicLabels()}
function setLanguage(lang){uiLang=normalizeLanguage(lang);writeStoredLanguage(uiLang);applyLanguage(uiLang);closeLanguageMenu()}
function toggleLanguageMenu(e){if(e)e.stopPropagation();langMenu.classList.toggle('show')}
function closeLanguageMenu(){langMenu.classList.remove('show')}
function openHelpModal(){closeLanguageMenu();helpOverlay.classList.add('show');helpModal.classList.add('show')}
function closeHelpModal(){helpOverlay.classList.remove('show');helpModal.classList.remove('show')}
function line(t){if(logPaused)return;log.textContent+=t+'\n';if(log.textContent.length>16000)log.textContent=log.textContent.slice(-12000);log.scrollTop=log.scrollHeight}
function clearLog(){log.textContent=''}
function togglePause(){logPaused=!logPaused;document.getElementById('pauseBtn').textContent=logPaused?t('button.resume'):t('button.pause')}
function toggleChart(){chartPaused=!chartPaused;document.getElementById('chartBtn').textContent=chartPaused?t('button.draw'):t('button.pause')}
function toggleChartFullscreen(){if(document.fullscreenElement===chartPanel)document.exitFullscreen();else chartPanel.requestFullscreen()}
document.addEventListener('fullscreenchange',()=>{document.getElementById('chartFullscreenBtn').textContent=document.fullscreenElement===chartPanel?t('button.split'):t('button.fullscreen');gridReady=false;draw()});
document.addEventListener('click',closeLanguageMenu);
function clearChart(){pointHead=0;pointCount=0;points.fill(null);scrollOffset=0;smoothedDt=16;gridReady=false;draw()}function enterScreenSaver(){screenSaverActive=true;screenSaverStartTime=performance.now();saverTime=0}function exitScreenSaver(){screenSaverActive=false;screenSaverStartTime=0;parkLockedAt=0;ch1Samples=[];clearChart()}
function toggleFold(id){const f=document.getElementById(id);if(!f)return;const open=!f.classList.contains('open');f.classList.toggle('open',open);const i=f.querySelector('.foldIcon'),b=f.querySelector('.foldHead');if(i)i.textContent=open?'▾':'▸';if(b)b.setAttribute('aria-expanded',open?'true':'false')}
function parseStatusPairs(t){const pairs=[],n=t.length;let i=0;while(i<n){while(i<n&&/\s/.test(t[i]))i++;let k='';while(i<n&&!/\s|=/.test(t[i]))k+=t[i++];if(!k||t[i]!=='='){while(i<n&&!/\s/.test(t[i]))i++;continue}i++;let v='';if(t[i]==='\"'){const q=t[i++];while(i<n&&t[i]!==q)v+=t[i++];if(i<n&&t[i]===q)i++}else{while(i<n&&!/\s/.test(t[i]))v+=t[i++]}pairs.push([k,v])}return pairs}
function parseStatusText(t){const m={};parseStatusPairs(t).forEach(p=>m[p[0]]=p[1]);return m}
function renderStatus(t){const pairs=parseStatusPairs(t);statusBox.textContent='';if(!pairs.length){statusBox.textContent=t;return}const table=document.createElement('div');table.className='statusTable';pairs.forEach(p=>{const r=document.createElement('div'),k=document.createElement('b'),v=document.createElement('span');r.className='statusRow';k.textContent=p[0];v.textContent=p[1];r.appendChild(k);r.appendChild(v);table.appendChild(r)});statusBox.appendChild(table)}
function ts(){tubSamples=[];tubStartedMs=0;tubStoppedMs=0;tubLastSeq=0;tubRecording=true}
function te(){if(!tubRecording)return;tubRecording=false;tubStoppedMs=tubSamples.length?tubSamples[tubSamples.length-1].t:tubStartedMs}
function tp(p){if(!tubRecording||!p||p.ch6===undefined)return;const s=Number(p.seq||0);if(s&&s===tubLastSeq)return;if(!tubSamples.length)tubStartedMs=Number(p.t||0);tubSamples.push(p);tubLastSeq=s;tubStoppedMs=Number(p.t||tubStoppedMs);if(tubSamples.length>=TUB_MAX_SAMPLES)te()}
function td(){if(!tubSamples.length)return;const x=tubRecording?(tubSamples[tubSamples.length-1].t||tubStartedMs):tubStoppedMs,p={schema:TUB_SCHEMA,source:'mus4-web-console',started_ms:tubStartedMs,stopped_ms:x,count:tubSamples.length,samples:tubSamples},b=new Blob([JSON.stringify(p)],{type:'application/json'}),a=document.createElement('a');a.href=URL.createObjectURL(b);a.download='mus4-tub.json';document.body.appendChild(a);a.click();setTimeout(()=>{URL.revokeObjectURL(a.href);a.remove()},0)}
function showToast(t,ok=true){toast.textContent=t;toast.style.borderColor=ok?'#39d98a':'#ff6b6b';toast.classList.add('show');clearTimeout(toastTimer);toastTimer=setTimeout(()=>toast.classList.remove('show'),1600)}
function fallbackCopyText(t){const a=document.createElement('textarea');a.value=t;a.style.position='fixed';a.style.left='-9999px';document.body.appendChild(a);a.focus();a.select();let ok=false;try{ok=document.execCommand('copy')}catch(e){ok=false}a.remove();return ok}
async function copyNetworkIp(){const ip=networkCopyIp;if(!ip||ip==='--'||ip==='0.0.0.0'||ip==='disabled'){showToast(t('toast.copyFailed'),false);return}try{if(navigator.clipboard&&navigator.clipboard.writeText)await navigator.clipboard.writeText(ip);else if(!fallbackCopyText(ip))throw new Error('copy failed');showToast(t('toast.copiedIp')+ip,true)}catch(e){if(fallbackCopyText(ip))showToast(t('toast.copiedIp')+ip,true);else showToast(t('toast.copyFailed'),false)}}
function setNetworkTab(t){networkTab=t;networkTabPinned=true;updateNetworkCard.last&&updateNetworkCard(updateNetworkCard.last)}
function selectedNetworkTab(){const s=updateNetworkCard.last||{};return networkTabPinned?networkTab:(s.sta_connected==='1'?'sta':'ap')}
function openNetworkSettings(){const selected=selectedNetworkTab();selected==='ap'?openWifiApModal():openWifiStaModal()}
function updateNetworkCard(s){updateNetworkCard.last=s;const ap=s.ap_ip||'--',sta=s.sta_ip||'0.0.0.0',apSsid=s.ap_ssid||'MUS4-DEBUG',staSsid=s.sta_ssid||'--',configured=s.sta_configured==='1',staConnected=s.sta_connected==='1',selected=networkTabPinned?networkTab:(staConnected?'sta':'ap');networkApTab.classList.toggle('active',selected==='ap');networkStaTab.classList.toggle('active',selected==='sta');if(selected==='ap'){networkCopyIp=ap;networkValue.textContent=ap;networkSsidValue.textContent=apSsid;networkCard.className='stateCard mode0'}else{networkCopyIp=configured?sta:'';networkValue.textContent=configured?sta:'disabled';networkSsidValue.textContent=configured?staSsid:'--';networkCard.className='stateCard '+(staConnected?'mode0':'driftOff')}if(s.version){versionLabel.textContent=s.version.replace(/^V/,'v')}}
async function refreshStatus(){try{const r=await fetch('/api/status');const t=await r.text();renderStatus(t);updateNetworkCard(parseStatusText(t))}catch(e){statusBox.textContent='status error: '+e}}
async function pollLog(){try{const r=await fetch('/api/log?since='+lastLogSeq);const j=await r.json();for(const e of j.entries){lastLogSeq=Math.max(lastLogSeq,e.seq);line('['+e.t+']['+e.src+'] '+e.line)}}catch(e){line('log error: '+e)}}
function updateState(p){const modes={0:['RC',t('mode.manual')],1:['ASSIST',t('mode.assist')],2:['AUTO',t('mode.auto')]},m=modes[p.mode]||['MODE '+p.mode,t('mode.unknown')];modeCard.className='stateCard mode'+p.mode;modeValue.textContent=m[0];modeSub.textContent=m[1];parkCard.className='stateCard '+(p.park?'parkLocked':'parkUnlocked');parkValue.textContent=p.park?'LOCKED':'UNLOCKED';parkSub.textContent=p.park?t('park.guarded'):t('park.enabled');const de=!!p.de,da=!!p.da,dc=Number(p.dc||0),gzf=Number(p.gzf||0);driftCard.className='stateCard '+(!de?'driftOff':da?'driftActive':'driftArmed');driftValue.textContent=!de?'OFF':da?'ACTIVE':'ARMED';driftSub.textContent='comp='+dc.toFixed(1)+' gzf='+gzf.toFixed(2);driftNeedle.style.left=Math.max(0,Math.min(100,(Math.max(-70,Math.min(70,dc))+70)*100/140))+'%';[p.ch1,p.ch2,p.ch3,p.ch4,p.ch5,p.ch6].forEach((v,i)=>chValues[i].textContent=v??'----');const v=Number(p.vol);if(!isNaN(v)&&v>=5){voltageValue.textContent=v.toFixed(1)+'V';const pct=Math.max(0,Math.min(100,Math.round((v-10.5)/(12.6-10.5)*100)));voltageSub.textContent=pct+'%';voltageCard.className='stateCard '+(pct>30?'mode0':pct>15?'driftArmed':'driftOff')}else{voltageValue.textContent=t('voltage.disconnected');voltageSub.textContent=t('battery');voltageCard.className='stateCard driftOff'}const parkLocked=!!p.park;if(parkLocked){if(parkLockedAt===0)parkLockedAt=performance.now()}else{parkLockedAt=0;if(screenSaverActive)exitScreenSaver()}const now=performance.now();const ch1Val=Number(p.ch1);if(!isNaN(ch1Val)){ch1Samples.push({t:now,v:ch1Val});while(ch1Samples.length>0&&now-ch1Samples[0].t>60000)ch1Samples.shift();if(ch1Samples.length>=2){let minCh1=Infinity,maxCh1=-Infinity;for(const s of ch1Samples){if(s.v<minCh1)minCh1=s.v;if(s.v>maxCh1)maxCh1=s.v}const range=maxCh1-minCh1;console.log('saver: active='+screenSaverActive+' park='+parkLocked+' ch1='+ch1Val+' range='+range.toFixed(1)+' n='+ch1Samples.length);if(!screenSaverActive&&parkLockedAt>0&&now-parkLockedAt>=10000&&range<10){enterScreenSaver()}else if(screenSaverActive&&range>=10){exitScreenSaver()}}else if(screenSaverActive&&ch1Samples.length===1){const last=ch1Samples[0].v;if(Math.abs(ch1Val-last)>=10){console.log('saver: instant exit ch1='+ch1Val+' last='+last);exitScreenSaver()}}}}
function handleDataPayload(j,transport,elapsed){const arr=j.points||[];let latest=j.latest||null;let added=0;for(const p of arr){p.req=transport==='ws'?0:elapsed;lastDataSeq=Math.max(lastDataSeq,p.seq||0);if(!chartPaused&&!screenSaverActive){addPoint(p);added++}}if(latest){lastDataSeq=Math.max(lastDataSeq,latest.seq||0);updateState(latest);tp(latest)}const p=latest||latestPoint();dataTransport=transport;if(p){thrMeta.textContent=p.thr;strMeta.textContent=p.str;gzMeta.textContent=Number(p.gz||0).toFixed(3)}if(added>0)draw()}
function decodeBinaryDataPayload(buffer){const v=new DataView(buffer);let o=0;const u8=()=>v.getUint8(o++),u16=()=>{const x=v.getUint16(o,true);o+=2;return x},u32=()=>{const x=v.getUint32(o,true);o+=4;return x},i16=()=>{const x=v.getInt16(o,true);o+=2;return x},f32=()=>{const x=v.getFloat32(o,true);o+=4;return x};if(u8()!==77||u8()!==52)throw new Error('bad magic');const version=u8();u8();if(version!==1)throw new Error('bad version');const dropped=u32(),seq=u32(),t=u32(),dt=u16(),thr=i16(),str=i16(),gz=f32(),mode=u8(),park=u8();const ch=[u16(),u16(),u16(),u16(),u16(),u16()];const latest={seq,t,dt,thr,str,gz,mode,park,ch1:ch[0],ch2:ch[1],ch3:ch[2],ch4:ch[3],ch5:ch[4],ch6:ch[5],rct:i16(),rcs:i16(),pt:i16(),ps:i16(),gzf:f32(),dc:f32(),de:u8(),da:u8(),vol:f32()};const count=u8(),points=[];for(let i=0;i<count;i++)points.push({seq:u32(),t:u32(),dt:u16(),thr:i16(),str:i16(),gz:f32()});return{type:'data',dropped,latest,points}}
function dataWsUrl(){return (location.protocol==='https:'?'wss:':'ws:')+'//'+location.hostname+':81/'}
function scheduleDataWsReconnect(){if(dataWsReconnectTimer)return;dataWsReconnectTimer=setTimeout(()=>{dataWsReconnectTimer=0;connectDataSocket();dataWsReconnectDelay=Math.min(8000,dataWsReconnectDelay*2)},dataWsReconnectDelay)}
function connectDataSocket(){try{if(dataWs&&dataWs.readyState!==WebSocket.CLOSED)return;if(dataWs){dataWs.onclose=null;dataWs.onerror=null;try{dataWs.close()}catch(e){}}const ws=new WebSocket(dataWsUrl());dataWs=ws;ws.binaryType='arraybuffer';ws.onopen=()=>{if(dataWs!==ws){ws.close();return}dataWsConnected=true;dataWsReconnectDelay=1000;dataTransport='ws';ws.send('since:'+lastDataSeq)};ws.onmessage=e=>{if(dataWs!==ws)return;try{if(e.data instanceof ArrayBuffer){handleDataPayload(decodeBinaryDataPayload(e.data),'ws',0);return}if(e.data instanceof Blob){e.data.arrayBuffer().then(b=>{if(dataWs===ws)handleDataPayload(decodeBinaryDataPayload(b),'ws',0)}).catch(err=>line('ws parse error: '+err));return}JSON.parse(e.data)}catch(err){line('ws parse error: '+err)}};ws.onclose=()=>{if(dataWs!==ws)return;dataWsConnected=false;dataWs=null;scheduleDataWsReconnect();if(!dataPolling)setTimeout(pollData,2000)};ws.onerror=()=>{if(dataWs!==ws)return;dataWsConnected=false;try{ws.close()}catch(e){}}}catch(e){dataWsConnected=false;dataWs=null;line('ws error: '+e);scheduleDataWsReconnect();if(!dataPolling)setTimeout(pollData,2000)}}
async function pollData(){if(dataWsConnected)return;if(dataPolling)return;dataPolling=true;let delay=60;const start=performance.now();try{const r=await fetch('/api/data?since='+lastDataSeq);const j=await r.json();const elapsed=performance.now()-start;handleDataPayload(j,'poll',elapsed);delay=(j.points||[]).length?Math.max(30,Math.min(80,Math.round(elapsed*1.2))):100}catch(e){delay=160;line('data error: '+e)}finally{dataPolling=false;if(!dataWsConnected)setTimeout(pollData,delay)}}
function explainCommandError(text){if(text.includes('PARK_REQUIRED'))return t('error.parkRequired');if(text.includes('AUTH_REQUIRED')||text.includes('UNAUTHORIZED'))return t('error.authRequired');return ''}
function showCommandError(text){const msg=explainCommandError(text);if(msg)alert(msg)}
async function sendCmd(){const v=cmd.value.trim();if(!v)return;const r=await fetch('/api/cmd',{method:'POST',headers:{'Content-Type':'text/plain'},body:v});const t=await r.text();showCommandError(t);cmd.value='';refreshStatus()}
function renderDevMode(v){devModeCheck.checked=!!v;devModeSwitchText.textContent=v?'ON':'OFF'}function toggleDevModeFromSwitch(){if(devModeCheck.checked){devModeModal.classList.add('show')}else{setDevMode(false)}}
async function refreshDevMode(){try{const r=await fetch('/api/devmode');const j=await r.json();renderDevMode(!!j.enabled)}catch(e){devModeSwitchText.textContent='ERR'}}
function requestDevModeToggle(){if(devModeCheck.checked){setDevMode(false);return}devModeModal.classList.add('show')}
function closeDevModeModal(ok){devModeModal.classList.remove('show');if(ok)setDevMode(true)}
async function setDevMode(v){try{const r=await fetch('/api/devmode',{method:'POST',headers:{'Content-Type':'text/plain'},body:v?'1':'0'});if(!r.ok)throw new Error(await r.text());const j=await r.json();renderDevMode(!!j.enabled);refreshStatus()}catch(e){line('dev mode error: '+e);refreshDevMode()}}
async function refreshWifiAp(){try{const r=await fetch('/api/wifi-ap');const j=await r.json();apSsid.value=j.ssid||'';return j}catch(e){line('ap config error: '+e);return null}}
async function openWifiApModal(){apNotice.textContent='保存后会重启 AP，当前浏览器连接会短暂断开。SSID 只能使用字母、数字和短横线，且不能以短横线开头或结尾。';apSaveBtn.disabled=false;apSaveBtn.textContent='保存并重启 AP';await refreshWifiAp();wifiApModal.classList.add('show')}
function closeWifiApModal(){if(apSaving)return;wifiApModal.classList.remove('show')}
async function saveWifiAp(){if(apSaving)return;try{apSaving=true;apSaveBtn.disabled=true;apSaveBtn.textContent='正在保存...';apNotice.textContent='正在保存';const body=new URLSearchParams();body.set('ssid',apSsid.value.trim());const r=await fetch('/api/wifi-ap',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});if(!r.ok){const t=await r.text();showCommandError(t);if(t.includes('invalid_ssid'))apNotice.textContent='SSID 只能使用字母、数字和短横线，长度需为 1-32 字符，且不能以短横线开头或结尾。';else apNotice.textContent='保存失败';throw new Error(t)}showToast('AP SSID 已保存，正在重启 AP',true);wifiApModal.classList.remove('show');refreshStatus()}catch(e){line('wifi ap save error: '+e)}finally{apSaving=false;apSaveBtn.disabled=false;apSaveBtn.textContent='保存并重启 AP'}}
function isWifiStaModalOpen(){return wifiStaModal.classList.contains('show')}
function updateStaPasswordEye(){staPasswordEye.textContent=staPasswordVisible?'🙈':'👁';staPasswordEye.title=staPasswordVisible?t('wifi.hidePassword'):t('wifi.showPassword')}
function renderStaPasswordState(j,force=false){if(!j)return;if(!force&&(document.activeElement===staPassword||staPasswordDirty))return;staPasswordVisible=false;staSavedPassword='';staSavedPasswordKnown=false;if(j.password_set&&Number(j.password_len||0)>0){staPassword.value='*'.repeat(Number(j.password_len||0));staPassword.type='password';staPasswordPlaceholder=true}else{staPassword.value='';staPassword.type='password';staPasswordPlaceholder=false}staPasswordDirty=false;updateStaPasswordEye()}
async function refreshWifiSta(forceFill=false){try{const r=await fetch('/api/wifi-sta');const j=await r.json();if(forceFill||(!isWifiStaModalOpen()&&document.activeElement!==staSsid))staSsid.value=j.ssid||'';renderStaPasswordState(j,forceFill);if(j.handoff_active&&j.handoff_sta_ip&&j.handoff_sta_ip!=='0.0.0.0')showWifiStaHandoffModal(j);return j}catch(e){line('sta config error: '+e);return null}}
function openWifiScanPopover(e){if(e)e.stopPropagation();wifiScanPopover.classList.add('show');refreshWifiScan();if(!wifiScanTimer)wifiScanTimer=setInterval(refreshWifiScan,1000)}
function closeWifiScanPopover(){wifiScanPopover.classList.remove('show');if(wifiScanTimer){clearInterval(wifiScanTimer);wifiScanTimer=0}wifiScanBusy=false}
async function refreshWifiScan(){if(wifiScanBusy)return;wifiScanBusy=true;try{const r=await fetch('/api/wifi-sta/scan');const j=await r.json();const nets=(j.networks||[]).sort((a,b)=>(b.rssi||-999)-(a.rssi||-999));wifiScanList.textContent='';if(!nets.length){wifiScanStatus.textContent=j.scanning?'扫描中...':'未扫描到 2.4G WiFi'}else{wifiScanStatus.textContent=j.scanning?'扫描中...':'选择 2.4G WiFi';nets.forEach(n=>{const b=document.createElement('button'),m=document.createElement('span');b.className='scanRow';b.type='button';b.onclick=()=>selectWifiSsid(n.ssid);b.textContent=n.ssid;m.className='scanMeta';m.textContent=(n.rssi||0)+' dBm CH'+(n.channel||'?')+(n.secure?' 🔒':' OPEN');b.appendChild(m);wifiScanList.appendChild(b)})}}catch(e){wifiScanStatus.textContent='扫描失败';line('wifi scan error: '+e)}finally{wifiScanBusy=false}}
function selectWifiSsid(ssid){staSsid.value=ssid;staPassword.value='';staPasswordPlaceholder=false;staPasswordDirty=false;staPasswordVisible=false;staSavedPassword='';staSavedPasswordKnown=false;updateStaPasswordEye();closeWifiScanPopover();staPassword.focus()}
async function fetchSavedStaPassword(){const r=await fetch('/api/wifi-sta/password');if(!r.ok){showCommandError(await r.text());return null}const j=await r.json();staSavedPassword=j.password||'';staSavedPasswordKnown=true;return staSavedPassword}
function maskStaPassword(){if(staSavedPasswordKnown&&!staPasswordDirty){staPassword.value='*'.repeat(staSavedPassword.length);staPasswordPlaceholder=staSavedPassword.length>0}else if(staPasswordPlaceholder){staPassword.value='*'.repeat(staPassword.value.length)}staPassword.type='password';staPasswordVisible=false;updateStaPasswordEye()}
async function toggleStaPasswordVisibility(){if(staPasswordVisible){maskStaPassword();return}if(staPasswordPlaceholder&&!staPasswordDirty){const p=await fetchSavedStaPassword();if(p===null)return;staPassword.value=p;staPasswordPlaceholder=false}else{staSavedPassword='';staSavedPasswordKnown=false}staPassword.type='text';staPasswordVisible=true;updateStaPasswordEye()}
async function openWifiStaModal(){closeWifiScanPopover();staNotice.textContent='注意只能连接2.4G WiFi';await refreshWifiSta(true);wifiStaModal.classList.add('show')}
function closeWifiStaModal(){closeWifiScanPopover();maskStaPassword();wifiStaModal.classList.remove('show')}
function closeWifiStaFailureModal(){wifiStaFailureModal.classList.remove('show')}
function closeWifiStaHandoffModal(){wifiStaHandoffModal.classList.remove('show')}
function handoffStaUrl(j){const ip=(j&&j.handoff_sta_ip)||'';return ip&&ip!=='0.0.0.0'?'http://'+ip+'/':''}
function showWifiStaHandoffModal(j){const target=(j&&j.handoff_target_ssid)||staSsid.value.trim()||'--',ap=(j&&j.handoff_ap_ssid)||'--',apUrl=(j&&j.handoff_ap_url)||'http://192.168.4.1/',url=handoffStaUrl(j);wifiStaHandoffText.textContent='请将电脑/手机切换到 Wi-Fi：'+target+'\n然后打开：'+(url||'等待设备获取新 IP')+'\n如果当前页面断开，请连接设备 AP：'+ap+'，再打开 '+apUrl+' 查看新 IP。';wifiStaHandoffModal.classList.add('show')}
async function copyHandoffIp(){const j=await refreshWifiSta(false),url=handoffStaUrl(j),ip=url.replace(/^http:\/\//,'').replace(/\/$/,'');if(!ip){showToast(t('toast.newIpUnavailable'),false);return}try{if(navigator.clipboard&&navigator.clipboard.writeText)await navigator.clipboard.writeText(ip);else if(!fallbackCopyText(ip))throw new Error('copy failed');showToast(t('toast.copiedIp')+ip,true)}catch(e){showToast(t('toast.copyFailed'),false)}}
async function openHandoffUrl(){const j=await refreshWifiSta(false),url=handoffStaUrl(j);if(!url){showToast(t('toast.newUrlUnavailable'),false);return}location.href=url}
function showWifiStaFailureModal(j){const ssid=(j&&j.ssid)||staSsid.value.trim()||'--',reason=(j&&j.last_error_message)||'STA 连接失败，请检查 SSID、密码与路由器状态。';wifiStaFailureText.textContent='SSID：'+ssid+'\n原因：'+reason+'\n建议：检查 SSID、密码、路由器距离后重新保存。';wifiStaFailureModal.classList.add('show')}
async function probeStaConsoleUrl(url){try{await fetch(url,{mode:'no-cors',cache:'no-store'});return true}catch(e){return false}}
async function redirectToStaConsole(ip){const url='http://'+ip+'/';staNotice.textContent='STA 已连接，IP：'+ip+'，正在跳转到 '+url;showToast('STA 已连接，正在跳转到 '+url,true);setTimeout(()=>{location.href=url},100);return true}
async function waitWifiStaConnectionResult(){const deadline=Date.now()+22000;while(Date.now()<deadline){let j=null;try{j=await refreshWifiSta(false)}catch(e){j=null}if(!j){await new Promise(resolve=>setTimeout(resolve,500));continue}if(j&&j.connected){staNotice.textContent='STA 已连接，IP：'+j.sta_ip+'，AP 保持开启，可继续通过 AP 配置';await refreshStatus();cmd.value='';if(j.handoff_active){showWifiStaHandoffModal(j);return true}if(j.sta_ip&&j.sta_ip!=='0.0.0.0'){await redirectToStaConsole(j.sta_ip)}closeWifiStaModal();return true}if(j&&(j.last_error||j.timed_out)){staNotice.textContent='连接失败';showWifiStaFailureModal(j);return false}await new Promise(resolve=>setTimeout(resolve,1000))}staNotice.textContent='连接失败';showWifiStaFailureModal({ssid:staSsid.value.trim(),last_error_message:'STA 连接超时，请检查 SSID、密码与路由器信号。'});return false}
async function saveWifiSta(){try{closeWifiScanPopover();staNotice.textContent='正在连接';const body=new URLSearchParams();body.set('ssid',staSsid.value.trim());body.set('source',location.hostname==='192.168.4.1'?'ap':'sta');if(!staPasswordDirty&&(staPasswordPlaceholder||staSavedPasswordKnown))body.set('keep_password','1');else body.set('password',staPassword.value);const r=await fetch('/api/wifi-sta',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});if(!r.ok){const t=await r.text();staNotice.textContent='连接失败';showWifiStaFailureModal({ssid:staSsid.value.trim(),last_error_message:t});throw new Error(t)}const saved=await r.json().catch(()=>null);if(saved&&saved.state&&saved.state.handoff_active){showWifiStaHandoffModal(saved.state);staNotice.textContent='设备正在切换 Wi-Fi；如果页面断开，请连接设备 AP 查看新 IP。'}staPassword.value='';staPasswordPlaceholder=false;staPasswordDirty=false;staPasswordVisible=false;staSavedPassword='';staSavedPasswordKnown=false;updateStaPasswordEye();await refreshWifiSta(true);refreshStatus();await new Promise(resolve=>setTimeout(resolve,1000));await waitWifiStaConnectionResult()}catch(e){line('wifi sta save error: '+e)}}
async function clearWifiSta(){if(!confirm('确认清除并禁用 STA 配置？'))return;try{closeWifiScanPopover();const r=await fetch('/api/wifi-sta/clear',{method:'POST'});if(!r.ok){const t=await r.text();showCommandError(t);throw new Error(t)}staPassword.value='';staPasswordPlaceholder=false;staPasswordDirty=false;staPasswordVisible=false;staSavedPassword='';staSavedPasswordKnown=false;updateStaPasswordEye();await refreshWifiSta(true);refreshStatus();closeWifiStaModal()}catch(e){line('wifi sta clear error: '+e)}}
async function quick(v){cmd.value=v;await sendCmd()}
staPassword.addEventListener('input',()=>{if(staPasswordPlaceholder){staPassword.value=staPassword.value.replace(/^\*+/,'');staPasswordPlaceholder=false}staPasswordDirty=true;staSavedPasswordKnown=false});
cmd.addEventListener('keydown',e=>{if(e.key==='Enter')sendCmd()});
function addPoint(p){const dt=Number(p.dt||16);smoothedDt=smoothedDt*0.85+Math.max(0,Math.min(80,dt))*0.15;p.dts=smoothedDt;points[pointHead]=p;pointHead=(pointHead+1)%points.length;if(pointCount<points.length)pointCount++}
function latestPoint(){return pointCount?points[(pointHead+points.length-1)%points.length]:null}
function pointAt(i){return points[(pointHead-pointCount+i+points.length)%points.length]}
function map(v,min,max,h){if(max===min)return h/2;return h-(v-min)*(h/(max-min))}
function ensureGrid(){const w=canvas.width,h=canvas.height;if(gridReady&&gridCanvas.width===w&&gridCanvas.height===h)return;gridCanvas.width=w;gridCanvas.height=h;gridCtx.clearRect(0,0,w,h);gridCtx.strokeStyle='#233041';gridCtx.lineWidth=1;for(let i=0;i<5;i++){const y=20+i*(h-40)/4;gridCtx.beginPath();gridCtx.moveTo(24,y);gridCtx.lineTo(w-16,y);gridCtx.stroke()}gridReady=true}
function drawSeries(key,color,min,max){const w=canvas.width,h=canvas.height,plotX=24,plotW=w-40,plotH=h-40;if(pointCount<2)return;const stepX=plotW/255,rightX=plotX+plotW,buckets=[];for(let i=0;i<pointCount;i++){const p=pointAt(i);if(!p)continue;const x=rightX-(pointCount-1-i)*stepX;const xi=Math.round(x);if(xi<plotX-5||xi>w-16+5)continue;const y=20+map(p[key]||0,min,max,plotH);let b=buckets[xi];if(!b)buckets[xi]={min:y,max:y,xSum:x,count:1};else{if(y<b.min)b.min=y;if(y>b.max)b.max=y;b.xSum+=x;b.count++}}ctx.strokeStyle=color;ctx.beginPath();let drawn=false;for(let xi=0;xi<=w;xi++){const b=buckets[xi];if(!b)continue;const x=b.xSum/b.count;const mid=(b.min+b.max)/2;if(!drawn){ctx.moveTo(x,mid);drawn=true}else{ctx.lineTo(x,mid)}if(b.max-b.min>1){ctx.moveTo(x,b.min);ctx.lineTo(x,b.max);ctx.moveTo(x,mid)}}if(drawn)ctx.stroke()}
function draw(){const w=canvas.width,h=canvas.height;ensureGrid();ctx.clearRect(24,0,w-40,h);ctx.drawImage(gridCanvas,24,0,w-40,h,24,0,w-40,h);ctx.fillStyle='#8fa1b5';ctx.font='10px sans-serif';ctx.textAlign='right';ctx.textBaseline='middle';const yLabels=[100,50,0,-50,-100];for(let i=0;i<5;i++){ctx.fillText(String(yLabels[i]),22,20+i*(h-40)/4)}ctx.save();ctx.beginPath();ctx.rect(24,0,w-40,h);ctx.clip();ctx.lineWidth=2;drawSeries('thr','#39d98a',-100,100);drawSeries('str','#5cc8ff',-100,100);drawSeries('gz','#ff6b6b',-5,5);if(screenSaverActive){ctx.fillStyle='#5cc8ff';ctx.font='20px sans-serif';ctx.textAlign='center';ctx.fillText('Drifting for Fun~',w/2,h/2)}ctx.restore()}
function renderLoop(now){requestAnimationFrame(renderLoop);let dt=Math.min(100,now-lastFrameTime);lastFrameTime=now;if(document.hidden||chartPaused)return;if(screenSaverActive){const stepX=(canvas.width-40)/255;scrollOffset+=dt/18*stepX;scrollOffset=Math.min(scrollOffset,stepX*1.5);while(scrollOffset>=stepX){addPoint({seq:0,t:saverTime,dt:16,thr:95*Math.sin(saverTime/400),str:70*Math.sin(saverTime/550+1),gz:3.5*Math.sin(saverTime/300+2)});saverTime+=16;scrollOffset-=stepX}if(now-lastDrawTime>=16){lastDrawTime=now;draw()}}}
applyLanguage(uiLang);refreshStatus();refreshDevMode();refreshWifiSta();setInterval(refreshStatus,5000);setInterval(refreshWifiSta,5000);setInterval(pollLog,1000);connectDataSocket();setTimeout(()=>{if(!dataWsConnected)pollData()},1200);draw();requestAnimationFrame(renderLoop);
</script>
</body>
</html>
)rawliteral";

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
        "<body><a href=\"" + url + "\">打开 DonkeyDrift Console</a></body></html>";
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

static const char WIFI_WEB_UPDATE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MUS4 OTA Update</title>
<style>
body{font-family:system-ui,sans-serif;margin:24px auto;max-width:480px;background:#101318;color:#e8edf2}
h1{font-size:20px;margin:0 0 16px}
#drop{border:2px dashed #475569;border-radius:12px;padding:40px 20px;text-align:center;transition:.2s;background:#171c24}
#drop.dragover{border-color:#5cc8ff;background:#1c2430}
#progress{width:100%;height:8px;background:#2b3441;border-radius:4px;margin-top:16px;overflow:hidden;display:none}
#progressBar{height:100%;width:0%;background:#5cc8ff;transition:.2s}
#status{margin-top:12px;font-size:14px;color:#8fa1b5;min-height:20px}
button{background:#5cc8ff;color:#0f1419;border:none;padding:10px 20px;border-radius:6px;font-weight:700;cursor:pointer;font-size:14px}
button:disabled{opacity:.5;cursor:not-allowed}
.muted{color:#667;font-size:12px;margin-top:12px}
</style>
</head>
<body>
<h1>MUS4 HTTP OTA</h1>
<div id="drop">
<div>拖放固件文件到此处，或 <button onclick="document.getElementById('file').click()">选择文件</button></div>
<input type="file" id="file" style="display:none" accept=".bin">
</div>
<div id="progress"><div id="progressBar"></div></div>
<div id="status">等待文件...</div>
<div class="muted">OTA 传输期间车辆会自动 Park Locked。需要认证 + Park 锁定（或开发模式）。</div>
<script>
const drop=document.getElementById('drop'),fileInput=document.getElementById('file'),progress=document.getElementById('progress'),bar=document.getElementById('progressBar'),status=document.getElementById('status');
function setStatus(t,c){status.textContent=t;status.style.color=c||'#8fa1b5'}
['dragenter','dragover','dragleave','drop'].forEach(e=>{drop.addEventListener(e,ev=>{ev.preventDefault();ev.stopPropagation()})});
['dragenter','dragover'].forEach(e=>drop.addEventListener(e,()=>drop.classList.add('dragover')));
['dragleave','drop'].forEach(e=>drop.addEventListener(e,()=>drop.classList.remove('dragover')));
drop.addEventListener('drop',e=>upload(e.dataTransfer.files[0]));
fileInput.addEventListener('change',e=>upload(e.target.files[0]));
async function upload(f){
  if(!f)return;
  setStatus('上传中...');
  progress.style.display='block';bar.style.width='0%';
  const form=new FormData();form.append('firmware',f);
  const xhr=new XMLHttpRequest();
  xhr.upload.addEventListener('progress',e=>{if(e.lengthComputable){bar.style.width=Math.round(e.loaded/e.total*100)+'%'}});
  xhr.addEventListener('load',()=>{
    if(xhr.status===200){setStatus('成功: '+xhr.responseText,'#39d98a');setTimeout(()=>location.href='/',3000)}
    else{setStatus('错误 '+xhr.status+': '+xhr.responseText,'#ff6666')}
  });
  xhr.addEventListener('error',()=>setStatus('网络错误','#ff6666'));
  xhr.open('POST','/update');xhr.send(form);
}
</script>
</body>
</html>
)rawliteral";

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
static bool processLocalOtaMaintenanceCommand(const String& line, Print& out, SerialBuf& sb)
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

bool parseAndValidateCommand(String cmd, int* throttle, int* steering)
{
    int colonIndex = cmd.indexOf(':');
    if (colonIndex <= 0)
    {
        return false;
    }

    String throttleStr = cmd.substring(0, colonIndex);
    String steeringStr = cmd.substring(colonIndex + 1);

    int t = throttleStr.toInt();
    int s = steeringStr.toInt();

    // Validate range: -100 to 100
    if (t < -100 || t > 100 || s < -100 || s > 100)
    {
        // Print errors only when this is not a test command, to avoid polluting output
        // Serial.print("[CMD ERROR] Out of range: T=");
        // Serial.print(t);
        // Serial.print(" S=");
        // Serial.println(s);
        return false;
    }

    *throttle = t;
    *steering = s;
    return true;
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

void update_drift_assist_control(bool driftValid, bool driftScaleValid)
{
    if (driftScaleValid) {
        uint16_t scalePwm = constrain(pwm_filtered[CH_DRIFT_SCALE], 1000, 2000);
        drift_assist_scale = (scalePwm - 1000) / 500.0f;
    } else {
        drift_assist_scale = 1.0f;
    }

    bool enabled = driftValid && pwm_filtered[CH_DRIFT] > 1500;
    if (!enabled) {
        drift_assist_active = false;
        drift_compensation = 0.0f;
        gyro_z_filtered = 0.0f;
    }
    drift_assist_enabled = enabled;
}


bool I2CRead(uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t *Data)
{
    bool ret = true;

    // Set register address
    Wire.beginTransmission(Address);
    Wire.write(Register);
    if (Wire.endTransmission())
    {
        ret = false;
        // Serial.println("I2C Read Errro"); // Suppress
    }

    // Read Nbytes
    Wire.requestFrom(Address, Nbytes);
    uint8_t index = 0;
    while (Wire.available())
    {
        Data[index++] = Wire.read();
    }

    return ret;
}

uint16_t I2CReadValue(uint8_t addr, uint8_t reg)
{
    uint16_t ret = -1;

    uint8_t data[2];
    if (I2CRead(addr, reg, 2, data))
    {
        ret = (uint16_t)data[0] << 8 | data[1];
    }

    return ret;
}

void I2CWriteValue(uint8_t Address, uint8_t Register, uint16_t Data)
{
    uint8_t *pData = (uint8_t *)&Data;

    // Set register address
    Wire.beginTransmission(Address);
    Wire.write(Register);
    Wire.write(pData[1]);
    Wire.write(pData[0]);
    if (Wire.endTransmission())
    {
        // Serial.println("I2C Write Error"); // Suppress
    }
}

const char *identifyI2CDeviceByAddress(uint8_t address)
{
    switch (address)
    {
    case 0x3C:
    case 0x3D:
        return "SSD1306 OLED / SH1106";
    case 0x40:
        return "INA219 / PCA9685";
    case 0x41:
        return "INA219 (alt address)";
    case 0x48:
    case 0x49:
    case 0x4A:
    case 0x4B:
        return "ADS1115 / TMP102";
    case 0x68:
    case 0x69:
        return "MPU6050 / MPU9250 / DS3231";
    case 0x76:
    case 0x77:
        return "BME280 / BMP280";
    default:
        return "Unknown";
    }
}

bool I2CReadRegister8(uint8_t address, uint8_t reg, uint8_t *value)
{
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)
    {
        return false;
    }

    if (Wire.requestFrom((int)address, 1) != 1)
    {
        return false;
    }

    *value = Wire.read();
    return true;
}

bool probeMPU6050AtAddress(uint8_t address, uint8_t *whoAmI)
{
    const uint8_t MPU6050_WHO_AM_I_REG = 0x75;
    uint8_t id = 0;

    if (!I2CReadRegister8(address, MPU6050_WHO_AM_I_REG, &id))
    {
        return false;
    }

    *whoAmI = id;
    return (id == 0x68 || id == 0x69);
}

void printLastI2CScanSummary()
{
    mus4LogLine("i2c", "Last scan summary:");
    if (g_i2cScanCount == 0)
    {
        mus4LogLine("i2c", "No devices recorded in last scan");
        return;
    }

    for (uint8_t i = 0; i < g_i2cScanCount; i++)
    {
        uint8_t addr = g_i2cScanAddresses[i];
        mus4Logf("i2c", "0x%02X - %s", addr, identifyI2CDeviceByAddress(addr));
    }
}

void read_ina219()
{
    // Read INA219 data
    float shuntvoltage = ina219.getShuntVoltage_mV();
    float busvoltage = ina219.getBusVoltage_V();
    float current_mA = ina219.getCurrent_mA();
    float power_mW = ina219.getPower_mW();
    float loadvoltage = busvoltage + (shuntvoltage / 1000);
    
    // Check data validity
    if (current_mA == 0 && busvoltage == 0 && power_mW == 0)
    {
        ina219Data.valid = false;
        return;
    }
    
    // Store data in global variables
    ina219Data.readCount++;
    ina219Data.lastReadTime = millis();
    ina219Data.busVoltage = busvoltage;
    ina219Data.shuntVoltage = shuntvoltage;
    ina219Data.loadVoltage = loadvoltage;
    ina219Data.current_mA = current_mA;
    ina219Data.power_mW = power_mW;
    ina219Data.valid = true;
}

void setup_ina219()
{
    mus4LogLine("ina219", "Initializing INA219 sensor...");

    if (!ina219.begin())
    {
        mus4LogLine("ina219", "ERROR Failed to find INA219 chip");
        mus4LogLine("ina219", "ERROR Please check I2C connection (SDA: GPIO 21, SCL: GPIO 22)");
        mus4LogLine("ina219", "ERROR Possible causes: address mismatch, wiring issue, power supply issue");
        while (1)
        {
            delay(1000);
            mus4LogLine("ina219", "ERROR Sensor not detected, waiting...");
        }
    }

    mus4LogLine("ina219", "Sensor initialized successfully!");

    // Use default calibration (32V, 2A range)
    // For higher precision, uncomment either line below:
    // ina219.setCalibration_32V_1A();  // 32V, 1A range (higher precision)
    // ina219.setCalibration_16V_400mA(); // 16V, 400mA range (highest precision)

    mus4LogLine("ina219", "Calibration: 32V, 2A range (default)");
    mus4LogLine("ina219", "Setup complete, ready for data acquisition");
}

void read_mpu6050()
{
    /* Get new sensor events with the readings */
    sensors_event_t a, g, temp;
    
    if (!mpu.getEvent(&a, &g, &temp))
    {
        mpu6050Data.valid = false;
        return;
    }
    
    // Store data in global variables
    mpu6050Data.readCount++;
    mpu6050Data.lastReadTime = millis();
    mpu6050Data.accelX = a.acceleration.x;
    mpu6050Data.accelY = a.acceleration.y;
    mpu6050Data.accelZ = a.acceleration.z;
    mpu6050Data.gyroX = g.gyro.x;
    mpu6050Data.gyroY = g.gyro.y;
    mpu6050Data.gyroZ = g.gyro.z;
    mpu6050Data.temperature = temp.temperature;
    mpu6050Data.valid = true;
}

void scanI2CBus()
{
    mus4LogLine("i2c", "Scanning I2C bus...");
    byte error, address;
    int nDevices = 0;
    g_mpuCandidateAddress = 0;
    g_mpuWhoAmIValue = 0;
    g_i2cScanCount = 0;

    for(address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0)
        {
            mus4Logf("i2c", "Found device at 0x%02X - %s", address, identifyI2CDeviceByAddress(address));
            if (g_i2cScanCount < sizeof(g_i2cScanAddresses))
            {
                g_i2cScanAddresses[g_i2cScanCount++] = address;
            }

            if (address == 0x68 || address == 0x69)
            {
                uint8_t whoAmI = 0;
                if (probeMPU6050AtAddress(address, &whoAmI))
                {
                    g_mpuCandidateAddress = address;
                    g_mpuWhoAmIValue = whoAmI;
                    mus4Logf("i2c", "MPU probe OK at 0x%02X (WHO_AM_I=0x%02X)", address, whoAmI);
                }
                else if (I2CReadRegister8(address, 0x75, &whoAmI))
                {
                    mus4Logf("i2c", "MPU-family device at 0x%02X but WHO_AM_I=0x%02X (not MPU6050)", address, whoAmI);
                }
                else
                {
                    mus4Logf("i2c", "Could not read WHO_AM_I at 0x%02X", address);
                }
            }
            nDevices++;
        }
        else if (error == 4)
        {
            mus4Logf("i2c", "Unknown error at 0x%02X", address);
        }
    }

    if (nDevices == 0)
    {
        mus4LogLine("i2c", "No I2C devices found!");
    }
    else
    {
        mus4Logf("i2c", "Found %d device(s)", nDevices);
    }
}

bool tryInitMPU6050OnCurrentBus(uint8_t *activeAddress, int maxRetriesPerAddress)
{
    uint8_t tryAddress[2] = {0x68, 0x69};
    if (g_mpuCandidateAddress == 0x69)
    {
        tryAddress[0] = 0x69;
        tryAddress[1] = 0x68;
    }

    for (int i = 0; i < 2; i++)
    {
        for (int retryCount = 1; retryCount <= maxRetriesPerAddress; retryCount++)
        {
            uint8_t addr = tryAddress[i];
            mus4Logf("mpu6050", "Try addr 0x%02X (attempt %d/%d)", addr, retryCount, maxRetriesPerAddress);

            if (mpu.begin(addr, &Wire))
            {
                *activeAddress = addr;
                return true;
            }

            mus4Logf("mpu6050", "Init failed at 0x%02X", addr);
            delay(300);
        }
    }

    return false;
}

void setup_mpu6050()
{
    mus4LogLine("mpu6050", "Initializing MPU6050 sensor...");
    uint8_t activeAddress = 0;
    const int maxRetriesPerAddress = 2;
    bool initOk = tryInitMPU6050OnCurrentBus(&activeAddress, maxRetriesPerAddress);

    if (!initOk)
    {
        mus4LogLine("mpu6050", "Retry with lower I2C speed: 100kHz");
        Wire.begin(SDA_PIN, SCL_PIN, 100000L);
        g_i2cWorkingSpeed = 100000L;
        delay(50);
        scanI2CBus();
        initOk = tryInitMPU6050OnCurrentBus(&activeAddress, maxRetriesPerAddress);
    }

    if (!initOk)
    {
        mus4LogLine("mpu6050", "Retry with lower I2C speed: 50kHz");
        Wire.begin(SDA_PIN, SCL_PIN, 50000L);
        g_i2cWorkingSpeed = 50000L;
        delay(50);
        scanI2CBus();
        initOk = tryInitMPU6050OnCurrentBus(&activeAddress, maxRetriesPerAddress);
    }

    if (!initOk)
    {
        mus4LogLine("mpu6050", "ERROR Failed to find MPU6050 chip");
        mus4LogLine("mpu6050", "ERROR Please check I2C connection (SDA: GPIO 21, SCL: GPIO 22)");
        mus4LogLine("mpu6050", "ERROR Possible causes:");
        mus4LogLine("mpu6050", "1. I2C address mismatch (try 0x68 or 0x69)");
        mus4LogLine("mpu6050", "2. Wiring issues (SDA/SCL swapped or loose)");
        mus4LogLine("mpu6050", "3. Power supply issue");
        mus4LogLine("mpu6050", "4. I2C bus speed too high");
        printLastI2CScanSummary();

        if (g_mpuCandidateAddress != 0)
        {
            mus4Logf("mpu6050", "ERROR Probe saw candidate at 0x%02X, WHO_AM_I=0x%02X",
                          g_mpuCandidateAddress, g_mpuWhoAmIValue);
        }

        unsigned long lastWaitLogMs = 0;
        unsigned long lastRescanMs = millis();
        while (1)
        {
            unsigned long now = millis();

            if (now - lastWaitLogMs >= 1000UL)
            {
                lastWaitLogMs = now;
                mus4LogLine("mpu6050", "ERROR Sensor not detected, waiting...");
            }

            if (now - lastRescanMs >= 5000UL)
            {
                lastRescanMs = now;
                mus4LogLine("mpu6050", "ERROR Auto re-scan I2C bus (5s interval)...");
                scanI2CBus();
                printLastI2CScanSummary();
            }

            delay(50);
        }
    }

    mus4LogLine("mpu6050", "Sensor initialized successfully!");
    mus4Logf("mpu6050", "Active I2C address: 0x%02X", activeAddress);
    mus4Logf("mpu6050", "Active I2C speed: %lu Hz", g_i2cWorkingSpeed);
    if (g_mpuWhoAmIValue != 0)
    {
        mus4Logf("mpu6050", "WHO_AM_I = 0x%02X", g_mpuWhoAmIValue);
    }

    // Confirm WHO_AM_I once for the initialized address to avoid misidentification caused by bus interference
    uint8_t whoAmI = 0;
    if (I2CReadRegister8(activeAddress, 0x75, &whoAmI))
    {
        mus4Logf("mpu6050", "WHO_AM_I readback: 0x%02X", whoAmI);
        if (whoAmI != 0x68 && whoAmI != 0x69)
        {
            mus4LogLine("mpu6050", "WARNING WHO_AM_I mismatch, device may not be MPU6050");
        }
    }
    else
    {
        mus4LogLine("mpu6050", "WARNING WHO_AM_I readback failed after init");
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    const char* accelRangeText = "Unknown";
    switch (mpu.getAccelerometerRange())
    {
    case MPU6050_RANGE_2_G:
        accelRangeText = "+-2G";
        break;
    case MPU6050_RANGE_4_G:
        accelRangeText = "+-4G";
        break;
    case MPU6050_RANGE_8_G:
        accelRangeText = "+-8G";
        break;
    case MPU6050_RANGE_16_G:
        accelRangeText = "+-16G";
        break;
    }
    mus4Logf("mpu6050", "Accelerometer range set to: %s", accelRangeText);

    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    const char* gyroRangeText = "Unknown";
    switch (mpu.getGyroRange())
    {
    case MPU6050_RANGE_250_DEG:
        gyroRangeText = "+- 250 deg/s";
        break;
    case MPU6050_RANGE_500_DEG:
        gyroRangeText = "+- 500 deg/s";
        break;
    case MPU6050_RANGE_1000_DEG:
        gyroRangeText = "+- 1000 deg/s";
        break;
    case MPU6050_RANGE_2000_DEG:
        gyroRangeText = "+- 2000 deg/s";
        break;
    }
    mus4Logf("mpu6050", "Gyro range set to: %s", gyroRangeText);

    mpu.setFilterBandwidth(MPU6050_BAND_94_HZ);
    const char* bandwidthText = "Unknown";
    switch (mpu.getFilterBandwidth())
    {
    case MPU6050_BAND_260_HZ:
        bandwidthText = "260 Hz";
        break;
    case MPU6050_BAND_184_HZ:
        bandwidthText = "184 Hz";
        break;
    case MPU6050_BAND_94_HZ:
        bandwidthText = "94 Hz (Sampling rate: ~100Hz)";
        break;
    case MPU6050_BAND_44_HZ:
        bandwidthText = "44 Hz";
        break;
    case MPU6050_BAND_21_HZ:
        bandwidthText = "21 Hz";
        break;
    case MPU6050_BAND_10_HZ:
        bandwidthText = "10 Hz";
        break;
    case MPU6050_BAND_5_HZ:
        bandwidthText = "5 Hz";
        break;
    }
    mus4Logf("mpu6050", "Filter bandwidth set to: %s", bandwidthText);
    mus4LogLine("mpu6050", "Setup complete, ready for data acquisition");
}

// End of MPU6050 functions

#ifdef ENABLE_GAMEPAD_MODE
void sendGamepadPacket() {
    if (bleGamepad.isConnected()) {
        // Map RC Channels (approx 1000-2000) to Gamepad Axes (-32767 to 32767)
        int lx = map(constrain(pwm_value[CH_STEERING], 1000, 2000), 1000, 2000, 0, 32767);
        int ly = map(constrain(pwm_value[CH_THROTTLE], 1300, 1800), 1300, 1800, 32767, 0);
        // int rx = map(constrain(pwm_value[CH_PARK], 1000, 2000), 1000, 2000, -32767, 32767);
        // int ry = map(constrain(pwm_value[CH_MODE], 1000, 2000), 1000, 2000, -32767, 32767);
        int rx = 0;
        int ry = 0;

        bleGamepad.setLeftThumb(0, ly);
        bleGamepad.setRightThumb(lx, 0);
    }
}
#endif

// --- Steering Signal Processing Logic ---

void reset_steering_filter() {
    for (int i = 0; i < MA_WINDOW_SIZE; i++) {
        steering_history[i] = 1488;
    }
    steering_index = 0;
    last_valid_steering_pwm = 1488;
    steering_error_count = 0;
    valid_signal_count = 0;
    safe_mode_active = false;
    is_history_initialized = true;
    
    // Reset PID State
    pid_state.integral = 0;
    pid_state.prev_error = 0;
    pid_state.current_smooth_output = 0;
}

int process_steering_signal(int raw_pwm) {
    // 0. Initialize history if needed
    if (!is_history_initialized) {
        reset_steering_filter();
    }

    // 1. Input Validation (Data Acquisition Layer)
    int current_pwm = raw_pwm;
    bool is_signal_valid = true;
    
    // Check range
    if (raw_pwm < PWM_VALID_MIN || raw_pwm > PWM_VALID_MAX) {
        // Invalid signal: use last valid value
        current_pwm = last_valid_steering_pwm;
        is_signal_valid = false;
    } 
    // Check slew rate (spike detection)
    // Reject if change > 800us in single frame (impossible for human input)
    // unless it persists (handled by consecutive valid checks, but for now simple rejection)
    else if (abs(raw_pwm - last_valid_steering_pwm) > 800) {
        // Treat as noise spike
        current_pwm = last_valid_steering_pwm;
        is_signal_valid = false; 
        // Serial.println("Warn: Steering Signal Spike Detected!");
    }
    else {
        last_valid_steering_pwm = current_pwm;
    }

    // 2. Smoothing (Moving Average) - Pre-filter
    steering_history[steering_index] = current_pwm;
    steering_index = (steering_index + 1) % MA_WINDOW_SIZE;

    long sum = 0;
    for (int i = 0; i < MA_WINDOW_SIZE; i++) {
        sum += steering_history[i];
    }
    int filtered_pwm = sum / MA_WINDOW_SIZE;

    // 3. Mapping to Control Range (-100 to 100)
    // Target steering based on filtered PWM
    int16_t cal_mid = steer_cal_enabled ? steer_cal.mid_pwm : RC_STEERING_MID;
    int16_t cal_min = steer_cal_enabled ? steer_cal.min_pwm : RC_STEERING_MIN;
    int16_t cal_max = steer_cal_enabled ? steer_cal.max_pwm : RC_STEERING_MAX;
    float target_steering;
    if (filtered_pwm < cal_mid) {
        target_steering = map(filtered_pwm - cal_mid, cal_min - cal_mid, 0, -100, 0);
    } else {
        target_steering = map(filtered_pwm - cal_mid, 0, cal_max - cal_mid, 0, 100);
    }

    // 4. PID Calculation
    float error = target_steering - pid_state.current_smooth_output;
    
    // Deadband check
    if (abs(error) < pid_config.deadband) {
        error = 0;
    }
    
    // Integral term
    pid_state.integral += error;
    pid_state.integral = constrain(pid_state.integral, -pid_config.integral_limit, pid_config.integral_limit);
    
    // Derivative term
    float derivative = error - pid_state.prev_error;
    
    // Calculate output change
    float output_change = (pid_config.Kp * error) + (pid_config.Ki * pid_state.integral) + (pid_config.Kd * derivative);
    
    // Update state
    pid_state.prev_error = error;
    pid_state.current_smooth_output += output_change;
    
    // 5. Post-Clamping
    int final_steering = constrain((int)pid_state.current_smooth_output, -100, 100);

    // 6. Fault Detection & Safety Mode Logic
    // Condition A: Sensor out of range (checked in step 1) or excessive value
    // Note: Since we clamp final_steering, we check the mapped target or raw signal validity
    
    if (!is_signal_valid || abs(target_steering) > 120) { // Allow some margin over 100 before error
        steering_error_count++;
        valid_signal_count = 0; // Reset recovery counter
        
        if (steering_error_count >= MAX_ERROR_COUNT) {
            if (!safe_mode_active) {
                safe_mode_active = true;
                mus4LogLine("steering", "ALARM: Steering Sensor Fault! Safe Mode Activated.");
            }
        }
    } else {
        // Signal is valid
        steering_error_count = 0; // Reset error counter
        
        if (safe_mode_active) {
            // Recovery logic
            valid_signal_count++;
            if (valid_signal_count > 50) { // Approx 1 second @ 50Hz (assuming loop speed)
                safe_mode_active = false;
                valid_signal_count = 0;
                mus4LogLine("steering", "INFO: Steering Signal Recovered. Exiting Safe Mode.");
                
                // Soft reset PID output to current target to avoid jump
                pid_state.current_smooth_output = target_steering;
            }
        }
    }

    // Override if safe mode
    if (safe_mode_active) {
        final_steering = 0; // Center steering
        pid_state.current_smooth_output = 0; // Reset PID output
        pid_state.integral = 0; // Reset integral
    }

    return final_steering;
}

// --- Drift Assist Logic ---
// Input: driver_steering - raw driver steering input (-100~100)
// Output: final steering value after adding drift compensation (-100~100)
int apply_drift_assist(int driver_steering) {
#if DRIFT_ASSIST_ENABLED
    // Only active in manual mode when Drift Assist is enabled
    if (car_output.mode != CAR_MODE_MANUAL || !drift_assist_enabled) {
        drift_assist_active = false;
        drift_compensation = 0.0f;
        return driver_steering;
    }

    // 1. Apply a first-order low-pass filter to gyroZ to remove sensor noise
    gyro_z_filtered = gyro_z_filtered * (1.0f - DRIFT_ASSIST_SMOOTH) +
                      mpu6050Data.gyroZ * DRIFT_ASSIST_SMOOTH;

    // 2. Determine whether drift is triggered
    float abs_gyro = fabs(gyro_z_filtered);
    if (abs_gyro > DRIFT_ASSIST_THRESHOLD) {
        // 3. Calculate counter-steer compensation. A negative sign would invert direction
        // (clockwise slide -> negative gyro -> positive compensation -> steer right?),
        // but the physical direction must be aligned first.
        // User definition: clockwise rear slide gives negative gyroZ -> counter-steer left is needed (<1439 -> negative value).
        // Therefore: negative gyroZ -> negative compensation -> counter-steer left.
        //            positive gyroZ -> positive compensation -> counter-steer right.
        // So should compensation have the same sign as gyroZ? Re-check carefully:
        // Clockwise rear slide (rear swings right) -> vehicle over-rotates right -> needs left counter-steer (steering value decreases / becomes negative).
        // User definition: clockwise rear slide gives negative gyroZ.
        // Therefore: negative gyroZ -> negative compensation -> counter-steer left.
        // Conclusion: compensation = gyroZ * GAIN (same sign).
        // Re-checking the user definitions:
        // "Value is negative when the rear slides clockwise, positive when it slides counterclockwise."
        // "At the RC transmitter, signal below 1439 steers wheels left, above 1439 steers wheels right."
        // Mapped to -100~100: -100 left, +100 right.
        // Clockwise rear slide (rear swings right / oversteers right) -> counter-steer left -> add a negative steering value.
        // At this moment gyroZ is negative -> compensation should also be negative.
        // Therefore compensation = gyroZ * GAIN.
        // Implement this logic first; adjust the sign during real-vehicle tuning if needed.
        float raw_comp = gyro_z_filtered * DRIFT_ASSIST_GAIN * drift_assist_scale;

        // 4. Clamp compensation
        float effectiveMaxComp = min(DRIFT_ASSIST_MAX_COMP * drift_assist_scale, 100.0f);
        raw_comp = constrain(raw_comp, -effectiveMaxComp, effectiveMaxComp);

        // 5. Smooth compensation output
        drift_compensation = drift_compensation * (1.0f - DRIFT_ASSIST_SMOOTH) +
                             raw_comp * DRIFT_ASSIST_SMOOTH;

        drift_assist_active = true;
    } else {
        // Below threshold; gradually decay compensation to 0
        drift_compensation *= DRIFT_ASSIST_DECAY;
        if (fabs(drift_compensation) < 0.5f) {
            drift_compensation = 0.0f;
            drift_assist_active = false;
        } else {
            drift_assist_active = true;
        }
    }

    // 6. Add compensation to the raw driver input and clamp the result
    int final_steering = driver_steering + (int)drift_compensation;
    final_steering = constrain(final_steering, -100, 100);

    return final_steering;
#else
    drift_assist_active = false;
    drift_compensation = 0.0f;
    return driver_steering;
#endif
}

#ifdef ENABLE_BOOT_STEERING_SELF_TEST
void run_steering_tests() {
    mus4LogLine("test", "--- Starting Steering Signal Processing Unit Tests (PID Enabled) ---");
    
    // Test 1: Normal Value (PID Convergence)
    reset_steering_filter();
    int res = 0;
    // Simulate convergence
    for(int i=0; i<20; i++) {
        res = process_steering_signal(1488);
    }
    mus4Logf("test", "Test 1 (Normal 1488 -> 0): Output=%d, Pass=%d", res, res == 0);

    // Test 2: Boundary Values
    reset_steering_filter();
    // Fill buffer to avoid smoothing delay effect for test
    for(int i=0; i<10; i++) process_steering_signal(872); 
    // Run PID loop to converge
    for(int i=0; i<20; i++) res = process_steering_signal(872);
    mus4Logf("test", "Test 2A (Min 872 -> -100): Output=%d, Pass=%d", res, res == -100);

    reset_steering_filter();
    for(int i=0; i<10; i++) process_steering_signal(2113);
    for(int i=0; i<20; i++) res = process_steering_signal(2113);
    mus4Logf("test", "Test 2B (Max 2113 -> 100): Output=%d, Pass=%d", res, res == 100);

    // Test 3: Noise Injection (Should be ignored or dampened)
    reset_steering_filter();
    // Converge to center
    for(int i=0; i<20; i++) process_steering_signal(1488); 
    
    // Inject single frame noise (0 is invalid PWM, so it uses last valid 1488)
    int noise_res = process_steering_signal(0); 
    mus4Logf("test", "Test 3 (Invalid Input 0 -> Hold Last): Output=%d, Pass=%d", noise_res, noise_res == 0);

    // Test 4: Hard Clamping
    reset_steering_filter();
    // Inject value that maps to > 100 but is valid PWM (e.g. 2200)
    for(int i=0; i<30; i++) res = process_steering_signal(2200);
    mus4Logf("test", "Test 4 (Clamp 2200 -> 100): Output=%d, Pass=%d", res, res == 100);

    // Test 5: Safety Mode Activation
    reset_steering_filter();
    // Trigger error. 
    // Since we have a 10-point moving average, we need enough samples for the average to cross the threshold.
    // Target threshold > 120 corresponds to filtered_pwm > approx 2237.
    // Input 2300.
    for(int i=0; i<15; i++) {
        process_steering_signal(2300);
    }
    mus4Logf("test", "Test 5 (Safety Mode Activation): Active=%d, Pass=%d", safe_mode_active, safe_mode_active == true);

    // Test 6: Safety Mode Recovery
    // Continue from Test 5, safe_mode_active is true.
    // Feed valid signals. We need > 50 valid signals.
    for(int i=0; i<50; i++) {
        process_steering_signal(1488);
    }
    // Should still be active (count = 50)
    bool still_active = safe_mode_active;
    
    // One more
    process_steering_signal(1488);
    bool recovered = !safe_mode_active;
    
    mus4Logf("test", "Test 6 (Safety Mode Recovery): Still Active at 50=%d, Recovered at 51=%d, Pass=%d",
                  still_active, recovered, still_active && recovered);

    mus4LogLine("test", "--- End Tests ---");
    reset_steering_filter(); // Reset for actual operation
}
#endif

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
