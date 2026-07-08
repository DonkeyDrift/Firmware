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
2. Serial1 上行协议（对齐上位机 DonkeyCar `ArdImu` / `Arduino` part，v1.7.9 起）：
  - MANUAL: T<t>S<s>\n          (~60Hz, 人工油门转向, 无冒号)
  - MANUAL: M<m>:P<p>\n         (状态变化时立即发 + 1Hz 心跳)
  - ALL:    $IMU,seq,ts_ms,ax,ay,az,gx,gy,gz\n  (~100Hz, m/s² + rad/s)
  Serial1 下行协议（保留）：<thr>:<str>[:seq][*CRC]\n
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
#include <esp_ota_ops.h>
#include "BuildInfo.h"
#include "SharedTypes.h"
#include "RcPwmCapture.h"
#include "TUI.h"
#include "WebConsoleAssets.h"
#include "StringPrint.h"
#include "JsonUtil.h"
#include "I2CBusTools.h"
#include "LedStatus.h"
#include "Mus4Log.h"
#include "JoystickCalibration.h"
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
#include "WifiOta.h"
#include "WebTelemetry.h"
#include "WifiManager.h"
#include "ControlMixer.h"
#include "SafetyState.h"
#include "ActuatorOutput.h"
#include "DriftAssist.h"
#include "SteeringControl.h"
#include "Diagnostics.h"
#ifdef ENABLE_AUTH_SERVICE
#include "AuthService.h"
#endif
#include "SerialBufferTypes.h"

#include "Buzzer.h"
// #include "test_runner.h"

TUI tui(Serial);
Buzzer buzzer(BUZZER_PIN);

#ifdef ENABLE_GAMEPAD_MODE
  #include <BleGamepad.h>
  BleGamepad bleGamepad("Gamepad MU02", "Espressif", 100);
#endif

Adafruit_MPU6050 mpu;
Adafruit_INA219 ina219;

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

CRGB leds[NUM_LEDS]; // Define the array of leds

// New global variables
unsigned long lastSensorUpdate = 0;
unsigned long lastRCDataUpdate = 0;
unsigned long lastRCFilterUpdate = 0;
unsigned long lastUIUpdate = 0;
unsigned long lastWaveUpdate = 0;     // Independent waveform refresh timer
const unsigned long WAVE_UPDATE_INTERVAL = 250; // 4Hz refresh rate
// Serial1 上行节流状态：$IMU 和 M:P 帧的发送时钟与去抖。
unsigned long lastImuEmitMs = 0;          // 上一次 $IMU 帧发送时间
unsigned long lastModeParkEmitMs = 0;     // 上一次 M:P 帧发送时间
unsigned long lastTelemWebLogMs = 0;      // 上一次 T..S.. 写入 Web 日志的时间（10Hz 节流）
int lastEmittedMode = -1;                 // -1 表示尚未发过；强制首次发
int lastEmittedPark = -1;                 // 同上
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
const unsigned long uiIntervalMax = 250;
unsigned long lastPerfEval = 0;
unsigned long lastUICycleDuration = 0;
unsigned long sensorTTL = 1000;
unsigned long rcTTL = 100;
unsigned long outputTTL = 100;
SerialBuf serial0Buf = {{0},0,0,0,false};
SerialBuf serial1Buf = {{0},0,0,0,false};
SerialBuf serial2Buf = {{0},0,0,0,false};
#ifdef ENABLE_WIFI_CONSOLE
#include "RuntimeState.h"
#include "WebLogBuffer.h"
#include "WebConsoleServer.h"
void ensureWifiOtaStarted();
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
SerialBuf wifiConsoleBuf = {{0},0,0,0,false};
WifiScanEntry wifiScanCache[16];
uint8_t wifiScanCacheCount = 0;
WebDataPoint wifiWebData[WIFI_WEB_DATA_CAPACITY];
// Aggregated runtime state passed by reference into wireless/STA/OTA modules.
// Existing code in this file continues to use the original names via
// reference/pointer aliases declared immediately below.
WifiRuntimeState wifiRuntime;
OtaRuntimeState otaRuntime;

