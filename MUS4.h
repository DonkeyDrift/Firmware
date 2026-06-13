#pragma once

// MUS4 firmware public API and project configuration.
//
// This single project header keeps the Arduino sketch approachable:
// - external Arduino/ESP32/library includes live at the top;
// - configuration and shared types are defined before module APIs;
// - each feature area is separated by a short, searchable section banner.

// ---- Build Feature Switches ------------------------------------------------
#define ENABLE_WIFI_CONSOLE
#ifdef ENABLE_WIFI_CONSOLE
#define ENABLE_WIFI_WEBSOCKET_TELEMETRY
#endif
// #define ENABLE_DIAGNOSTIC_COMMANDS
// #define ENABLE_BOOT_STEERING_SELF_TEST

#ifndef ENABLE_WIFI_CONSOLE
#define ENABLE_GAMEPAD_MODE
#endif

#ifdef ENABLE_WIFI_CONSOLE
#define ENABLE_WIFI_NETBIOS_DISCOVERY
#define ENABLE_WIFI_LLMNR_DISCOVERY
#endif

// ---- Arduino, ESP32, and Third-party Libraries ----------------------------
#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_INA219.h>
#include <driver/mcpwm_cap.h>
#include <stdarg.h>
#include <string.h>

#ifdef ENABLE_WIFI_CONSOLE
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <Preferences.h>
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#endif
#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
#include <NetBIOS.h>
#endif
#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
#include <WiFiUdp.h>
#endif
#endif

// ---- Web UI Asset ----------------------------------------------------------
// Kept separate because it is a generated PROGMEM HTML/CSS/JS blob, not API.
#ifdef ENABLE_WIFI_CONSOLE
#include "WebConsoleAssets.h"
#endif


// ============================================================================
// Build Flags, Pins, Timing, and Calibration
// ============================================================================
#define MUS4_LOG_TARGET_SERIAL 0
#define MUS4_LOG_TARGET_WEB 1
#ifndef MUS4_LOG_TARGET
#ifdef ENABLE_WIFI_CONSOLE
#define MUS4_LOG_TARGET MUS4_LOG_TARGET_WEB
#else
#define MUS4_LOG_TARGET MUS4_LOG_TARGET_SERIAL
#endif
#endif

// RC Receiver Calibration Defaults (PWM pulse width in microseconds)
#define RC_THROTTLE_MIN 888
#define RC_THROTTLE_MID 1493
#define RC_THROTTLE_MAX 2149
#define RC_STEERING_MIN 872
#define RC_STEERING_MID 1488
#define RC_STEERING_MAX 2113

#define BUZZER_PIN 2

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

#define RC_SIGNAL_TIMEOUT 1000000UL  // RC signal timeout (µs)
#define RC_PWM_MIN 800   // Minimum valid PWM (µs)
#define RC_PWM_MAX 2200  // Maximum valid PWM (µs)
#define ENABLE_RC_MCPWM_CAPTURE 0
#define RC_MCPWM_CAPTURE_RESOLUTION_HZ 1000000
#define RC_MCPWM_CAPTURE_GROUP_ID 0

#define PWM_FILTER_SIZE 5  // Sliding-window median filter size (5-7)

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
#define SENSOR_UPDATE_INTERVAL 2     // Sensor data update interval (ms) - ~500Hz
#define RC_DATA_UPDATE_INTERVAL 2    // RC data update interval (ms) - ~500Hz
#define RC_FILTER_UPDATE_INTERVAL 2   // RC filter update interval (ms) - ~500Hz, balances response and stability
#define UI_UPDATE_INTERVAL 2         // UI update interval (ms) - smooth 500Hz experience

// Waveform parameters
#ifndef WAVE_WIDTH
#define WAVE_WIDTH 20                 // Waveform width (reduced for performance)
#endif
#ifndef WAVE_HEIGHT
#define WAVE_HEIGHT 6                 // Waveform height (reduced for performance)
#endif

