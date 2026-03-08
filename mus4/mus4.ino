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

#define ENABLE_GAMEPAD_MODE
#ifdef ENABLE_GAMEPAD_MODE
  #include <BleGamepad.h>
  BleGamepad bleGamepad("Gamepad MU03", "Espressif", 100);
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
#define I2C_SPEED 100000L

#define CH_STEERING 0 // index of pwm_value[]
#define CH_THROTTLE 1 // index of pwm_value[]
#define CH_PARK 2     // index of pwm_value[]
#define CH_MODE 3     // index of pwm_value[]

#define CAR_MODE_MANUAL 0    // 0为遥控模式
#define CAR_MODE_SEMI_AUTO 1 // 1为自动方向和手动油门模式
#define CAR_MODE_FULL_AUTO 2 // 2为自动驾驶模式

volatile uint16_t pwm_value[4] = {0, 0, 0, 0};           // value of CH1, CH2, CH3, CH4 (uint16_t for atomic access)
volatile unsigned long rise_time[4] = {0, 0, 0, 0}; // time of rising edge of CH1, CH2, CH3, CH4

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
#define SENSOR_UPDATE_INTERVAL 1000   // 传感器数据更新间隔（毫秒）- 1Hz足够
#define RC_DATA_UPDATE_INTERVAL 16    // RC数据更新间隔（毫秒）- ~60Hz
#define UI_UPDATE_INTERVAL 16         // UI更新间隔（毫秒）- 60Hz丝滑体验

// 波形图参数
#define WAVE_WIDTH 40                 // 波形图宽度
#define WAVE_HEIGHT 8                 // 波形图高度

// 新增全局变量
unsigned long lastSensorUpdate = 0;
unsigned long lastRCDataUpdate = 0;
unsigned long lastUIUpdate = 0;
bool toggleActive = false;
CRGB toggleColor1, toggleColor2;
unsigned long toggleTime = 0;
unsigned long toggleInterval = 250; // LED切换间隔为250ms

// 波形图数据
int throttleWave[WAVE_WIDTH] = {0};
int steeringWave[WAVE_WIDTH] = {0};
int waveIndex = 0;

// 传感器数据存储
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
} ina219Data = {0}, mpu6050Data = {0};
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


