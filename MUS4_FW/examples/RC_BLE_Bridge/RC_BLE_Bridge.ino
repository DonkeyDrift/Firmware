/*
 * RC_BLE_Bridge —— 把 HOT RC CT-8B（F-08A 接收机）的 PWM 信号桥接成 BLE HID 手柄
 *
 * 用途：CT-8B 枪控本身没有蓝牙、也没有任何电脑接口，只与自家接收机通信。
 * 本固件让一块普通 ESP32 开发板读取接收机 CH1/CH2 的 PWM 脉宽，把自己伪装成
 * 标准 BLE HID Gamepad（设备名 "Gamepad MU02"）。Mac/PC 在蓝牙设置里配对后，
 * 打开 DD（DonkeyDrift）页面驾驶页，输入源选「手柄」即可操控模拟器
 * （DD 默认 z-axis 预设：转向=右摇杆 X、油门=左摇杆 Y 反向，与本固件映射一致，
 * 通常免校准；有偏差时在 DD 手柄设置面板校准一次即可，按设备记忆）。
 *
 * 接线（F-08A 接收机 → ESP32 开发板）：
 *   VCC ← 5V（VIN）     GND —— GND（必须共地）
 *   CH1（转向）→ GPIO36  CH2（油门）→ GPIO39
 * 接收机可临时拆用车上的，或另购 F-08A 与 CT-8B 对码（对码不影响车上原有的绑定）。
 *
 * 编译（在 MUS4_FW 目录下执行，复用本仓库 vendored 的 ESP32-BLE-Gamepad + NimBLE）：
 *   arduino-cli compile --fqbn esp32:esp32:esp32 \
 *     --libraries libraries --build-path build-bridge examples/RC_BLE_Bridge
 * 刷机：arduino-cli upload -p <串口> --fqbn esp32:esp32:esp32 \
 *     --libraries libraries examples/RC_BLE_Bridge
 */

#include <Arduino.h>
#include <BleGamepad.h>

// ---------- 引脚 ----------
#define CH1_PIN 36  // 接收机 CH1：转向
#define CH2_PIN 39  // 接收机 CH2：油门

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

BleGamepad bleGamepad("Gamepad MU02", "Espressif", 100);

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

void setup()
{
    Serial.begin(115200);
    pinMode(CH1_PIN, INPUT);
    pinMode(CH2_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(CH1_PIN), isrCh1, CHANGE);
    attachInterrupt(digitalPinToInterrupt(CH2_PIN), isrCh2, CHANGE);

    bleGamepad.begin();
    Serial.println("RC_BLE_Bridge ready: 广播 BLE 手柄 \"Gamepad MU02\"，等待配对…");
}

void loop()
{
    sendGamepadPacket();

    // 调试输出：连接后每 500ms 打印一次脉宽，方便不接电脑也能核对信号
    static unsigned long lastPrintMs = 0;
    if (bleGamepad.isConnected() && millis() - lastPrintMs >= 500)
    {
        lastPrintMs = millis();
        Serial.printf("CH1(转向)=%uµs  CH2(油门)=%uµs\n", pwm_us[0], pwm_us[1]);
    }
    delay(1);
}