// ============================================================================
// Firmware Identity
// ============================================================================
#define MUS4_FIRMWARE_NAME "MUS4"
#define MUS4_FIRMWARE_VERSION "v1.7.6-Serial"
#define MUS4_BUILD_DATE __DATE__
#define MUS4_BUILD_TIME __TIME__
#define MUS4_BUILD_INFO MUS4_FIRMWARE_NAME " " MUS4_FIRMWARE_VERSION " " MUS4_BUILD_DATE " " MUS4_BUILD_TIME

// ============================================================================
// Wi-Fi Console Constants and Web Data Types
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE
// Use static linkage to avoid ODR violations when this header is included by
// multiple translation units.
static const char* WIFI_CONSOLE_AP_DEFAULT_SSID = "MUS4-ESP";
static const char* WIFI_CONSOLE_AP_PASSWORD = "mus4-debug";
static const char* WIFI_AP_SSID_SUFFIX = "-ESP";
static const uint8_t WIFI_AP_SSID_PREFIX_MAX_LEN = 6;
static const uint16_t WIFI_CONSOLE_PORT = 2323;
static const uint16_t WIFI_WEB_CONSOLE_PORT = 80;
static const uint32_t WIFI_WEB_TELEMETRY_MIN_FREE_HEAP = 60000;
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
static const uint16_t WIFI_WEB_SOCKET_PORT = 81;
static const unsigned long WIFI_WEB_SOCKET_PUSH_INTERVAL_MS = 16;
static const uint8_t WIFI_WEB_SOCKET_MAX_POINTS_PER_FRAME = 8;
static const uint8_t WIFI_WEB_SOCKET_MAX_REPLAY_POINTS = 32;
static const uint16_t WIFI_WEB_SOCKET_KEEPALIVE_SECONDS = 60;
#endif
static const uint8_t WIFI_CONSOLE_CHANNEL = 6;
static const uint8_t WIFI_CONSOLE_MAX_CLIENTS = 1;
static const unsigned long WIFI_CONSOLE_RETRY_INTERVAL_MS = 5000;
static const unsigned long WIFI_STA_CONNECT_TIMEOUT_MS = 15000;
static const unsigned long WIFI_STA_APPLY_DELAY_MS = 800;
static const char* WIFI_OTA_HOSTNAME = "mus4-ota";
static const char* WIFI_OTA_PASSWORD = "mus4-debug";
static const uint16_t WIFI_OTA_PORT = 3232;
static const unsigned long WIFI_OTA_WINDOW_MS = 120000UL;
static const char* MUS4_PREF_NAMESPACE = "mus4";
static const char* MUS4_PREF_DEV_MODE_KEY = "dev_mode";
static const char* MUS4_PREF_AP_SSID_KEY = "ap_ssid";
static const char* MUS4_PREF_STA_ENABLED_KEY = "sta_en";
static const char* MUS4_PREF_STA_SSID_KEY = "sta_ssid";
static const char* MUS4_PREF_STA_PASSWORD_KEY = "sta_pass";
static const char* MUS4_PREF_STEER_MIN_KEY = "str_min";
static const char* MUS4_PREF_STEER_MID_KEY = "str_mid";
static const char* MUS4_PREF_STEER_MAX_KEY = "str_max";
static const char* MUS4_PREF_STEER_CAL_EN_KEY = "str_cal";
static const uint8_t WIFI_AP_SSID_MAX_LEN = 32;
static const uint8_t WIFI_STA_SSID_MAX_LEN = 32;
static const uint8_t WIFI_STA_PASSWORD_MAX_LEN = 63;
static const uint8_t WIFI_STA_PASSWORD_MIN_LEN = 8;
static const uint8_t WIFI_WEB_LOG_CAPACITY = 64;
static const uint8_t SERIAL1_WEB_LOG_CAPACITY = 64;
static const uint16_t WIFI_WEB_DATA_CAPACITY = 256;
static const unsigned long WIFI_WEB_DATA_INTERVAL_MS = 16;

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
#endif

