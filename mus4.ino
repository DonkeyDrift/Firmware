//=============================================================
/* 
[Note]
1. 针对MUS4-v2.3 PCB 调整了部分引脚定义
    - CH1_PIN 36 // 接收机pwm输入CH1通道
    - CH2_PIN 39 // 接收机pwm输入CH2通道
    - CH3_PIN 34 // 接收机pwm输入CH3通道
    - CH4_PIN 26 // 接收机pwm输入CH4通道
    - CH1_ST 23 // CH1转向舵机
    - CH2_TH 25 // CH2油门电调
    - PWM_1 32 // PWM输出1号通道
    - PWM_2 33 // PWM输出2号通道

2. 为测试接收机，屏蔽了模式选择和停车功能【注意】

[Experience]
1. 固件程序下载速率为115200
2. 串口协议：T:S\n
  T代表Throttle
  S代表Steering
  结尾为"\n
*/

#include <Wire.h>
#include <FastLED.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_INA219.h>
#include "SharedTypes.h"
#include "TUI.h"

#define BUZZER_PIN 2

#include "Buzzer.h"
// #include "test_runner.h"

TUI tui(Serial);
Buzzer buzzer(BUZZER_PIN);

int lastCarMode = -1;
bool lastParkState = false;

#define ENABLE_GAMEPAD_MODE
#ifdef ENABLE_GAMEPAD_MODE
  #include <BleGamepad.h>
  BleGamepad bleGamepad("Gamepad MU02", "Espressif", 100);
#endif

Adafruit_MPU6050 mpu;
Adafruit_INA219 ina219;

// #define DEBUG // Uncomment to enable debugging output

#define CH1_PIN 36 // 接收机pwm输入CH1通道
#define CH2_PIN 39 // 接收机pwm输入CH2通道
#define CH3_PIN 34 // 接收机pwm输入CH3通道
#define CH4_PIN 26// 接收机pwm输入CH4通道

#define STEERING_PIN 23 // CH1转向舵机
#define THROTTLE_PIN 25 // CH2油门电调

#define PWM_1 32 // PWM输出1号通道
#define PWM_2 33 // PWM输出2号通道

#define LED_PIN 5
#define NUM_LEDS 1
#define BRIGHTNESS 64
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

#define BAUD_RATE_0 115200
#define RX_1_PIN 16
#define TX_1_PIN 17
// #define RX_1_PIN 19
// #define TX_1_PIN 18      // MU02 无法联通，统一切 PIN 16, 17
#define BAUD_RATE_1 115200
#define UART_SEL 12

#define SDA_PIN 21
#define SCL_PIN 22
#define I2C_SPEED 400000L

#define CH_STEERING 0 // index of pwm_value[]
#define CH_THROTTLE 1 // index of pwm_value[]
#define CH_PARK 2     // index of pwm_value[]
#define CH_MODE 3     // index of pwm_value[]

#define CAR_MODE_MANUAL 0    // 0为遥控模式
#define CAR_MODE_SEMI_AUTO 1 // 1为自动方向和手动油门模式
#define CAR_MODE_FULL_AUTO 2 // 2为自动驾驶模式

#define PARK_LOCKED true     // 锁定状态
#define PARK_UNLOCKED false  // 解锁状态

volatile uint16_t pwm_value[4] = {0, 0, 0, 0};           // value of CH1, CH2, CH3, CH4 (uint16_t for atomic access)
volatile unsigned long rise_time[4] = {0, 0, 0, 0}; // time of rising edge of CH1, CH2, CH3, CH4
volatile unsigned long last_valid_time[4] = {0, 0, 0, 0}; // last valid signal time for each channel
#define RC_SIGNAL_TIMEOUT 1000000UL  // RC信号超时时间 (µs)
#define RC_PWM_MIN 800   // 最小有效PWM (µs)
#define RC_PWM_MAX 2200  // 最大有效PWM (µs)

#define PWM_FILTER_SIZE 5  // 滑动窗口中值滤波器大小 (5-7)
uint16_t pwm_filter_buf[4][PWM_FILTER_SIZE] = {{0}};  // 滤波缓冲区
uint8_t pwm_filter_idx[4] = {0};
uint16_t pwm_filtered[4] = {0, 0, 0, 0};  // 滤波后的PWM值
bool filterDebugEnabled = false;          // 调试输出开关

const int Channels[4] = {CH1_PIN, CH2_PIN, CH3_PIN, CH4_PIN};

CRGB leds[NUM_LEDS]; // Define the array of leds

// 终端控制宏
#define CLEAR_SCREEN "\033[2J"         // 清屏
#define CURSOR_HOME "\033[H"           // 光标回到 home 位置
#define CURSOR_UP(n) "\033[" #n "A"    // 光标上移 n 行
#define CURSOR_DOWN(n) "\033[" #n "B"  // 光标下移 n 行
#define CURSOR_RIGHT(n) "\033[" #n "C" // 光标右移 n 列
#define CURSOR_LEFT(n) "\033[" #n "D"  // 光标左移 n 列
#define SAVE_CURSOR "\033[s"           // 保存光标位置
#define RESTORE_CURSOR "\033[u"        // 恢复光标位置
#define HIDE_CURSOR "\033[?25l"        // 隐藏光标
#define SHOW_CURSOR "\033[?25h"        // 显示光标