// Wi-Fi runtime state aliases
bool& wifiConsoleStarted = wifiRuntime.consoleStarted;
bool& wifiConsoleAuthenticated = wifiRuntime.consoleAuthenticated;
bool& wifiStaConfigured = wifiRuntime.staConfigured;
bool& wifiStaConnected = wifiRuntime.staConnected;
bool& wifiStaTimedOut = wifiRuntime.staTimedOut;
bool& wifiStaConnecting = wifiRuntime.staConnecting;
extern char* const wifiStaLastError = wifiRuntime.staLastError;
extern char* const wifiStaLastErrorMessage = wifiRuntime.staLastErrorMessage;
bool& wifiStaApplyPending = wifiRuntime.staApplyPending;
bool& wifiStaApplyFromAp = wifiRuntime.staApplyFromAp;
uint8_t& wifiStaTargetChannel = wifiRuntime.staTargetChannel;
bool& wifiApRestartPending = wifiRuntime.apRestartPending;
bool& wifiMdnsStarted = wifiRuntime.mdnsStarted;
bool& wifiStaHandoffActive = wifiRuntime.staHandoffActive;
extern char* const wifiStaHandoffTargetSsid = wifiRuntime.staHandoffTargetSsid;
extern char* const wifiStaHandoffStaIp = wifiRuntime.staHandoffStaIp;
extern char* const wifiStaHandoffApSsid = wifiRuntime.staHandoffApSsid;
unsigned long& wifiStaHandoffStartedMs = wifiRuntime.staHandoffStartedMs;
bool& wifiDevModeEnabled = wifiRuntime.devModeEnabled;
extern char* const wifiApSsid = wifiRuntime.apSsid;
extern char* const wifiStaSsid = wifiRuntime.staSsid;
extern char* const wifiStaPassword = wifiRuntime.staPassword;
bool& wifiStaPasswordSet = wifiRuntime.staPasswordSet;
unsigned long& lastWifiConsoleStartAttemptMs = wifiRuntime.lastConsoleStartAttemptMs;
unsigned long& wifiStaConnectStartMs = wifiRuntime.staConnectStartMs;
unsigned long& wifiStaApplyDeadlineMs = wifiRuntime.staApplyDeadlineMs;
unsigned long& wifiApRestartDeadlineMs = wifiRuntime.apRestartDeadlineMs;
unsigned long& wifiStaUpGraceDeadlineMs = wifiRuntime.staUpGraceDeadlineMs;
unsigned long& wifiStaDownGraceDeadlineMs = wifiRuntime.staDownGraceDeadlineMs;
bool& wifiInApOnlyMode = wifiRuntime.inApOnlyMode;

// OTA runtime state aliases
bool& wifiOtaStarted = otaRuntime.started;
bool& wifiOtaWindowOpen = otaRuntime.windowOpen;
bool& wifiOtaInProgress = otaRuntime.inProgress;
bool& wifiOtaParkGuardActive = otaRuntime.parkGuardActive;
unsigned long& wifiOtaDeadlineMs = otaRuntime.deadlineMs;
uint8_t& wifiOtaLastProgressPct = otaRuntime.lastProgressPct;

// Shared web telemetry data buffer (used by WebConsoleServer and WebTelemetry)
unsigned long lastWifiWebDataSampleMs = 0;
uint32_t wifiWebDataSeq = 0;
uint16_t wifiWebDataHead = 0;
uint16_t wifiWebDataCount = 0;

// Preferences remains a standalone global object; the runtime state keeps a
// pointer to it so that modules can access it without an extra extern.
Preferences mus4Prefs;
#endif
int lastSeq = -1;                     // Last received sequence number

ControlData esp_now_data = {0, 0, 0, PARK_LOCKED}; // Initialize the structure at declaration
ControlData rc_data = {0, 0, 0, PARK_LOCKED};      // Initialize the structure at declaration
ControlData pilot_data = {0, 0, 0, PARK_LOCKED};   // Initialize the structure at declaration
ControlData car_output = {0, 0, 0, PARK_LOCKED};   // Initialize the structure at declaration

// Sensor data storage
SensorData ina219Data = {0}, mpu6050Data = {0};
uint8_t g_mpuCandidateAddress = 0;
uint8_t g_mpuWhoAmIValue = 0;
uint32_t g_i2cWorkingSpeed = I2C_SPEED;
uint8_t g_i2cScanAddresses[16] = {0};
uint8_t g_i2cScanCount = 0;


#ifdef ENABLE_WIFI_CONSOLE
static float wifiPseudoSpeedFiltered = 0.0f;