// ============================================================================
// Shared Runtime Data Types
// ============================================================================
// Sensor Data Structure
struct SensorData {
    float busVoltage;
    float shuntVoltage;
    float loadVoltage;
    float current_mA;
    float power_mW;
    float accelX, accelY, accelZ;
    float gyroX, gyroY, gyroZ;
    float temperature;
    unsigned long lastReadTime;
    unsigned long readCount;
    bool valid;
};

// Control Data Structure (matching struct_message in mus4.ino logic)
struct ControlData {
    int throttle;
    int steering;
    int mode;
    bool park;
};

// ============================================================================
// Wi-Fi and OTA Runtime State
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE

// Aggregated Wi-Fi runtime state to replace scattered extern bool/char variables
// in MUS4_FW.ino. This structure is owned by the main sketch and passed by
// reference into the wireless/STA/OTA modules.
struct WifiRuntimeState {
    // Console authentication
    bool consoleStarted = false;
    bool consoleAuthenticated = false;

    // STA configuration
    bool staConfigured = false;
    bool staConnected = false;
    bool staTimedOut = false;
    bool staConnecting = false;
    char staLastError[24] = {0};
    char staLastErrorMessage[128] = {0};
    bool staApplyPending = false;
    bool staPasswordSet = false;
    char staSsid[WIFI_STA_SSID_MAX_LEN + 1] = {0};
    char staPassword[WIFI_STA_PASSWORD_MAX_LEN + 1] = {0};

    // AP / mDNS / handoff
    bool apRestartPending = false;
    bool mdnsStarted = false;
    bool staHandoffActive = false;
    char staHandoffTargetSsid[WIFI_STA_SSID_MAX_LEN + 1] = {0};
    char staHandoffStaIp[16] = {0};
    char staHandoffApSsid[WIFI_AP_SSID_MAX_LEN + 1] = {0};
    unsigned long staHandoffStartedMs = 0;
    char apSsid[WIFI_AP_SSID_MAX_LEN + 1] = {0};

    // Dev mode
    bool devModeEnabled = false;

    // Timing
    unsigned long staConnectStartMs = 0;
    unsigned long staApplyDeadlineMs = 0;
    unsigned long apRestartDeadlineMs = 0;
    unsigned long lastConsoleStartAttemptMs = 0;

    // Shared Preferences instance (pointer to the global Preferences in MUS4_FW.ino)
    Preferences* prefs = nullptr;
};

// Aggregated OTA runtime state to replace scattered extern OTA variables.
struct OtaRuntimeState {
    bool started = false;
    bool windowOpen = false;
    bool inProgress = false;
    bool parkGuardActive = false;
    unsigned long deadlineMs = 0;
    uint8_t lastProgressPct = 0;
};

#endif // ENABLE_WIFI_CONSOLE

// ============================================================================
// Line Buffer Types
// ============================================================================
struct SerialBuf { char buf[256]; uint16_t len; uint32_t frames; uint32_t errors; bool overflow; };

// ============================================================================
// String-backed Print Adapter
// ============================================================================
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

// ============================================================================
// Buzzer State Machine API
// ============================================================================
#define BUZZER_MODE_MANUAL 0
#define BUZZER_MODE_SEMI_AUTO 1
#define BUZZER_MODE_FULL_AUTO 2
#define BUZZER_PARK_LOCK 3
#define BUZZER_PARK_UNLOCK 4

#define BUZZER_VOLUME 40
#define BUZZER_SOUND_ENABLED 0

// 音符定义
#define NOTE_REST 0
#define NOTE_C4 262
#define NOTE_E4 330
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_D5 587

// 音符时值定义
#define N8 8
#define N4 4

struct BuzzerNote {
    int pitch;
    int duration;
};