// 颜色宏
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"

// 输出控制参数
#define SENSOR_UPDATE_INTERVAL 8     // 传感器数据更新间隔（毫秒）- ~60Hz
#define RC_DATA_UPDATE_INTERVAL 8    // RC数据更新间隔（毫秒）- ~60Hz
#define RC_FILTER_UPDATE_INTERVAL 4   // RC滤波更新间隔（毫秒）- ~125Hz，平衡响应和稳定
#define UI_UPDATE_INTERVAL 16         // UI更新间隔（毫秒）- 60Hz丝滑体验

// 波形图参数
#define WAVE_WIDTH 20                 // 波形图宽度 (reduced for performance)
#define WAVE_HEIGHT 6                 // 波形图高度 (reduced for performance)

// 新增全局变量
unsigned long lastSensorUpdate = 0;
unsigned long lastRCDataUpdate = 0;
unsigned long lastRCFilterUpdate = 0;
unsigned long lastUIUpdate = 0;
unsigned long lastWaveUpdate = 0;     // 波形刷新独立计时
const unsigned long WAVE_UPDATE_INTERVAL = 250; // 4Hz刷新率
bool toggleActive = false;
CRGB toggleColor1, toggleColor2;
unsigned long toggleTime = 0;
unsigned long toggleInterval = 250; // LED切换间隔为250ms
bool degradeMode = false;
uint32_t degradeReason = 0;
bool ansiEnabled = true;
bool ansiDetected = false;            // 自动检测ANSI支持状态
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
int lastWaveTh[WAVE_WIDTH] = {0};     // 缓存上一帧波形用于脏矩形
int lastWaveSt[WAVE_WIDTH] = {0};     // 缓存上一帧波形用于脏矩形
bool forceRedraw = false;             // 强制重绘标志
int lastSeq = -1;                     // 记录收到的最后序号

// 优化的插入排序中值滤波 (O(n^2)但对于n=5非常快且稳定)
static uint16_t medianFilter(uint16_t* buf, int size) {
    uint16_t temp[8]; // 最大支持8个元素
    // 复制数据
    for (int i = 0; i < size; i++) temp[i] = buf[i];
    
    // 插入排序
    for (int i = 1; i < size; i++) {
        uint16_t key = temp[i];
        int j = i - 1;
        while (j >= 0 && temp[j] > key) {
            temp[j + 1] = temp[j];
            j = j - 1;
        }
        temp[j + 1] = key;
    }
    
    // 返回中值
    return temp[size / 2];
}

static bool runFilterTests()
{
    Serial.println("Running Filter Tests...");
    bool passed = true;
    
    // 模拟缓冲区
    uint16_t testBuf[PWM_FILTER_SIZE];
    for(int i=0; i<PWM_FILTER_SIZE; i++) testBuf[i] = 1500;
    
    // Test 1: 稳态测试
    uint16_t out = medianFilter(testBuf, PWM_FILTER_SIZE);
    if (out != 1500) { Serial.printf("Filter Test 1 Failed: Expected 1500, got %d\n", out); passed = false; }
    
    // Test 2: 单点尖峰抑制 (2000us 突变)
    testBuf[2] = 2000; // 中间突变
    out = medianFilter(testBuf, PWM_FILTER_SIZE);
    if (out != 1500) { Serial.printf("Filter Test 2 Failed: Spike not suppressed, got %d\n", out); passed = false; }
    testBuf[2] = 1500; // 恢复
    
    // Test 3: 双点尖峰 (连续两个异常值，对于5点窗口仍应被抑制)
    testBuf[1] = 2000;
    testBuf[2] = 2000;
    out = medianFilter(testBuf, PWM_FILTER_SIZE);
    if (out != 1500) { Serial.printf("Filter Test 3 Failed: Double spike not suppressed, got %d\n", out); passed = false; }
    
    // Test 4: 阶跃响应 (多数变为新值)
    testBuf[0] = 1600;
    testBuf[1] = 1600;
    testBuf[2] = 1600; // 3/5 变为 1600
    out = medianFilter(testBuf, PWM_FILTER_SIZE);
    if (out != 1600) { Serial.printf("Filter Test 4 Failed: Step response failed, got %d\n", out); passed = false; }

    if (passed) Serial.println("Filter Tests Passed!");
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
    Serial.printf("BENCH: loops=%lu duration=%lums\n", score, t1);
    return score > 1;
}
struct struct_message
{
    int throttle; // 油门值
    int steering; // 转向值
    int mode;     // 驾驶模式，0为遥控模式，1为自动方向和手动油门模式，2为自动驾驶模式
    bool park;    // 停车状态，0为停车，1为起步
};

struct struct_message esp_now_data = {0, 0, 0, PARK_LOCKED}; // Initialize the structure at declaration
struct struct_message rc_data = {0, 0, 0, PARK_LOCKED};      // Initialize the structure at declaration
struct struct_message pilot_data = {0, 0, 0, PARK_LOCKED};   // Initialize the structure at declaration
struct struct_message car_output = {0, 0, 0, PARK_LOCKED};   // Initialize the structure at declaration