static float computePseudoSpeed()
{
    if (!mpu6050Data.valid) {
        wifiPseudoSpeedFiltered = 0.0f;
        return 0.0f;
    }

    const float gyroDeadzone = 0.15f;
    const float gyroRef = 2.5f;
    const float gyroWeight = 0.6f;
    const float throttleWeight = 0.4f;
    const float riseAlpha = 0.12f;
    const float fallAlpha = 0.05f;

    float gyroMag = fabsf(mpu6050Data.gyroZ);
    if (gyroMag < gyroDeadzone) gyroMag = 0.0f;
    float gyroScore = min(gyroMag / gyroRef, 1.0f);
    float throttleScore = min(fabsf((float)car_output.throttle) / 100.0f, 1.0f);
    float pseudoRaw = gyroScore * gyroWeight + throttleScore * throttleWeight;
    float alpha = pseudoRaw > wifiPseudoSpeedFiltered ? riseAlpha : fallAlpha;
    wifiPseudoSpeedFiltered += (pseudoRaw - wifiPseudoSpeedFiltered) * alpha;
    return constrain(wifiPseudoSpeedFiltered * 100.0f, 0.0f, 100.0f);
}

void sampleWifiWebData()
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
    // IMU 五轴透传：为 Donkey 漂移模型采集 gyro_x/y 与 accel_x/y/z（已在 mpu6050Data 中采样，
    // 此处仅做结构体搬运，归一化与去重力交给训练侧 tools/train_tub_driver.py 处理）。
    point.gyroX = mpu6050Data.gyroX;
    point.gyroY = mpu6050Data.gyroY;
    point.accelX = mpu6050Data.accelX;
    point.accelY = mpu6050Data.accelY;
    point.accelZ = mpu6050Data.accelZ;
    point.driftEnabled = drift_assist_enabled;
    point.driftActive = drift_assist_active;
    point.driftCompensation = drift_compensation;
    point.gyroZFiltered = gyro_z_filtered;
    point.pseudoSpeed = computePseudoSpeed();
    point.actuatorSteeringDuty = actuator_steering_duty;
    point.actuatorThrottleDuty = actuator_throttle_duty;
    wifiWebDataHead = (wifiWebDataHead + 1) % WIFI_WEB_DATA_CAPACITY;
    if (wifiWebDataCount < WIFI_WEB_DATA_CAPACITY) wifiWebDataCount++;
}

static void setupWifiOtaCallbacks()
{
    ArduinoOTA.setHostname(WIFI_OTA_HOSTNAME);
    ArduinoOTA.setPassword(WIFI_OTA_PASSWORD);
    ArduinoOTA.setPort(WIFI_OTA_PORT);
    // v1.7.27：ArduinoOTA 默认 read timeout 仅 1000ms，Flash 写入期间 TCP 窗口
    // 为 0 时容易触发 OTA_RECEIVE_ERROR。设置为 30s，与 HTTP OTA 保持一致。
    ArduinoOTA.setTimeout(30000);
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
            ensureWifiOtaStarted();
            otaRuntime.windowOpen = true;
            otaRuntime.deadlineMs = millis() + 120000UL;
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
            ensureWifiOtaStarted();
            otaRuntime.windowOpen = true;
            otaRuntime.deadlineMs = millis() + 120000UL;
        } else {
            wifiOtaWindowOpen = false;
            wifiOtaParkGuardActive = false;
            wifiOtaDeadlineMs = 0;
        }
        mus4Logf("ota", "error: %u", error);
    });
}

void ensureWifiOtaStarted()
{
    if (wifiOtaStarted) return;
    setupWifiOtaCallbacks();
    ArduinoOTA.begin();
    wifiOtaStarted = true;
}

#else
#endif


int User_throttle = 0;  // User throttle value from the RC transmitter
int User_steering = 0;  // User steering value from the RC transmitter
int Pilot_throttle = 0; // Throttle value from the host computer
int Pilot_steering = 0; // Steering value from the host computer

// RC calibration defaults moved to top of file (must precede function definitions for Arduino preprocessor compatibility)