class Buzzer {
private:
    bool _playing = false;
    int _pin;
    int _channel;
    int _volume;
    static int _channelCounter;
    
    void playNoteWithVolume(int pitch, int durationMs);
    void playMelody(const BuzzerNote* melody, int length);
    
public:
    Buzzer(int pin);
    void playModeSound(int mode);
    void playParkLockSound();
    void playParkUnlockSound();
    bool isPlaying() { return _playing; }
    void update();
    void setVolume(int volume);
};

// ============================================================================
// Terminal UI API
// ============================================================================
class TUI {
public:
    TUI(Print& out);
    void update(unsigned long currentTime);
    void render();
    void setRC(int ch1, int ch2, int ch3, int ch4, int ch5, int ch6);
    void setOutput(int throttle, int steering, int mode, bool park);
    void setSensors(const SensorData& data);
    
    // Configuration
    void setRefreshRate(unsigned long ms);
    void setAnsiEnabled(bool enabled);
    void setWaveformEnabled(bool enabled);
    void forceRedraw();
    unsigned long getLastRenderDuration() const { return _lastRenderDuration; }
    void log(const char* format, ...);

private:
    Print& _out;
    unsigned long _lastUpdate;
    unsigned long _refreshRate;
    unsigned long _lastRenderDuration;
    bool _forceRedraw;
    bool _ansiEnabled;
    bool _waveformEnabled;
    bool _initialized;
    bool _outputStateInitialized;
    char _logBuffer[64];
    unsigned long _logTime;

    // Current State
    struct State {
        int ch1, ch2, ch3, ch4, ch5, ch6;
        ControlData output;
        SensorData sensors;
        int throttleWave[WAVE_WIDTH];
        int steeringWave[WAVE_WIDTH];
    } _state;

    // Previous State for Dirty Checking
    struct State _lastState;
    unsigned long _lastWaveUpdate;

    // Helper methods
    void drawHeader();
    void drawMode();
    void drawPark();
    void drawRC();
    void drawOutput();
    void drawWaveforms();
    void drawSensors();
    void drawLog();
    void cursorTo(int row, int col);
    void updateWaveformData();
};

// ============================================================================
// Logging API
// ============================================================================
typedef void (*Mus4LogSink)(const char* source, const String& line);

extern uint8_t mus4LogTarget;

void mus4SetWebLogSink(Mus4LogSink sink);
void setMus4LogTargetWeb();
void mus4LogLine(const char* source, const String& line);
void mus4Logf(const char* source, const char* fmt, ...);

// ============================================================================
// JSON Helpers
// ============================================================================
void appendJsonString(String& out, const char* text);

// ============================================================================
// I2C Bus Helpers
// ============================================================================
bool I2CRead(uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t *Data);
uint16_t I2CReadValue(uint8_t addr, uint8_t reg);
void I2CWriteValue(uint8_t Address, uint8_t Register, uint16_t Data);
const char *identifyI2CDeviceByAddress(uint8_t address);
bool I2CReadRegister8(uint8_t address, uint8_t reg, uint8_t *value);
bool probeMPU6050AtAddress(uint8_t address, uint8_t *whoAmI);

// ============================================================================
// Status LED API
// ============================================================================
void setLEDColor(CRGB targetColor);
void setLEDToggle(CRGB color1, CRGB color2);
void scanLEDToggle();

// ============================================================================
// I2C Sensor API
// ============================================================================
void printLastI2CScanSummary();
void read_ina219();
void setup_ina219();
void read_mpu6050();
void scanI2CBus();
bool tryInitMPU6050OnCurrentBus(uint8_t *activeAddress, int maxRetriesPerAddress);
void setup_mpu6050();

// ============================================================================
// BLE Gamepad API
// ============================================================================
void sendGamepadPacket();