// 300Hz PWM输出参数 (适用于舵机和电调)
// 频率 = 80MHz / (prescale * resolution)
// 300Hz = 80000000 / (prescale * 16384) → prescale ≈ 16
// 脉宽计算: count = (pulse_us / period_us) * 2^14
// 周期 = 1000000/300 = 3333.33µs
const int PWM_PERIOD_US = 3333;  // 300Hz周期 (µs)
const int PWM_MIN_V = 4915;      // 1000µs @ 300Hz (1000/3333.33×16384 ≈ 4915)
const int PWM_MAX_V = 9830;      // 2000µs @ 300Hz (2000/3333.33×16384 ≈ 9830)
const int MOTOR_MID_V = 7372;    // 1500µs @ 300Hz
const int MOTOR_RANGE_V = 2458; // ±500µs范围
const int SERVO_MID_V = 7372;    // 1500µs @ 300Hz
const int SERVO_RANGE_V = 2458; // ±500µs范围
const int MOTOR_OFFSET_V = 1;
const int SERVO_OFFSET_V = -1;

static bool runStress()
{
    uint32_t errs0 = serial0Buf.errors;
    for (int i=0;i<50;i++)
    {
        int tt,ss,seq;
        processLine(String("999:999"), &tt,&ss,&seq);
    }
    Serial.printf("STRESS: errors_delta=%lu\n", serial0Buf.errors-errs0);
    return true;
}

static bool runRegression()
{
    int v = map(-100, -100, 100, SERVO_MID_V - SERVO_RANGE_V, SERVO_MID_V + SERVO_RANGE_V);
    int v2 = map(100, -100, 100, SERVO_MID_V - SERVO_RANGE_V, SERVO_MID_V + SERVO_RANGE_V);
    bool ok = (v <= v2);
    Serial.printf("REGRESS: ok=%d\n", ok?1:0);
    return ok;
}

// 波形图数据
int throttleWave[WAVE_WIDTH] = {0};
int steeringWave[WAVE_WIDTH] = {0};
int waveIndex = 0;

// 传感器数据存储
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
unsigned long emergencyStopStartTime = 0;                 // 标志是否准备刹车
const unsigned long EMERGENCY_STOP_READY_DURATION = 500;  // 刹车准备时间100ms
const unsigned long EMERGENCY_STOP_BRAKE_DURATION = 1500; // 刹车持续时间1500ms

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
// ------------------------------------------------------

// 修改setLEDColor函数
void setLEDColor(CRGB targetColor)
{
    if (toggleActive)
    {
        toggleActive = false; // 关闭切换模式
        toggleTime = 0;       // 重置切换时间
    }
    if (leds[0] != targetColor)
    {
        leds[0] = targetColor;
        FastLED.show(); // 不一致时才更新显示
    }
}

// 新增setLEDToggle函数
void setLEDToggle(CRGB color1, CRGB color2)
{
    toggleColor1 = color1;
    toggleColor2 = color2;
    toggleActive = true;
    toggleTime = 0; // 确保首次切换立即执行
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
    if (ansiEnabled)
    {
        Serial.print(COLOR_RED);
        Serial.println("DEGRADED MODE ACTIVE");
        Serial.print(COLOR_RESET);
    }
    else
    {
        Serial.println("DEGRADED MODE ACTIVE");
    }
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
    // 如果是命令
    if (line.equalsIgnoreCase("NOANSI")) { ansiEnabled = false; tui.setAnsiEnabled(false); tui.forceRedraw(); return false; }
    if (line.equalsIgnoreCase("ANSI")) { ansiEnabled = true; tui.setAnsiEnabled(true); tui.forceRedraw(); return false; }
    if (line.equalsIgnoreCase("FILTER_DEBUG")) { 
        filterDebugEnabled = !filterDebugEnabled; 
        Serial.printf("Filter Debug: %s\n", filterDebugEnabled ? "ON" : "OFF"); 
        return false; 
    }
    if (line.equalsIgnoreCase("FILTER_TEST")) { 
        runFilterTests(); 
        return false; 
    }

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
            
            // 尝试解析SEQ: T:S:SEQ
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
    
    // 无校验，尝试解析 T:S:SEQ
    int col2 = line.lastIndexOf(':');
    int col1 = line.indexOf(':');
    if (col2 > col1 && col1 > 0) {
            String seqStr = line.substring(col2+1);
            *seq = seqStr.toInt();
            return parseAndValidateCommand(line.substring(0, col2), throttle, steering);
    }
    
    return parseAndValidateCommand(line, throttle, steering);
}