// Serial2 双向联通验证：独立 ping-pong 协议处理。
// 不走 dispatchCommandLine，避免 PING/PONG 被当作控制命令解析。
static void handleSerial2()
{
    static char line[128];
    static uint8_t idx = 0;
#ifdef ENABLE_SERIAL2_ECHO_TO_SERIAL0
    static unsigned long lastSerial2ByteMs = 0;
#endif
    while (Serial2.available())
    {
        char c = Serial2.read();
#ifdef ENABLE_SERIAL2_ECHO_TO_SERIAL0
        lastSerial2ByteMs = millis();
        // 逐字节实时输出到 Serial0（可打印字符直接显示，不可打印用 \xNN 转义）
        if (c >= 32 && c <= 126)
            Serial.print(c);
        else
            Serial.printf("\\x%02X", (unsigned char)c);
#endif
        if (c == '\r') continue;
        if (c == '\n')
        {
            line[idx] = '\0';
#ifdef ENABLE_SERIAL2_ECHO_TO_SERIAL0
            Serial.println();
#endif
            if (strncmp(line, "STATUS|", 7) == 0)
            {
                // 上位机配网响应：STATUS|CONNECTING
                extern String hostWifiStatus;
                hostWifiStatus = String(line + 7);
                appendWebLog("serial2", String("HOST-WIFI: ") + line);
            }
            else if (strncmp(line, "OK|", 3) == 0)
            {
                // 上位机配网响应：OK|<ip>
                extern String hostWifiStatus;
                extern String hostWifiIp;
                hostWifiStatus = "connected";
                hostWifiIp = String(line + 3);
                appendWebLog("serial2", String("HOST-WIFI: connected ip=") + hostWifiIp);
            }
            else if (strncmp(line, "FAIL|", 5) == 0)
            {
                // 上位机配网响应：FAIL|<reason>
                extern String hostWifiStatus;
                extern String hostWifiError;
                hostWifiStatus = "failed";
                hostWifiError = String(line + 5);
                appendWebLog("serial2", String("HOST-WIFI: failed reason=") + hostWifiError);
            }
            else if (strncmp(line, "PING,", 5) == 0)
            {
                // 第1级：PING → PONG（ping-pong 双向联通协议）
                int seq = atoi(line + 5);
                Serial2.printf("PONG,%d,%lu\n", seq, millis());
            }
#ifdef ENABLE_AUTH_SERVICE
            else if (strncmp(line, "CMD:", 4) == 0 || strncmp(line, "ARG:", 4) == 0)
            {
                // 第2级：Auth 身份识别命令 → processAuthCommand
                String cmdLine(line);
                if (!processAuthCommand(cmdLine, Serial2))
                {
                    // Auth 未消费时回退 ECHO（防御性，正常不会进入此分支）
                    Serial2.printf("ECHO,%s\n", line);
                }
            }
#endif
            else
            {
                // 第3级：其他任意文本 → ECHO 回显
                Serial2.printf("ECHO,%s\n", line);
            }
            idx = 0;
        }
        else if (idx < sizeof(line) - 1)
        {
            line[idx++] = c;
        }
        else
        {
            // 行超长：丢弃超长数据，无条件重置缓冲区
            idx = 0;
        }
#ifdef ENABLE_SERIAL2_ECHO_TO_SERIAL0
        // buffer 满时强制换行（debug echo 开启时同步刷新 Serial0）
        if (idx >= sizeof(line) - 1) {
            Serial.println();
            idx = 0;
        }
#endif
    }
#ifdef ENABLE_SERIAL2_ECHO_TO_SERIAL0
    // 超时刷新：超过 200ms 无新数据则换行
    if (idx > 0 && (millis() - lastSerial2ByteMs > 200)) {
        Serial.println();
        idx = 0;
    }
#endif
    // 每秒心跳
    static unsigned long lastBeat = 0;
    unsigned long now = millis();
    if (now - lastBeat >= 1000)
    {
        Serial2.printf("BEAT,%lu\n", now);
        lastBeat = now;
    }
}