// ============================================================================
// RC Filtering API
// ============================================================================
uint16_t medianFilter(uint16_t* buf, int size);
bool isAuxiliaryRcChannel(int ch);
bool isPrimaryRcChannel(int ch);
uint16_t smoothPrimaryPWM(int ch, uint16_t value, bool valid);
uint16_t stabilizeAuxiliaryPWM(int ch, uint16_t value, bool valid);

#ifdef ENABLE_DIAGNOSTIC_COMMANDS
bool runFilterTests();
#endif

// ============================================================================
// RC PWM Capture API
// ============================================================================
// Raw PWM values updated by ISRs (volatile for interrupt safety)
extern volatile uint16_t pwm_value[RC_CHANNEL_COUNT];
// Timestamp of last valid pulse per channel (microseconds)
extern volatile unsigned long last_valid_time[RC_CHANNEL_COUNT];

// Initialize RC receiver pins, attach interrupts, and optionally set up MCPWM capture.
void setupRcPwmCapture();

// ============================================================================
// Control Mixer API
// ============================================================================
// Update driving mode according to the RC mode channel value.
void mode_change(bool modeValid);

// Compute final throttle/steering output based on mode, park state, RC/Pilot inputs.
void updateControlOutput();

// ============================================================================
// Park and Emergency Stop API
// ============================================================================
enum EmergencyStopState
{
    EST_IDLE,
    EST_READY,
    EST_BRAKING,
    EST_DONE
};

extern EmergencyStopState emergencyStopState;

// Emergency stop state machine (updates car_output.throttle)
void emergencyStop();

// Park button state machine (updates rc_data.park / car_output.park)
void park_change();

// ============================================================================
// Actuator Output API
// ============================================================================
// Servo midpoint and range (extern for Diagnostics.cpp linkage)
extern const int SERVO_MID_V;
extern const int SERVO_RANGE_V;

// Initialize PWM output channels (ledcAttachChannel)
void setupActuatorOutput();

// Write current car_output values to servo/ESC PWM channels
void updateActuatorOutput();

// ============================================================================
// Drift Assist API
// ============================================================================
extern bool drift_assist_enabled;
extern bool drift_assist_active;
extern float drift_compensation;
extern float gyro_z_filtered;
extern float drift_assist_scale;

void update_drift_assist_control(bool driftValid, bool driftScaleValid);
int apply_drift_assist(int driver_steering);