static void readSerialBuf(HardwareSerial& ser, SerialBuf& sb, bool isRS232)
{
    while (ser.available())
    {
        int c = ser.read();
        if (c < 0) break;
        if (c == '\r') continue;
        if (c == '\n')
        {
            sb.buf[sb.len] = 0;
            String line = String(sb.buf);
            if (line.equalsIgnoreCase("TEST"))
            {
                bool ok = runUnitTests();
                Serial.printf("TEST: total=%d passed=%d ok=%d\n", testsTotal, testsPassed, ok?1:0);
                sb.len = 0; sb.overflow = false; continue;
            }
            if (line.equalsIgnoreCase("TEST_TUI"))
            {
                // TestRegistry::runAll();
                Serial.println("Skipped TEST_TUI");
                sb.len = 0; sb.overflow = false; continue;
            }
            if (line.equalsIgnoreCase("BENCH"))
            {
                bool ok = runBenchmarks();
                Serial.printf("BENCH_OK=%d\n", ok?1:0);
                sb.len = 0; sb.overflow = false; continue;
            }
            if (line.equalsIgnoreCase("STRESS"))
            {
                bool ok = runStress();
                Serial.printf("STRESS_OK=%d\n", ok?1:0);
                sb.len = 0; sb.overflow = false; continue;
            }
            if (line.equalsIgnoreCase("REGRESS"))
            {
                bool ok = runRegression();
                Serial.printf("REGRESS_OK=%d\n", ok?1:0);
                sb.len = 0; sb.overflow = false; continue;
            }
            int t, s, seq;
            bool ok = processLine(line, &t, &s, &seq);
            if (ok)
            {
                pilot_data.throttle = t;
                pilot_data.steering = s;
                lastSeq = seq;
                if (isRS232) {
                    if (seq >= 0) ser.printf("ACK:%d\n", seq);
                    else ser.println("ACK");
                }
                else {
                    if (seq >= 0) Serial.printf("ACK:%d\n", seq);
                    else Serial.println("ACK");
                }
                sb.frames++;
            }
            else
            {
                if (isRS232) {
                     if (seq >= 0) ser.printf("NACK:%d\n", seq);
                     else ser.println("NACK");
                }
                else {
                     if (seq >= 0) Serial.printf("NACK:%d\n", seq);
                     else Serial.println("NACK");
                }
                sb.errors++;
            }
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


void IRAM_ATTR handle_interrupt(int channel)
{ // interrupt handler
    static int pin_state[4] = {0, 0, 0, 0};
    static unsigned long last_edge_time[4] = {0, 0, 0, 0};
    static unsigned long last_rise_time[4] = {0, 0, 0, 0};

    unsigned long now = micros();
    // 防抖：两个边沿之间至少间隔100µs（更灵敏）
    if (now - last_edge_time[channel] < 100) return;
    last_edge_time[channel] = now;

    pin_state[channel] = digitalRead(Channels[channel]);
    if (pin_state[channel] == HIGH)
    {
        last_rise_time[channel] = now;
    }
    else
    {
        uint16_t width = now - last_rise_time[channel];
        // 范围检查，只接受有效PWM值
        if (width >= RC_PWM_MIN && width <= RC_PWM_MAX) {
            uint16_t prev = pwm_value[channel];
            int diff = abs((int)width - (int)prev);
            
            // 小变化直接接受
            if (diff <= 120) {
                pwm_value[channel] = width;
                last_valid_time[channel] = now;
            }
            // 中等变化需要一次确认
            else if (diff <= 200) {
                static uint16_t candidate_pwm[4] = {0};
                if (abs((int)width - (int)candidate_pwm[channel]) < 80) {
                    pwm_value[channel] = width;
                    last_valid_time[channel] = now;
                }
                candidate_pwm[channel] = width;
            }
            // 大变化需要两次确认（防止误触发）
            else {
                static uint16_t large_change_count[4] = {0};
                static uint16_t last_large_pwm[4] = {0};
                if (abs((int)width - (int)last_large_pwm[channel]) < 100) {
                    large_change_count[channel]++;
                    if (large_change_count[channel] >= 2) {
                        pwm_value[channel] = width;
                        last_valid_time[channel] = now;
                        large_change_count[channel] = 0;
                    }
                } else {
                    large_change_count[channel] = 0;
                }
                last_large_pwm[channel] = width;
            }
        }
    }
}

void IRAM_ATTR CH1_interrupt() { handle_interrupt(0); } // interrupt handler
void IRAM_ATTR CH2_interrupt() { handle_interrupt(1); }
void IRAM_ATTR CH3_interrupt() { handle_interrupt(2); }
void IRAM_ATTR CH4_interrupt() { handle_interrupt(3); }

void (*isr_functions[4])() = {CH1_interrupt, CH2_interrupt, CH3_interrupt, CH4_interrupt}; // array of function pointers

int User_throttle = 0;  // RC遥控器发来的用户油门值
int User_steering = 0;  // RC遥控器发来的用户转向值
int Pilot_throttle = 0; // 上位机发来的油门值
int Pilot_steering = 0; // 上位机发来的转向值

// RC Receiver Calibration Values (PWM pulse width in microseconds)
const int RC_THROTTLE_MIN = 888;   // Throttle minimum pulse
const int RC_THROTTLE_MID = 1493;  // Throttle center pulse
const int RC_THROTTLE_MAX = 2149;  // Throttle maximum pulse
const int RC_STEERING_MIN = 872;   // Steering minimum pulse
const int RC_STEERING_MID = 1488;  // Steering center pulse
const int RC_STEERING_MAX = 2113;  // Steering maximum pulse

int carOutputModeLast = -1;
unsigned long counter;

void emergencyStop()
{
    // 如果停车信号已解除，重置状态机
    if (car_output.park == 0 && emergencyStopState == EST_DONE)
    {
        emergencyStopState = EST_IDLE;
        tui.log("Emergency Stop FSM reset: Park unlocked");
        return;
    }

    switch (emergencyStopState)
    {
    // case default:
    case EST_IDLE:
        if (car_output.throttle > 0)
        {
            tui.log("Start Emergency stop");
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
            tui.log("Emergency STOP ready");
        }
        break;

    case EST_BRAKING:
        if (millis() - emergencyStopStartTime >= EMERGENCY_STOP_BRAKE_DURATION)
        {
            emergencyStopState = EST_DONE;
            tui.log("Emergency STOP done");
        }
        break;

    case EST_DONE:
        // 刹车完成，油门归零
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
    bool isPressed = (pwm_value[CH_PARK] > 1500);

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
                        tui.log("System Unlocked: Park Mode Exited");
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
                        tui.log("System Locked: Park Mode Entered");
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

    // 校验范围：-100 ~ 100
    if (t < -100 || t > 100 || s < -100 || s > 100)
    {
        // 只有当不是测试命令时才打印错误，避免污染输出
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

void mode_change() // 根据遥控器的mode值，切换驾驶模式
{
    rc_data.mode = pwm_value[CH_MODE];
    if (rc_data.mode <= 1400)
    {
        car_output.mode = CAR_MODE_MANUAL; // 0为遥控模式
    }
    else if (rc_data.mode > 1400 && rc_data.mode < 1600)
    {
        car_output.mode = CAR_MODE_SEMI_AUTO; // 1为自动方向和手动油门模式
    }
    else
    {
        car_output.mode = CAR_MODE_FULL_AUTO; // 2为自动驾驶模式
    }

    if (car_output.mode != lastCarMode)
    {
        buzzer.playModeSound(car_output.mode);
        lastCarMode = car_output.mode;
    }
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
    Serial.println("[I2C SCAN] Last scan summary:");
    if (g_i2cScanCount == 0)
    {
        Serial.println("[I2C SCAN]   No devices recorded in last scan");
        return;
    }

    for (uint8_t i = 0; i < g_i2cScanCount; i++)
    {
        uint8_t addr = g_i2cScanAddresses[i];
        Serial.printf("[I2C SCAN]   0x%02X - %s\n", addr, identifyI2CDeviceByAddress(addr));
    }
}

void read_ina219()
{
    // 读取INA219数据
    float shuntvoltage = ina219.getShuntVoltage_mV();
    float busvoltage = ina219.getBusVoltage_V();
    float current_mA = ina219.getCurrent_mA();
    float power_mW = ina219.getPower_mW();
    float loadvoltage = busvoltage + (shuntvoltage / 1000);
    
    // 检查数据有效性
    if (current_mA == 0 && busvoltage == 0 && power_mW == 0)
    {
        ina219Data.valid = false;
        return;
    }
    
    // 存储数据到全局变量
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
    Serial.println("[INA219] Initializing INA219 sensor...");
    
    if (!ina219.begin())
    {
        Serial.println("[INA219 ERROR] Failed to find INA219 chip");
        Serial.println("[INA219 ERROR] Please check I2C connection (SDA: GPIO 21, SCL: GPIO 22)");
        Serial.println("[INA219 ERROR] Possible causes:");
        Serial.println("  1. I2C address mismatch (default is 0x40)");
        Serial.println("  2. Wiring issues (SDA/SCL swapped or loose)");
        Serial.println("  3. Power supply issue");
        while (1)
        {
            delay(1000);
            Serial.println("[INA219 ERROR] Sensor not detected, waiting...");
        }
    }
    
    Serial.println("[INA219] Sensor initialized successfully!");
    
    // 使用默认校准（32V, 2A范围）
    // 如需更高精度，可以取消注释以下任一行：
    // ina219.setCalibration_32V_1A();  // 32V, 1A范围（更高精度）
    // ina219.setCalibration_16V_400mA(); // 16V, 400mA范围（最高精度）
    
    Serial.println("[INA219] Calibration: 32V, 2A range (default)");
    Serial.println("[INA219] Setup complete, ready for data acquisition");
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
    
    // 存储数据到全局变量
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
    Serial.println("[I2C SCAN] Scanning I2C bus...");
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
            Serial.print("[I2C SCAN] Found device at 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            Serial.print(" - ");
            Serial.println(identifyI2CDeviceByAddress(address));
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
                    Serial.print("[I2C SCAN] MPU probe OK at 0x");
                    if (address < 16) Serial.print("0");
                    Serial.print(address, HEX);
                    Serial.print(" (WHO_AM_I=0x");
                    if (whoAmI < 16) Serial.print("0");
                    Serial.print(whoAmI, HEX);
                    Serial.println(")");
                }
                else if (I2CReadRegister8(address, 0x75, &whoAmI))
                {
                    Serial.print("[I2C SCAN] MPU-family device at 0x");
                    if (address < 16) Serial.print("0");
                    Serial.print(address, HEX);
                    Serial.print(" but WHO_AM_I=0x");
                    if (whoAmI < 16) Serial.print("0");
                    Serial.print(whoAmI, HEX);
                    Serial.println(" (not MPU6050)");
                }
                else
                {
                    Serial.print("[I2C SCAN] Could not read WHO_AM_I at 0x");
                    if (address < 16) Serial.print("0");
                    Serial.println(address, HEX);
                }
            }
            nDevices++;
        }
        else if (error == 4)
        {
            Serial.print("[I2C SCAN] Unknown error at 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
        }
    }
    
    if (nDevices == 0)
    {
        Serial.println("[I2C SCAN] No I2C devices found!");
    }
    else
    {
        Serial.printf("[I2C SCAN] Found %d device(s)\n", nDevices);
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
            Serial.printf("[MPU6050] Try addr 0x%02X (attempt %d/%d)\n", addr, retryCount, maxRetriesPerAddress);

            if (mpu.begin(addr, &Wire))
            {
                *activeAddress = addr;
                return true;
            }

            Serial.printf("[MPU6050] Init failed at 0x%02X\n", addr);
            delay(300);
        }
    }

    return false;
}

void setup_mpu6050()
{
    Serial.println("[MPU6050] Initializing MPU6050 sensor...");
    uint8_t activeAddress = 0;
    const int maxRetriesPerAddress = 2;
    bool initOk = tryInitMPU6050OnCurrentBus(&activeAddress, maxRetriesPerAddress);

    if (!initOk)
    {
        Serial.println("[MPU6050] Retry with lower I2C speed: 100kHz");
        Wire.begin(SDA_PIN, SCL_PIN, 100000L);
        g_i2cWorkingSpeed = 100000L;
        delay(50);
        scanI2CBus();
        initOk = tryInitMPU6050OnCurrentBus(&activeAddress, maxRetriesPerAddress);
    }

    if (!initOk)
    {
        Serial.println("[MPU6050] Retry with lower I2C speed: 50kHz");
        Wire.begin(SDA_PIN, SCL_PIN, 50000L);
        g_i2cWorkingSpeed = 50000L;
        delay(50);
        scanI2CBus();
        initOk = tryInitMPU6050OnCurrentBus(&activeAddress, maxRetriesPerAddress);
    }

    if (!initOk)
    {
        Serial.println("[MPU6050 ERROR] Failed to find MPU6050 chip");
        Serial.println("[MPU6050 ERROR] Please check I2C connection (SDA: GPIO 21, SCL: GPIO 22)");
        Serial.println("[MPU6050 ERROR] Possible causes:");
        Serial.println("  1. I2C address mismatch (try 0x68 or 0x69)");
        Serial.println("  2. Wiring issues (SDA/SCL swapped or loose)");
        Serial.println("  3. Power supply issue");
        Serial.println("  4. I2C bus speed too high");
        printLastI2CScanSummary();

        if (g_mpuCandidateAddress != 0)
        {
            Serial.printf("[MPU6050 ERROR] Probe saw candidate at 0x%02X, WHO_AM_I=0x%02X\n",
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
                Serial.println("[MPU6050 ERROR] Sensor not detected, waiting...");
            }

            if (now - lastRescanMs >= 5000UL)
            {
                lastRescanMs = now;
                Serial.println("[MPU6050 ERROR] Auto re-scan I2C bus (5s interval)...");
                scanI2CBus();
                printLastI2CScanSummary();
            }

            delay(50);
        }
    }

    Serial.println("[MPU6050] Sensor initialized successfully!");
    Serial.printf("[MPU6050] Active I2C address: 0x%02X\n", activeAddress);
    Serial.printf("[MPU6050] Active I2C speed: %lu Hz\n", g_i2cWorkingSpeed);
    if (g_mpuWhoAmIValue != 0)
    {
        Serial.printf("[MPU6050] WHO_AM_I = 0x%02X\n", g_mpuWhoAmIValue);
    }

    // 对已初始化地址做一次WHO_AM_I确认，避免总线干扰导致误识别
    uint8_t whoAmI = 0;
    if (I2CReadRegister8(activeAddress, 0x75, &whoAmI))
    {
        Serial.printf("[MPU6050] WHO_AM_I readback: 0x%02X\n", whoAmI);
        if (whoAmI != 0x68 && whoAmI != 0x69)
        {
            Serial.println("[MPU6050 WARNING] WHO_AM_I mismatch, device may not be MPU6050");
        }
    }
    else
    {
        Serial.println("[MPU6050 WARNING] WHO_AM_I readback failed after init");
    }

    // set accelerometer Range
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    Serial.print("[MPU6050] Accelerometer range set to: ");

    switch (mpu.getAccelerometerRange())
    {
    case MPU6050_RANGE_2_G:
        Serial.println("+-2G");
        break;
    case MPU6050_RANGE_4_G:
        Serial.println("+-4G");
        break;
    case MPU6050_RANGE_8_G:
        Serial.println("+-8G");
        break;
    case MPU6050_RANGE_16_G:
        Serial.println("+-16G");
        break;
    }

    // set Gyro Range
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    Serial.print("[MPU6050] Gyro range set to: ");
    switch (mpu.getGyroRange())
    {
    case MPU6050_RANGE_250_DEG:
        Serial.println("+- 250 deg/s");
        break;
    case MPU6050_RANGE_500_DEG:
        Serial.println("+- 500 deg/s");
        break;
    case MPU6050_RANGE_1000_DEG:
        Serial.println("+- 1000 deg/s");
        break;
    case MPU6050_RANGE_2000_DEG:
        Serial.println("+- 2000 deg/s");
        break;
    }

    // Set filter bandwidth to 94Hz for approximately 100Hz sampling rate
    mpu.setFilterBandwidth(MPU6050_BAND_94_HZ);
    Serial.print("[MPU6050] Filter bandwidth set to: ");
    switch (mpu.getFilterBandwidth())
    {
    case MPU6050_BAND_260_HZ:
        Serial.println("260 Hz");
        break;
    case MPU6050_BAND_184_HZ:
        Serial.println("184 Hz");
        break;
    case MPU6050_BAND_94_HZ:
        Serial.println("94 Hz (Sampling rate: ~100Hz)");
        break;
    case MPU6050_BAND_44_HZ:
        Serial.println("44 Hz");
        break;
    case MPU6050_BAND_21_HZ:
        Serial.println("21 Hz");
        break;
    case MPU6050_BAND_10_HZ:
        Serial.println("10 Hz");
        break;
    case MPU6050_BAND_5_HZ:
        Serial.println("5 Hz");
        break;
    default:
        Serial.println("Unknown");
        break;
    }
    Serial.println("[MPU6050] Setup complete, ready for data acquisition");
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
    float target_steering = map(filtered_pwm - 1488, 872 - 1488, 2113 - 1488, -100, 100);

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
                Serial.println("ALARM: Steering Sensor Fault! Safe Mode Activated.");
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
                Serial.println("INFO: Steering Signal Recovered. Exiting Safe Mode.");
                
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

void run_steering_tests() {
    Serial.println("--- Starting Steering Signal Processing Unit Tests (PID Enabled) ---");
    
    // Test 1: Normal Value (PID Convergence)
    reset_steering_filter();
    int res = 0;
    // Simulate convergence
    for(int i=0; i<20; i++) {
        res = process_steering_signal(1488);
    }
    Serial.printf("Test 1 (Normal 1488 -> 0): Output=%d, Pass=%d\n", res, res == 0);

    // Test 2: Boundary Values
    reset_steering_filter();
    // Fill buffer to avoid smoothing delay effect for test
    for(int i=0; i<10; i++) process_steering_signal(872); 
    // Run PID loop to converge
    for(int i=0; i<20; i++) res = process_steering_signal(872);
    Serial.printf("Test 2A (Min 872 -> -100): Output=%d, Pass=%d\n", res, res == -100);

    reset_steering_filter();
    for(int i=0; i<10; i++) process_steering_signal(2113);
    for(int i=0; i<20; i++) res = process_steering_signal(2113);
    Serial.printf("Test 2B (Max 2113 -> 100): Output=%d, Pass=%d\n", res, res == 100);

    // Test 3: Noise Injection (Should be ignored or dampened)
    reset_steering_filter();
    // Converge to center
    for(int i=0; i<20; i++) process_steering_signal(1488); 
    
    // Inject single frame noise (0 is invalid PWM, so it uses last valid 1488)
    int noise_res = process_steering_signal(0); 
    Serial.printf("Test 3 (Invalid Input 0 -> Hold Last): Output=%d, Pass=%d\n", noise_res, noise_res == 0);

    // Test 4: Hard Clamping
    reset_steering_filter();
    // Inject value that maps to > 100 but is valid PWM (e.g. 2200)
    for(int i=0; i<30; i++) res = process_steering_signal(2200);
    Serial.printf("Test 4 (Clamp 2200 -> 100): Output=%d, Pass=%d\n", res, res == 100);

    // Test 5: Safety Mode Activation
    reset_steering_filter();
    // Trigger error. 
    // Since we have a 10-point moving average, we need enough samples for the average to cross the threshold.
    // Target threshold > 120 corresponds to filtered_pwm > approx 2237.
    // Input 2300.
    for(int i=0; i<15; i++) {
        process_steering_signal(2300);
    }
    Serial.printf("Test 5 (Safety Mode Activation): Active=%d, Pass=%d\n", safe_mode_active, safe_mode_active == true);

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
    
    Serial.printf("Test 6 (Safety Mode Recovery): Still Active at 50=%d, Recovered at 51=%d, Pass=%d\n", 
                  still_active, recovered, still_active && recovered);

    Serial.println("--- End Tests ---");
    reset_steering_filter(); // Reset for actual operation
}

void setup()
{
    pinMode(UART_SEL, OUTPUT);
    // digitalWrite(UART_SEL, HIGH);
    digitalWrite(UART_SEL, LOW);

    Serial.begin(BAUD_RATE_0);                                  // TypeC
    Serial1.begin(BAUD_RATE_1, SERIAL_8N1, RX_1_PIN, TX_1_PIN); // RS232: rx = 16, tx = 17
    Serial.println("ESP32 Receiver Serial Ready!");
    Serial1.println("ESP32 Receiver Serial1 Ready!");

    run_steering_tests(); // Run unit tests for steering signal processing

    #ifdef ENABLE_GAMEPAD_MODE
      bleGamepad.begin();
    #endif

    g_i2cWorkingSpeed = I2C_SPEED;
    Wire.begin(SDA_PIN, SCL_PIN, g_i2cWorkingSpeed); // SDA = 21, SCL = 22
    delay(100);
    scanI2CBus();
    setup_ina219();
    setup_mpu6050();
    delay(100);

    // Set the RC receiver pins as inputs and attach the interrupts
    for (int i = 0; i < 4; i++)
    {
        if (Channels[i] == 26) {
            // GPIO 26 支持内部下拉电阻
            pinMode(Channels[i], INPUT_PULLDOWN);
        } else {
            // GPIO 36, 39, 34 是仅输入引脚，不支持内部上拉/下拉
            pinMode(Channels[i], INPUT);
        }
        attachInterrupt(digitalPinToInterrupt(Channels[i]), isr_functions[i], CHANGE);
    }

    ledcAttachChannel(STEERING_PIN, 300, 14, CH_STEERING);
    ledcAttachChannel(THROTTLE_PIN, 300, 14, CH_THROTTLE);

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);

    // 替换原有直接设置颜色的方式
    setLEDColor(CRGB::Blue); // 使用新函数设置初始颜色

    // Initialize Park State (Default Locked)
    rc_data.park = PARK_LOCKED; 
    car_output.park = PARK_LOCKED;
    emergencyStopState = EST_IDLE;
    tui.log("System Locked: Park Mode Active");

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

    readSerialBuf(Serial, serial0Buf, false);
    readSerialBuf(Serial1, serial1Buf, true);

    // RC信号读取：检查超时和有效性，应用滑动平均滤波（带更新间隔控制）
    unsigned long nowUs = micros();
    bool steeringValid = (nowUs - last_valid_time[CH_STEERING]) < RC_SIGNAL_TIMEOUT;
    bool throttleValid = (nowUs - last_valid_time[CH_THROTTLE]) < RC_SIGNAL_TIMEOUT;
    bool parkValid = (nowUs - last_valid_time[CH_PARK]) < RC_SIGNAL_TIMEOUT;
    bool modeValid = (nowUs - last_valid_time[CH_MODE]) < RC_SIGNAL_TIMEOUT;

    if (millis() - lastRCFilterUpdate >= RC_FILTER_UPDATE_INTERVAL) {
        // 改进的滤波：滑动窗口中值滤波 (Size=5)
        auto filterPWM = [&](int ch, uint16_t raw, bool valid) -> uint16_t {
            if (!valid) return 1500;
            
            // 边界保护：检查是否在合理 PWM 范围内 (800-2200us)
            // 如果超出范围，视为噪声丢弃（不更新缓冲区，直接返回上一次滤波值）
            if (raw < RC_PWM_MIN || raw > RC_PWM_MAX) {
                return pwm_filtered[ch];
            }

            uint8_t idx = pwm_filter_idx[ch];
            pwm_filter_buf[ch][idx] = raw;
            pwm_filter_idx[ch] = (idx + 1) % PWM_FILTER_SIZE;
            
            // 纯中值滤波：确保输出为窗口内排序后的中间值
            uint16_t median = medianFilter(pwm_filter_buf[ch], PWM_FILTER_SIZE);
            
            // 调试输出
            if (filterDebugEnabled && ch == CH_THROTTLE) {
                 Serial.printf("F_DBG: ch=%d, raw=%d, med=%d\n", ch, raw, median);
            }

            return median;
        };

        // 对所有通道应用滤波
        for (int i = 0; i < 4; i++) {
            bool valid = (nowUs - last_valid_time[i]) < RC_SIGNAL_TIMEOUT;
            pwm_filtered[i] = filterPWM(i, pwm_value[i], valid);
        }
        lastRCFilterUpdate = millis();
    }

    // 信号有效时更新rc_data，否则保持默认值（中立位置）
    if (steeringValid) {
        rc_data.steering = pwm_filtered[CH_STEERING];
    } else {
        rc_data.steering = RC_STEERING_MID; // 超时后使用中值
    }
    if (throttleValid) {
        rc_data.throttle = pwm_filtered[CH_THROTTLE];
    } else {
        rc_data.throttle = RC_THROTTLE_MID; // 超时后使用中值
    }

    // Park和Mode通道也做类似处理
    pwm_filtered[CH_PARK] = parkValid ? pwm_filtered[CH_PARK] : 1500;
    pwm_filtered[CH_MODE] = modeValid ? pwm_filtered[CH_MODE] : 1500;

    park_change();
    mode_change();

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
        car_output.steering = map(rc_data.steering, RC_STEERING_MIN, RC_STEERING_MAX, -100, 100);
    }

    // Update TUI
    tui.setRC(pwm_filtered[0], pwm_filtered[1], pwm_filtered[2], pwm_filtered[3]);
    tui.setOutput(car_output.throttle, car_output.steering, car_output.mode, car_output.park);
    
    // Merge Sensor Data
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
    
    // Set refresh rate dynamically based on load (from old logic)
    tui.setRefreshRate(uiIntervalCurrent);
    tui.setAnsiEnabled(ansiEnabled);
    tui.setWaveformEnabled(false); // 禁用波形显示，因为滤波会引入延迟
    
    tui.update(millis());
    lastUICycleDuration = tui.getLastRenderDuration();

    if (millis() - lastRCDataUpdate >= RC_DATA_UPDATE_INTERVAL)
    {
        Serial1.printf("T%d:S%d\n", car_output.throttle, car_output.steering); // RC => Type-C
        lastRCDataUpdate = millis();
    }

#ifdef DEBUG // Print the values for debugging
    // Read the RC receiver values
    for (int i = 0; i < 4; i++)
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
    delay(10);
}