void IRAM_ATTR handle_interrupt(int channel)
{ // interrupt handler
    static int pin_state[4] = {0, 0, 0, 0};
    pin_state[channel] = digitalRead(Channels[channel]);
    if (pin_state[channel] == HIGH)
    {
        rise_time[channel] = micros();
    }
    else
    {
        pwm_value[channel] = micros() - rise_time[channel];
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

const int PWM_MIN = 819;     // 'minimum' pulse length count (out of 4096)
const int PWM_MAX = 1638;    // 'maximum' pulse length count (out of 4096)
const int MOTOR_MID = 1229;  // 需要实际测试
const int MOTOR_RANGE = 390; // Pulse range for Motor Throttle
const int SERVO_MID = 1250;  // 需要实际测试
const int SERVO_RANGE = 440; // Pulse range for Motor Throttle
const int MOTOR_OFFSET = 1;
const int SERVO_OFFSET = -1;

// RC Receiver Calibration Values (PWM pulse width in microseconds)
const int RC_THROTTLE_MIN = 888;   // Throttle minimum pulse
const int RC_THROTTLE_MID = 1493;  // Throttle center pulse
const int RC_THROTTLE_MAX = 2149;  // Throttle maximum pulse
const int RC_STEERING_MIN = 872;   // Steering minimum pulse
const int RC_STEERING_MID = 1488;  // Steering center pulse
const int RC_STEERING_MAX = 2113;  // Steering maximum pulse

int carOutputModeLast = -1;
unsigned long counter;

// Define a data structure
struct struct_message
{
    int throttle; // 油门值
    int steering; // 转向值
    int mode;     // 驾驶模式，0为遥控模式，1为自动方向和手动油门模式，2为自动驾驶模式
    bool park;    // 停车状态，0为停车，1为起步
};

struct struct_message esp_now_data = {0, 0, 0, false}; // Initialize the structure at declaration
struct struct_message rc_data = {0, 0, 0, false};      // Initialize the structure at declaration
struct struct_message pilot_data = {0, 0, 0, false};   // Initialize the structure at declaration
struct struct_message car_output = {0, 0, 0, false};   // Initialize the structure at declaration

void emergencyStop()
{
    // 如果停车信号已解除，重置状态机
    if (car_output.park == 0 && emergencyStopState == EST_DONE)
    {
        emergencyStopState = EST_IDLE;
        Serial.println("Emergency Stop FSM reset: Park unlocked");
        return;
    }

    switch (emergencyStopState)
    {
    // case default:
    case EST_IDLE:
        if (car_output.throttle > 0)
        {
            Serial.println("Start Emergency stop");
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
            Serial.println("Emergency STOP ready");
        }
        break;

    case EST_BRAKING:
        if (millis() - emergencyStopStartTime >= EMERGENCY_STOP_BRAKE_DURATION)
        {
            emergencyStopState = EST_DONE;
            Serial.println("Emergency STOP done");
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

// 生成波形图
void generateWaveform(int data[], int length, const char* title)
{
    Serial.printf("%s:\n", title);
    
    // 顶部边框
    Serial.print("  ");
    for (int i = 0; i < length; i++) Serial.print("─");
    Serial.println();
    
    // 波形行
    for (int y = WAVE_HEIGHT; y >= 0; y--)
    {
        Serial.print("  ");
        for (int x = 0; x < length; x++)
        {
            int value = data[x];
            int normalized = map(value, -100, 100, 0, WAVE_HEIGHT);
            
            if (normalized == y)
                Serial.print("█");
            else if (y == WAVE_HEIGHT / 2)
                Serial.print("─");
            else
                Serial.print(" ");
        }
        Serial.println();
    }
    
    // 底部边框
    Serial.print("  ");
    for (int i = 0; i < length; i++) Serial.print("─");
    Serial.println();
}

// 更新波形图数据
void updateWaveformData()
{
    // 移动数据
    for (int i = 1; i < WAVE_WIDTH; i++)
    {
        throttleWave[i-1] = throttleWave[i];
        steeringWave[i-1] = steeringWave[i];
    }
    
    // 添加新数据
    throttleWave[WAVE_WIDTH-1] = car_output.throttle;
    steeringWave[WAVE_WIDTH-1] = car_output.steering;
}

// 显示主界面
void showMainUI()
{
    // 清屏并隐藏光标
    Serial.print(CLEAR_SCREEN);
    Serial.print(CURSOR_HOME);
    Serial.print(HIDE_CURSOR);
    
    // 标题
    Serial.printf(COLOR_CYAN "MUS4 Control System" COLOR_RESET "\n");
    Serial.println("====================");
    
    // 系统状态
    Serial.printf("Mode: ");
    switch (car_output.mode)
    {
        case CAR_MODE_MANUAL:
            Serial.printf(COLOR_GREEN "Manual" COLOR_RESET "\n");
            break;
        case CAR_MODE_SEMI_AUTO:
            Serial.printf(COLOR_YELLOW "Semi-Auto" COLOR_RESET "\n");
            break;
        case CAR_MODE_FULL_AUTO:
            Serial.printf(COLOR_BLUE "Full-Auto" COLOR_RESET "\n");
            break;
    }
    
    Serial.printf("Park: %s\n", car_output.park ? COLOR_RED "Locked" COLOR_RESET : COLOR_GREEN "Unlocked" COLOR_RESET);
    
    // RC 数据
    Serial.println("\nRC Channels:");
    Serial.printf("CH1 (Steering): %4d | CH2 (Throttle): %4d\n", pwm_value[CH_STEERING], pwm_value[CH_THROTTLE]);
    Serial.printf("CH3 (Park): %4d | CH4 (Mode): %4d\n", pwm_value[CH_PARK], pwm_value[CH_MODE]);
    
    // 控制输出
    Serial.println("\nControl Output:");
    Serial.printf("Throttle: %4d | Steering: %4d\n", car_output.throttle, car_output.steering);
    
    // 波形图
    Serial.println("\nWaveforms:");
    generateWaveform(throttleWave, WAVE_WIDTH, "Throttle");
    generateWaveform(steeringWave, WAVE_WIDTH, "Steering");
    
    // 传感器数据
    Serial.println("\nSensors:");
    
    // INA219 数据
    if (ina219Data.valid)
    {
        Serial.printf("INA219[%lu|%lu] Bus:%.2fV Current:%.1fmA Power:%.1fmW\n", 
            ina219Data.lastReadTime, ina219Data.readCount,
            ina219Data.busVoltage, ina219Data.current_mA, ina219Data.power_mW);
    }
    else
    {
        Serial.println("INA219: No data");
    }
    
    // MPU6050 数据
    if (mpu6050Data.valid)
    {
        Serial.printf("MPU6050[%lu|%lu] Accel:X=%.2f Y=%.2f Z=%.2f Gyro:X=%.2f Y=%.2f Z=%.2f T:%.1fC\n",
            mpu6050Data.lastReadTime, mpu6050Data.readCount,
            mpu6050Data.accelX, mpu6050Data.accelY, mpu6050Data.accelZ,
            mpu6050Data.gyroX, mpu6050Data.gyroY, mpu6050Data.gyroZ,
            mpu6050Data.temperature);
    }
    else
    {
        Serial.println("MPU6050: No data");
    }
    
    // 显示光标
    Serial.print(SHOW_CURSOR);
}

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
                        Serial.println("System Unlocked: Park Mode Exited");
                    }
                }
                else
                { // Currently Unlocked (Drive Mode)
                    // Lock Logic: Hold for 0.5s
                    if (duration >= PARK_LOCK_HOLD_TIME)
                    {
                        rc_data.park = true; // Lock
                        parkActionTaken = true;
                        Serial.println("System Locked: Park Mode Entered");
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
        Serial.print("[CMD ERROR] Out of range: T=");
        Serial.print(t);
        Serial.print(" S=");
        Serial.println(s);
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
        Serial.println("I2C Read Errro");
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
        Serial.println("I2C Write Error");
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
    
    for(address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        
        if (error == 0)
        {
            Serial.print("[I2C SCAN] Found device at 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            
            // 识别常见设备
            switch(address) {
                case 0x40:
                    Serial.println(" - INA219");
                    break;
                case 0x41:
                    Serial.println(" - INA219 (alt address)");
                    break;
                case 0x68:
                    Serial.println(" - MPU6050");
                    break;
                case 0x69:
                    Serial.println(" - MPU6050 (alt address)");
                    break;
                default:
                    Serial.println(" - Unknown device");
                    break;
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

void setup_mpu6050()
{
    int retryCount = 0;
    const int maxRetries = 3;
    
    while (retryCount < maxRetries)
    {
        if (!mpu.begin())
        {
            retryCount++;
            Serial.printf("[MPU6050] Initialization attempt %d/%d failed\n", retryCount, maxRetries);
            
            if (retryCount < maxRetries)
            {
                delay(500);
                continue;
            }
            
            Serial.println("[MPU6050 ERROR] Failed to find MPU6050 chip");
            Serial.println("[MPU6050 ERROR] Please check I2C connection (SDA: GPIO 21, SCL: GPIO 22)");
            Serial.println("[MPU6050 ERROR] Possible causes:");
            Serial.println("  1. I2C address mismatch (try 0x68 or 0x69)");
            Serial.println("  2. Wiring issues (SDA/SCL swapped or loose)");
            Serial.println("  3. Power supply issue");
            Serial.println("  4. I2C bus speed too high");
            while (1)
            {
                delay(1000);
                Serial.println("[MPU6050 ERROR] Sensor not detected, waiting...");
            }
        }
        else
        {
            break;
        }
    }
    Serial.println("[MPU6050] Sensor initialized successfully!");

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

void setup()
{
    pinMode(UART_SEL, OUTPUT);
    // digitalWrite(UART_SEL, HIGH);
    digitalWrite(UART_SEL, LOW);

    Serial.begin(BAUD_RATE_0);                                  // TypeC
    Serial1.begin(BAUD_RATE_1, SERIAL_8N1, RX_1_PIN, TX_1_PIN); // RS232: rx = 16, tx = 17
    Serial.println("ESP32 Receiver Serial Ready!");
    Serial1.println("ESP32 Receiver Serial1 Ready!");

    #ifdef ENABLE_GAMEPAD_MODE
      bleGamepad.begin();
    #endif

    Wire.begin(SDA_PIN, SCL_PIN, I2C_SPEED); // SDA = 21, SCL = 22, 100kHz
    delay(100);
    scanI2CBus();
    setup_ina219();
    setup_mpu6050();
    delay(100);

    // Set the RC receiver pins as inputs and attach the interrupts
    for (int i = 0; i < 4; i++)
    {
        pinMode(Channels[i], INPUT);
        attachInterrupt(digitalPinToInterrupt(Channels[i]), isr_functions[i], CHANGE);
    }

    ledcAttachChannel(STEERING_PIN, 50, 14, CH_STEERING);
    ledcAttachChannel(THROTTLE_PIN, 50, 14, CH_THROTTLE);

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);

    // 替换原有直接设置颜色的方式
    setLEDColor(CRGB::Blue); // 使用新函数设置初始颜色

    // Initialize Park State (Default Locked)
    rc_data.park = true; 
    car_output.park = true;
    emergencyStopState = EST_IDLE;
    Serial.println("System Initialized: Park Locked");

    delay(1000);
}

void loop()
{
    // 传感器数据更新（限制频率）
    if (millis() - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL)
    {
        read_ina219();
        read_mpu6050();
        lastSensorUpdate = millis();
    }

    // Serial represents the Type-C USB port
    if (Serial.available())
    {
        String CMD = Serial.readStringUntil('\n');
        int throttle, steering;
        if (parseAndValidateCommand(CMD, &throttle, &steering))
        {
            pilot_data.throttle = throttle;
            pilot_data.steering = steering;
            Serial.print("CMD-T:");
            Serial.print(pilot_data.throttle);
            Serial.print(" CMD-S:");
            Serial.println(pilot_data.steering);
        }
    }

    // Serial1 represents the RS232 port
    if (Serial1.available())
    {
        String CMD = Serial1.readStringUntil('\n');
        Serial.println(CMD);
        int throttle, steering;
        if (parseAndValidateCommand(CMD, &throttle, &steering))
        {
            pilot_data.throttle = throttle;
            pilot_data.steering = steering;
            Serial.print("CMD-T:");
            Serial.print(pilot_data.throttle);
            Serial.print(" CMD-S:");
            Serial.println(pilot_data.steering);
        }
    }

    rc_data.steering = pwm_value[CH_STEERING];
    rc_data.throttle = pwm_value[CH_THROTTLE];
    park_change(); // pwm_value[CH_PARK]
    mode_change(); // pwm_value[CH_MODE]

    if (car_output.mode == CAR_MODE_FULL_AUTO)
    {
        // Controlled by Pilot
        if (car_output.park == 1)
        {
            // car_output.throttle = 0;
            emergencyStop();
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
            emergencyStop();
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
            emergencyStop();
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

    // 更新波形图数据
    updateWaveformData();

    // UI更新（限制频率）
    if (millis() - lastUIUpdate >= UI_UPDATE_INTERVAL)
    {
        showMainUI();
        lastUIUpdate = millis();
    }

    // RC数据更新（限制频率）- 保持原有功能
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

    int pwm_steering = map(car_output.steering, -100, 100, SERVO_MID - SERVO_RANGE, SERVO_MID + SERVO_RANGE);
    int pwm_throttle = map(car_output.throttle, -100, 100, MOTOR_MID - MOTOR_RANGE, MOTOR_MID + MOTOR_RANGE);

    pwm_steering = min(max(pwm_steering, PWM_MIN), PWM_MAX);
    pwm_throttle = min(max(pwm_throttle, PWM_MIN), PWM_MAX);

    ledcWriteChannel(CH_STEERING, pwm_steering);
    ledcWriteChannel(CH_THROTTLE, pwm_throttle);

    delay(10);
    counter += 1;

    scanLEDToggle();
}