// ============================================================================
// Steering Control API
// ============================================================================
struct PIDConfig {
    float Kp = 0.8;
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

extern PIDConfig pid_config;
extern PIDState pid_state;
extern bool safe_mode_active;

void reset_steering_filter();
int process_steering_signal(int raw_pwm);

#ifdef ENABLE_BOOT_STEERING_SELF_TEST
void run_steering_tests();
#endif

// ============================================================================
// Steering Calibration API
// ============================================================================
void setSteeringCalibrationRuntimeState(WifiRuntimeState& ws);

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

extern SteeringCalibration steer_cal;
extern bool steer_cal_enabled;
extern SteerCalState steer_cal_state;
extern unsigned long steer_cal_stage_start_ms;
extern int16_t steer_cal_temp_min;
extern int16_t steer_cal_temp_max;

void loadSteeringCalibration();
bool saveSteeringCalibration();
void resetSteeringCalibration();
int mapSteeringCalibrated(int16_t pwm);
void printCalStatus(Print& out);
bool startSteerCalibration(Print& out);
void updateSteerCalibration();

// ============================================================================
// Diagnostics API
// ============================================================================
void notifyDegrade();
void evalDegrade();

#ifdef ENABLE_DIAGNOSTIC_COMMANDS
bool runBenchmarks();
bool runRegression();
bool runStress();
#endif

// ============================================================================
// Pilot Command Parser API
// ============================================================================
uint8_t parseHex2(const char* s);
uint8_t calcChecksum(const char* s, int n);
bool parsePilotCommandLine(const String& line, int* throttle, int* steering, int* seq);
bool parseAndValidateCommand(String cmd, int* throttle, int* steering);

#ifdef ENABLE_DIAGNOSTIC_COMMANDS
bool runUnitTests();
#endif

// ============================================================================
// Command Dispatcher API
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE
void setCommandDispatcherRuntimeStates(OtaRuntimeState& os, WifiRuntimeState& ws);
#endif

bool dispatchCommandLine(const String& line, Print& out, SerialBuf& sb);

// ============================================================================
// Local Serial Command API
// ============================================================================
bool processLine(const String& line, int* throttle, int* steering, int* seq);

// ============================================================================
// Serial Line Reader API
// ============================================================================
void readSerialBuf(HardwareSerial& ser, SerialBuf& sb);

// ============================================================================
// Wireless Command Policy API
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE

enum WirelessCommandOrigin { WIRELESS_ORIGIN_TCP, WIRELESS_ORIGIN_WEB };

void processWirelessConsoleLine(const String& line, Print& out, WirelessCommandOrigin origin);
void printWirelessStatus(Print& out);

bool isWirelessControlCommand(const String& line);
bool isWirelessOtaOpenCommand(const String& line);
bool isLocalOtaOpenCommand(const String& line);
bool isWirelessOtaStatusCommand(const String& line);
bool isWirelessOtaCloseCommand(const String& line);
bool isWifiStaConfigCommand(const String& line);
bool isParkLockedWirelessCommand(const String& line);
bool isWirelessCommandAllowed(const String& line, WirelessCommandOrigin origin, WifiRuntimeState& ws);
String redactWirelessConsoleLine(const String& line);
#endif

// ============================================================================
// Wi-Fi Identity API
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE

void setWifiIdentityRuntimeState(WifiRuntimeState& ws);

bool isMdnsSafeHostnameChar(char c);
bool isMdnsSafeHostname(const String& value);
bool isValidApSsidPrefix(const String& value);
bool copyWifiApSsid(const String& ssid);
String wifiMdnsHostText();
String wifiMdnsUrlText();
#endif

// ============================================================================
// Wi-Fi STA Configuration API
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE

void setWifiRuntimeState(WifiRuntimeState& ws);

bool copyWifiStaSsid(const String& ssid);
bool copyWifiStaPassword(const String& password);
String wifiStaIpText();
void clearWifiStaLastError();
void setWifiStaLastError(const char* code, const char* message, bool timedOut);
void scheduleWifiStaApply();
bool saveWifiStaPreference(const String& ssid, const String& password);
bool saveWifiStaSsidPreference(const String& ssid);
bool saveWifiStaPasswordPreference(const String& password);
void clearWifiStaRuntimeStateWithoutDisconnect();
bool clearWifiStaPreference();
void loadWifiStaPreference();
void printWifiStaStatus(Print& out);
bool processWifiStaConfigCommand(const String& line, Print& out, WifiRuntimeState& ws);
#else
inline bool processWifiStaConfigCommand(const String& line, Print& out)
{
    (void)line;
    (void)out;
    return false;
}
#endif

// ============================================================================
// OTA API
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE
unsigned long wifiOtaTtlMs(OtaRuntimeState& os, WifiRuntimeState& ws);
void printWifiOtaStatus(Print& out, OtaRuntimeState& os, WifiRuntimeState& ws);
void closeWifiOtaWindow(const char* reason, OtaRuntimeState& os);
void forceWifiOtaParkLocked();
void keepDevModeOtaWindowActive(OtaRuntimeState& os, WifiRuntimeState& ws);
bool shouldEmitSerial1Telemetry(OtaRuntimeState& os);
void openLocalWifiOtaWindow(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os);
bool processLocalOtaMaintenanceCommand(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os, WifiRuntimeState& ws);
void openWifiOtaWindow(Print& out, WirelessCommandOrigin origin, OtaRuntimeState& os, WifiRuntimeState& ws);
void updateWifiOta(OtaRuntimeState& os, WifiRuntimeState& ws);
#else
inline unsigned long wifiOtaTtlMs(OtaRuntimeState& os, WifiRuntimeState& ws) { (void)os; (void)ws; return 0; }
inline void printWifiOtaStatus(Print& out, OtaRuntimeState& os, WifiRuntimeState& ws) { (void)out; (void)os; (void)ws; }
inline void closeWifiOtaWindow(const char* reason, OtaRuntimeState& os) { (void)reason; (void)os; }
inline void forceWifiOtaParkLocked() {}
inline void keepDevModeOtaWindowActive(OtaRuntimeState& os, WifiRuntimeState& ws) { (void)os; (void)ws; }
inline bool shouldEmitSerial1Telemetry(OtaRuntimeState& os) { (void)os; return true; }
inline void openLocalWifiOtaWindow(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os) { (void)line; (void)out; (void)sb; (void)os; }
inline bool processLocalOtaMaintenanceCommand(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os, WifiRuntimeState& ws) { (void)line; (void)out; (void)sb; (void)os; (void)ws; return false; }
inline void openWifiOtaWindow(Print& out, WirelessCommandOrigin origin, OtaRuntimeState& os, WifiRuntimeState& ws) { (void)out; (void)origin; (void)os; (void)ws; }
inline void updateWifiOta(OtaRuntimeState& os, WifiRuntimeState& ws) { (void)os; (void)ws; }
#endif

// ============================================================================
// Web Log Buffer API
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE

// Optional real-time sink for log entries.  Set by the WebSocket module to
// push logs to the browser as they are appended.
typedef void (*WebLogSocketSink)(uint32_t seq, unsigned long t, const char* source, const char* line);

// Initialize the web log ring buffer. Call once before any append/read.
void webLogBufferInit();

// Register a sink that receives every appended log line in real time.
void webLogBufferSetSocketSink(WebLogSocketSink sink);

// Append a single log line to the web log ring buffer.
void appendWebLog(const char* source, const String& line);

// Split a multi-line text and append each non-empty line to the buffer.
void appendWebLogLines(const char* source, const String& text);

// Return the number of dropped entries due to buffer overflow.
uint32_t webLogBufferDropped();

// Append a JSON representation of the log buffer (entries newer than `since`)
// to the provided String. The output includes a "dropped" counter and an
// "entries" array.
void writeWebLogsJson(String& response, uint32_t since);

#endif

// ============================================================================
// WebSocket Telemetry API
// ============================================================================
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY


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

// ============================================================================
// Web Console HTTP API
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE

// Register all HTTP routes for the Web Console and start the WebServer.
void setupWebConsoleServer();

// Process one update tick for the Web Console: sample telemetry data,
// handle pending HTTP clients, and record handler timing statistics.
// Does NOT handle WebSocket (see WebTelemetry), AP restart, or DNS server.
void updateWebConsoleServer();

#endif

// ============================================================================
// Wi-Fi Runtime Manager API
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE


void loadDevModePreference();
bool saveDevModePreference(bool enabled);

void startWifiMdnsIfNeeded();
void stopWifiMdnsIfNeeded();

#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
void startWifiNetbiosIfNeeded();
void stopWifiNetbiosIfNeeded();
#endif

#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
void startWifiLlmnrIfNeeded();
void stopWifiLlmnrIfNeeded();
#endif

void clearWifiStaHandoff();
void finishWifiStaHandoff();
void startWifiStaHandoff(const String& targetSsid);
void disconnectWifiStaOnly();
void applyWifiStaCredentials();
void scheduleWifiApRestart();
bool ensureWifiApAvailable();
bool restartWifiAp();

void loadWifiApPreference();
bool saveWifiApPreference(const String& ssid);
String getActiveWifiApSsid();

void setupWifiConsole();
void updateWifiSta();
void updateWifiConsole();

void setupWifiWebConsole();
void updateWifiWebConsole();

#endif // ENABLE_WIFI_CONSOLE