void setup()
{
    // Bind the shared Preferences instance into the runtime state before any
    // module uses wifiRuntime.prefs.
    wifiRuntime.prefs = &mus4Prefs;

    // Provide runtime state references to modules that were previously using
    // scattered extern bool/char variables.
    setWifiRuntimeState(wifiRuntime);
    setWifiIdentityRuntimeState(wifiRuntime);
    setJoystickCalibrationRuntimeState(wifiRuntime);
    setCommandDispatcherRuntimeStates(otaRuntime, wifiRuntime);

    pinMode(UART_SEL, OUTPUT);
    digitalWrite(UART_SEL, HIGH); // Set HIGH to enable TTL <=> CPU: RX_2_PIN = 19, TX_2_PIN = 18      
                           
    Serial.begin(BAUD_RATE_0);                                  // TypeC
    Serial1.begin(BAUD_RATE_1, SERIAL_8N1, RX_1_PIN, TX_1_PIN); // TTL <=> CPU: RX_1_PIN = 16, TX_1_PIN = 17
    Serial1.setTxBufferSize(1024);  // v1.7.33：增大 TX 缓冲区，避免 100Hz IMU + 60Hz 遥测并发写入时
                                    // 默认 256B 环形缓冲区溢出导致帧拼接和字符丢失。
    Serial2.begin(BAUD_RATE_1, SERIAL_8N1, RX_2_PIN, TX_2_PIN); // TTL <=> CPU: RX_2_PIN = 19, TX_2_PIN = 18
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
      pinMode(WIFI_BOOT_RESET_PIN, INPUT_PULLUP);
      loadDevModePreference();
      loadWifiApPreference();
      loadWifiStaPreference();
      loadJudgeConfigPreference();
      loadJoystickCalibration();
      loadServoOutputConfig();
      loadMotorOutputConfig();
      cleanupInvalidOtaPartition();
      // v1.7.28：启动后标记当前固件为有效，取消 bootloader 的 OTA 回滚。
      // 否则第一次 OTA 成功后新分区长期处于 PENDING_VERIFY，下次 reset
      // 可能被 bootloader 回滚到旧固件，表现为 Reset 后仍无法 OTA。
      if (esp_ota_mark_app_valid_cancel_rollback() != ESP_OK) {
        mus4LogLine("ota", "warn: mark app valid failed");
      } else {
        mus4LogLine("ota", "app valid: rollback cancelled");
      }
      setupWifiConsole();
      keepDevModeOtaWindowActive(otaRuntime, wifiRuntime);
    #endif

    g_i2cWorkingSpeed = I2C_SPEED;
    Wire.begin(SDA_PIN, SCL_PIN, g_i2cWorkingSpeed); // SDA = 21, SCL = 22
    delay(100);
    scanI2CBus();
    setup_ina219();
    setup_mpu6050();
    delay(100);

    setupRcPwmCapture();
    setupActuatorOutput();

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

    updateJoystickCalibration();

    readSerialBuf(Serial, serial0Buf);
    readSerialBuf(Serial1, serial1Buf);
    handleSerial2();
    #ifdef ENABLE_WIFI_CONSOLE
      updateWifiConsole();
      updateWifiWebConsole();
      updateWifiSta();
      updateWifiBootResetButton();
      updateWifiOta(otaRuntime, wifiRuntime);
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
    if (!parkValid) {
        // Force Park channel to a clearly released value when RC signal is lost
        // to prevent accidental park locking from stale or last-known pressed PWM.
        pwm_filtered[CH_PARK] = 1000;
        aux_stable_pwm[CH_PARK] = 1000;
        aux_candidate_pwm[CH_PARK] = 1000;
        aux_candidate_count[CH_PARK] = 0;
    }
    if (!driftValid && !aux_stable_initialized[CH_DRIFT]) pwm_filtered[CH_DRIFT] = 1000;
    if (!driftScaleValid && !aux_stable_initialized[CH_DRIFT_SCALE]) pwm_filtered[CH_DRIFT_SCALE] = 1500;

    park_change();
    #ifdef ENABLE_WIFI_CONSOLE
    if (wifiOtaParkGuardActive || wifiOtaInProgress) forceWifiOtaParkLocked();
    #endif
    mode_change(modeValid);
    update_drift_assist_control(driftValid, driftScaleValid);

    updateControlOutput();

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

    // ── Serial1 上行帧拼装与发送 ──────────────────────────────────────
    // v1.7.33：将所有 Serial1 上行数据（T<S>, M:P, $IMU）拼入单一栈缓冲，
    // 一次 write() 发出，彻底消除多次 print/write 调用导致的 TX 环形缓冲区
    // 指针竞争与帧拼接（字符丢失、\n 被吞并）。
    //
    // 发送策略：
    //   - 遥测  T<t>S<s>\n    60Hz（仅 MANUAL 模式）
    //   - 模式  M<m>:P<p>\n   1Hz 心跳 / 状态变化即时
    //   - IMU   $IMU,...\n    100Hz（所有模式，MPU 在线且非 OTA）
    //
    // 缓冲区 512 字节足够容纳三类帧的任意组合（最坏 ~200 字节）。
    {
        char s1Buf[512];
        int  s1Len = 0;

        // --- 遥测帧 T<t>S<s> (60Hz, MANUAL only) ---
        if (car_output.mode == CAR_MODE_MANUAL &&
            millis() - lastRCDataUpdate >= RC_DATA_UPDATE_INTERVAL)
        {
            if (shouldEmitSerial1Telemetry(otaRuntime)) {
                int n = snprintf(s1Buf + s1Len, sizeof(s1Buf) - s1Len,
                                 "T%dS%d\n", car_output.throttle, car_output.steering);
                if (n > 0 && n < (int)(sizeof(s1Buf) - s1Len)) s1Len += n;
#ifdef ENABLE_WIFI_CONSOLE
                if (millis() - lastTelemWebLogMs >= TELEM_WEB_LOG_INTERVAL_MS) {
                    String telem = String("T") + car_output.throttle + "S" + car_output.steering + "\n";
                    appendWebLog("serial1", telem);
                    lastTelemWebLogMs = millis();
                }
#endif

                // --- M<m>:P<p> 模式帧 (1Hz 心跳 / 变化即时) ---
                bool modeChanged = (car_output.mode != lastEmittedMode);
                bool parkChanged = ((int)car_output.park != lastEmittedPark);
                bool heartbeatDue = (millis() - lastModeParkEmitMs) >= MODE_PARK_HEARTBEAT_MS;
                if (modeChanged || parkChanged || heartbeatDue) {
                    int n2 = snprintf(s1Buf + s1Len, sizeof(s1Buf) - s1Len,
                                      "M%d:P%d\n", car_output.mode, (int)car_output.park);
                    if (n2 > 0 && n2 < (int)(sizeof(s1Buf) - s1Len)) s1Len += n2;
#ifdef ENABLE_WIFI_CONSOLE
                    String mp = String("M") + car_output.mode + ":P" + ((int)car_output.park) + "\n";
                    appendWebLog("serial1", mp);
#endif
                    lastEmittedMode = car_output.mode;
                    lastEmittedPark = (int)car_output.park;
                    lastModeParkEmitMs = millis();
                }
            }
            lastRCDataUpdate = millis();
        }

        // --- $IMU 帧 (100Hz, 所有模式) ---
        if (millis() - lastImuEmitMs >= IMU_TELEMETRY_INTERVAL_MS) {
            if (mpu6050Data.valid && shouldEmitSerial1Telemetry(otaRuntime)) {
                static uint16_t imuSeq = 0;
                int n = snprintf(s1Buf + s1Len, sizeof(s1Buf) - s1Len,
                    "$IMU,%u,%lu,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",
                    (unsigned)imuSeq, (unsigned long)millis(),
                    mpu6050Data.accelX, mpu6050Data.accelY, mpu6050Data.accelZ,
                    mpu6050Data.gyroX, mpu6050Data.gyroY, mpu6050Data.gyroZ);
                if (n > 0 && n < (int)(sizeof(s1Buf) - s1Len)) s1Len += n;
                imuSeq++;
            }
            lastImuEmitMs = millis();
        }

        // ── 一次性发出 ──
        if (s1Len > 0) {
            Serial1.write((const uint8_t*)s1Buf, (size_t)s1Len);
        }
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

    updateActuatorOutput();

    if (mus4LogTarget == MUS4_LOG_TARGET_SERIAL) {
        tui.setActuatorDuty(actuator_steering_duty, actuator_throttle_duty);
    }

    buzzer.update();

    scanLEDToggle();
    if (now - lastPerfEval >= 1000)
    {
        evalDegrade();
        if (lastUICycleDuration > 250) uiIntervalCurrent = min(uiIntervalCurrent + 30, uiIntervalMax);
        else uiIntervalCurrent = (uiIntervalCurrent > uiIntervalMin ? uiIntervalCurrent - 20 : uiIntervalMin);
        lastPerfEval = now;
    }
    delay(5);
}
