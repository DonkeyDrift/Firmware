/*
 * RC_BLE_Bridge —— 把 HOT RC CT-8B（F-08A 接收机）的 PWM 信号桥接成 BLE HID 手柄
 *
 * 用途：CT-8B 枪控本身没有蓝牙、也没有任何电脑接口，只与自家接收机通信。
 * 本固件让 ESP32 读取接收机 CH1/CH2 的 PWM 脉宽，把自己伪装成标准 BLE HID
 * Gamepad（设备名 "Gamepad MU02"）。Mac/PC 蓝牙配对后，打开 DD（DonkeyDrift）
 * 页面驾驶页，输入源选「手柄」即可操控模拟器（DD 默认 z-axis 预设与本固件
 * 映射一致，通常免校准；有偏差时在 DD 手柄设置面板校准一次，按设备记忆）。
 *
 * ── 车上复用（推荐，零新增硬件）────────────────────────────────
 * 直接 OTA 刷到车上 ESP32 用：接收机本来就接在 GPIO36/39、已与 CT-8B 对码。
 * 桥模式下车端 Web Console/驾驶功能暂停（固件不在跑），舵机/电调无输出
 * （车原地不动，天然安全）；玩完通过本固件自带的 OTA 通道把车固件刷回即
 * 100% 复原。刷桥（车端正常固件下执行）：
 *   curl -F "update=@build-bridge/RC_BLE_Bridge.ino.bin" \
 *        "http://<车IP>/update?auth="
 * 刷回车固件（桥模式下执行，两条通道任选）：
 *   curl -F "update=@build/MUS4_FW.ino.bin" "http://<车IP>/update"
 *   或 ArduinoOTA（端口 3232，密码 mus4-debug，hostname mus4-rc-bridge）
 * Wi-Fi 连不上时自动开兜底 AP：SSID "MUS4-RC-Bridge"（开放），地址 192.168.4.1，
 * 同样带 /update——永远刷得回来。
 *
 * ── 独立开发板用法 ─────────────────────────────────────────────
 * 接线（F-08A 接收机 → ESP32）：VCC←5V（VIN）、GND 共地、
 * CH1（转向）→GPIO36、CH2（油门）→GPIO39。
 *
 * 编译（在 MUS4_FW 目录下，复用仓库 vendored 库，分区方案与车端一致）：
 *   arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=min_spiffs \
 *     --libraries libraries --build-path build-bridge examples/RC_BLE_Bridge
 * 有线刷机：arduino-cli upload -p <串口> --fqbn esp32:esp32:esp32 \
 *     --libraries libraries examples/RC_BLE_Bridge
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ArduinoOTA.h>
#include <BleGamepad.h>

// ---------- WiFi 凭据 ----------
// 把本机 libraries/mus4_core/src/WirelessSecrets.h 复制到本 sketch 目录
// （examples/RC_BLE_Bridge/WirelessSecrets.h）即可参与编译；该文件名已被
// .gitignore 全局忽略、不入库。没有该文件时下面占位生效：STA 不连接，直接走
// 兜底 AP（MUS4-RC-Bridge，192.168.4.1），功能不受影响。
#if __has_include("WirelessSecrets.h")
#include "WirelessSecrets.h"
#endif
#ifndef WIFI_STA_SSID
#define WIFI_STA_SSID ""      // 占位：无密钥文件时 STA 不连接，直接走 AP 兜底
#define WIFI_STA_PASSWORD ""
#endif

// ---------- 引脚 ----------
#define CH1_PIN 36  // 接收机 CH1：转向（与车上接线一致）
#define CH2_PIN 39  // 接收机 CH2：油门（与车上接线一致）

// ---------- RC 校准（与 MUS4_FW FirmwareConfig.h 中 CT-8B 的实测校准值保持一致） ----------
#define RC_STEERING_MIN 872
#define RC_STEERING_MID 1488
#define RC_STEERING_MAX 2113
#define RC_THROTTLE_MIN 888
#define RC_THROTTLE_MID 1493
#define RC_THROTTLE_MAX 2149
#define RC_PWM_MIN 800   // 有效脉宽下限（µs）
#define RC_PWM_MAX 2200  // 有效脉宽上限（µs）
#define RC_SIGNAL_TIMEOUT_US 1000000UL  // 1s 无有效脉冲判失控

// ---------- 手柄轴（ESP32-BLE-Gamepad 默认量程 0..32767，中位 16384） ----------
#define AXIS_MIN 0
#define AXIS_MID 16384
#define AXIS_MAX 32767
#define SEND_INTERVAL_MS 20  // 发送节奏 50Hz，避免淹没 BLE 通知通道

// ---------- WiFi / OTA（回刷通道） ----------
#define WIFI_STA_TIMEOUT_MS 15000          // STA 连接超时，超时转 AP 兜底
#define BRIDGE_AP_SSID "MUS4-RC-Bridge"    // 兜底 AP（开放，与车端 WIFI_CONSOLE_AP_PASSWORD 为空的习惯一致）
#define OTA_HOSTNAME "mus4-rc-bridge"      // ArduinoOTA 主机名（端口 3232）
#define OTA_PASSWORD "mus4-debug"          // ArduinoOTA 密码（与车端一致）

BleGamepad bleGamepad("Gamepad MU02", "Espressif", 100);
WebServer otaServer(80);

volatile uint16_t pwm_us[2] = {0, 0};            // 最近一次有效脉宽（0 = 尚无信号）
volatile unsigned long last_valid_us[2] = {0, 0};
static unsigned long rise_us[2] = {0, 0};

// 脉宽落在有效窗口内才接受；无效脉冲直接丢弃（窗口外多半是干扰尖峰）
static void IRAM_ATTR handleEdge(int ch, int pin)
{
    unsigned long now = micros();
    if (digitalRead(pin) == HIGH)
    {
        rise_us[ch] = now;
        return;
    }
    unsigned long width = now - rise_us[ch];
    if (width >= RC_PWM_MIN && width <= RC_PWM_MAX)
    {
        pwm_us[ch] = (uint16_t)width;
        last_valid_us[ch] = now;
    }
}

static void IRAM_ATTR isrCh1() { handleEdge(0, CH1_PIN); }
static void IRAM_ATTR isrCh2() { handleEdge(1, CH2_PIN); }

// 把一路 RC 脉宽按校准 MIN/MID/MAX 分段线性映射到手柄轴，中位精确落在 AXIS_MID
static int16_t mapRcAxis(uint16_t pwm, uint16_t minUs, uint16_t midUs, uint16_t maxUs)
{
    int p = constrain((int)pwm, (int)minUs, (int)maxUs);
    if (p <= (int)midUs)
    {
        return (int16_t)map(p, (int)minUs, (int)midUs, AXIS_MIN, AXIS_MID);
    }
    return (int16_t)map(p, (int)midUs, (int)maxUs, AXIS_MID, AXIS_MAX);
}

// 通道在有效脉宽窗口内且未超时才算存活
static bool rcChannelAlive(int ch, unsigned long now)
{
    return pwm_us[ch] >= RC_PWM_MIN && (now - last_valid_us[ch]) < RC_SIGNAL_TIMEOUT_US;
}

void sendGamepadPacket()
{
    if (!bleGamepad.isConnected()) return;

    static unsigned long lastSendMs = 0;
    unsigned long nowMs = millis();
    if (nowMs - lastSendMs < SEND_INTERVAL_MS) return;
    lastSendMs = nowMs;

    unsigned long now = micros();

    // 失控保护：通道无有效信号时该轴回中位，避免沿用旧值让被控端跑偏
    int lx = AXIS_MID;
    if (rcChannelAlive(0, now))
    {
        lx = mapRcAxis(pwm_us[0], RC_STEERING_MIN, RC_STEERING_MID, RC_STEERING_MAX);
    }

    // 油门按手柄惯例反向：前推到底 = 轴最小值（等同摇杆前推为负），
    // DD 默认 z-axis 预设的油门 invert=true 正好还原为正油门
    int ly = AXIS_MID;
    if (rcChannelAlive(1, now))
    {
        ly = AXIS_MAX - mapRcAxis(pwm_us[1], RC_THROTTLE_MIN, RC_THROTTLE_MID, RC_THROTTLE_MAX);
    }

    bleGamepad.setLeftThumb(0, ly);
    bleGamepad.setRightThumb(lx, 0);
}

// ---------- HTTP /update（回刷通道 2）：POST multipart 文件字段 "update" ----------
static bool otaStarted = false;

static void handleUpdateGet()
{
    otaServer.send(200, "text/html",
                   "<form method='POST' action='/update' enctype='multipart/form-data'>"
                   "<input type='file' name='update'><input type='submit' value='Update'></form>");
}

static void handleUpdatePost()
{
    // 鉴权要求与车端一致：只有确实发生过上传才重启，空 POST 直接 400
    if (otaStarted)
    {
        otaServer.send(200, "text/plain", "ACK:UPDATE_OK");
        delay(200);
        ESP.restart();
    }
    else
    {
        otaServer.send(400, "text/plain", "NACK:NO_UPLOAD");
    }
}

static void handleUpdateUpload()
{
    HTTPUpload &upload = otaServer.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
        otaStarted = true;
        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
        {
            Update.printError(Serial);
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        {
            Update.printError(Serial);
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (!Update.end(true))
        {
            Update.printError(Serial);
        }
    }
    else if (upload.status == UPLOAD_FILE_ABORTED)
    {
        Update.abort();
        otaStarted = false;
    }
}

static void setupWifiAndOta()
{
    bool staOk = false;
    if (WIFI_STA_SSID[0] != '\0')
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_STA_TIMEOUT_MS)
        {
            delay(250);
        }
        staOk = (WiFi.status() == WL_CONNECTED);
    }

    if (staOk)
    {
        Serial.printf("WiFi STA 已连接，IP=%s\n", WiFi.localIP().toString().c_str());
    }
    else
    {
        // 兜底 AP：连不上家里 Wi-Fi（或没有密钥文件）时也能通过 192.168.4.1 刷回
        WiFi.mode(WIFI_AP);
        WiFi.softAP(BRIDGE_AP_SSID);
        Serial.printf("WiFi STA 失败，已开兜底 AP \"%s\"，IP=%s\n",
                      BRIDGE_AP_SSID, WiFi.softAPIP().toString().c_str());
    }

    // 回刷通道 1：ArduinoOTA（端口 3232）
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.begin();

    // 回刷通道 2：HTTP /update（端口 80）
    otaServer.on("/update", HTTP_GET, handleUpdateGet);
    otaServer.on("/update", HTTP_POST, handleUpdatePost, handleUpdateUpload);
    otaServer.begin();
    Serial.println("OTA 双通道就绪：ArduinoOTA(3232) + HTTP /update(80)");
}

void setup()
{
    Serial.begin(115200);
    pinMode(CH1_PIN, INPUT);
    pinMode(CH2_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(CH1_PIN), isrCh1, CHANGE);
    attachInterrupt(digitalPinToInterrupt(CH2_PIN), isrCh2, CHANGE);

    // 先起 BLE，保证 WiFi 连接等待期间手柄已可配对
    bleGamepad.begin();
    Serial.println("RC_BLE_Bridge ready: 广播 BLE 手柄 \"Gamepad MU02\"，等待配对…");

    setupWifiAndOta();
}

void loop()
{
    sendGamepadPacket();
    ArduinoOTA.handle();
    otaServer.handleClient();

    // 调试输出：连接后每 500ms 打印一次脉宽，方便不接电脑也能核对信号
    static unsigned long lastPrintMs = 0;
    if (bleGamepad.isConnected() && millis() - lastPrintMs >= 500)
    {
        lastPrintMs = millis();
        Serial.printf("CH1(转向)=%uµs  CH2(油门)=%uµs\n", pwm_us[0], pwm_us[1]);
    }
    delay(1);
}
