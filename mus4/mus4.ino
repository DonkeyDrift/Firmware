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

// #include <esp_now.h>
// #include <WiFi.h>
#include <Wire.h>
#include <FastLED.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define ENABLE_GAMEPAD_MODE
#ifdef ENABLE_GAMEPAD_MODE
  #include <BleGamepad.h>
  BleGamepad bleGamepad("Gamepad MU03", "Espressif", 100);
#endif

Adafruit_MPU6050 mpu;

#define DEBUG // Uncomment to enable debugging output

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

#define BUAD_RATE_0 115200
#define RX_1_PIN 16
#define TX_1_PIN 17
// #define RX_1_PIN 19
// #define TX_1_PIN 18      // MU02 无法联通，统一切 PIN 16, 17
#define BUAD_RATE_1 115200
#define UART_SEL 12

#define SDA_PIN 13
#define SCL_PIN 14
#define I2C_SPEED 400000L

#define CH_STEERING 0 // index of pwm_value[]
#define CH_THROTTLE 1 // index of pwm_value[]
#define CH_PARK 2     // index of pwm_value[]
#define CH_MODE 3     // index of pwm_value[]

#define CAR_MODE_MANUAL 0    // 0为遥控模式
#define CAR_MODE_SEMI_AUTO 1 // 1为自动方向和手动油门模式
#define CAR_MODE_FULL_AUTO 2 // 2为自动驾驶模式

volatile int pwm_value[4] = {0, 0, 0, 0};           // value of CH1, CH2, CH3, CH4
volatile unsigned long rise_time[4] = {0, 0, 0, 0}; // time of rising edge of CH1, CH2, CH3, CH4

const int Channels[4] = {CH1_PIN, CH2_PIN, CH3_PIN, CH4_PIN};

CRGB leds[NUM_LEDS]; // Define the array of leds

// 新增全局变量
bool toggleActive = false;
CRGB toggleColor1, toggleColor2;
unsigned long toggleTime = 0;
unsigned long toggleInterval = 250; // LED切换间隔为250ms
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

// Create a structure object
// struct_message* myData;

// callback function executed when data is received.
// void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
// 	myData = (struct_message*)incomingData;

//   esp_now_data = *myData; //将接收到的数据赋值给esp_now_data

// }

int adj(int v, int s) // v: value, s: step
{
    v = v + s;
    if (v > 4095)
        v = 4095;
    if (v < 0)
        v = 0;
    return v;
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
    uint16_t current = I2CReadValue(0x41, 1);
    uint16_t voltage = I2CReadValue(0x41, 2);
    
    if (current == 0xFFFF || voltage == 0xFFFF)
    {
        Serial.println("[INA219 ERROR] Failed to read sensor data");
        return;
    }
    
    float current_mA = current / 100.0 / 0.01;
    float voltage_V = voltage / 2 / 1000.0;
    
    Serial.printf("[INA219] Current: %.2f mA, Voltage: %.2f V\r\n", current_mA, voltage_V);
}

void read_mpu6050()
{
    static unsigned long lastReadTime = 0;
    static unsigned long readCount = 0;
    
    /* Get new sensor events with the readings */
    sensors_event_t a, g, temp;
    
    if (!mpu.getEvent(&a, &g, &temp))
    {
        Serial.println("[MPU6050 ERROR] Failed to read sensor data");
        return;
    }
    
    readCount++;
    lastReadTime = millis();
    
    /* Print out the values with timestamp */
    Serial.print("[MPU6050] Time: ");
    Serial.print(lastReadTime);
    Serial.print("ms | Count: ");
    Serial.print(readCount);
    Serial.print(" | Status: OK");
    Serial.println();
    
    Serial.print("  Acceleration (m/s^2): X=");
    Serial.print(a.acceleration.x, 3);
    Serial.print(" Y=");
    Serial.print(a.acceleration.y, 3);
    Serial.print(" Z=");
    Serial.println(a.acceleration.z, 3);
    
    Serial.print("  Gyro (deg/s): X=");
    Serial.print(g.gyro.x, 3);
    Serial.print(" Y=");
    Serial.print(g.gyro.y, 3);
    Serial.print(" Z=");
    Serial.println(g.gyro.z, 3);
    
    Serial.print("  Temperature: ");
    Serial.print(temp.temperature, 2);
    Serial.println(" degC");
    Serial.println();
}

void setup_mpu6050()
{
    // Try to initialize!
    if (!mpu.begin())
    {
        Serial.println("[MPU6050 ERROR] Failed to find MPU6050 chip");
        Serial.println("[MPU6050 ERROR] Please check I2C connection (SDA: GPIO 13, SCL: GPIO 14)");
        while (1)
        {
            delay(1000);
            Serial.println("[MPU6050 ERROR] Sensor not detected, waiting...");
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

    Serial.begin(BUAD_RATE_0);                                  // TypeC
    Serial1.begin(BUAD_RATE_1, SERIAL_8N1, RX_1_PIN, TX_1_PIN); // RS232: rx = 16, tx = 17
    Serial.println("ESP32 Receiver Serial Ready!");
    Serial1.println("ESP32 Receiver Serial1 Ready!");

    #ifdef ENABLE_GAMEPAD_MODE
      bleGamepad.begin();
    #endif

    Wire.begin(SDA_PIN, SCL_PIN, I2C_SPEED); // SDA = 13, SCL = 14, 400kHz
    setup_mpu6050();
    delay(100);

    // Set the RC receiver pins as inputs and attach the interrupts
    for (int i = 0; i < 4; i++)
    {
        pinMode(Channels[i], INPUT);
        attachInterrupt(digitalPinToInterrupt(Channels[i]), isr_functions[i], CHANGE);
    }

    // // 分配PWM输出通道到管脚
    ledcAttachChannel(STEERING_PIN, 50, 14, CH_STEERING);
    ledcAttachChannel(THROTTLE_PIN, 50, 14, CH_THROTTLE);

    // WiFi.mode(WIFI_STA);
    // for(int i = 0; i < 10; i++)
    //   Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());

    // if (esp_now_init() != esp_OK) {
    //   Serial.println("Error initializing ESP-NOW");
    //   return;
    // }

    // esp_now_register_recv_cb(OnDataRecv);

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
    read_ina219();
    read_mpu6050();

    // Serial represents the Type-C USB port
    if (Serial.available())
    {
        String CMD = Serial.readStringUntil('\n');
        int colonIndex = CMD.indexOf(':');
        if (colonIndex > 0)
        {
            pilot_data.throttle = CMD.substring(0, colonIndex).toInt();
            pilot_data.steering = CMD.substring(colonIndex + 1).toInt();
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
        int colonIndex = CMD.indexOf(':');
        if (colonIndex > 0)
        {
            pilot_data.throttle = CMD.substring(0, colonIndex).toInt();
            pilot_data.steering = CMD.substring(colonIndex + 1).toInt();
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
            car_output.throttle = map(rc_data.throttle - 1493, 888 - 1493, 2149 - 1493, -100, 100);
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
            car_output.throttle = map(rc_data.throttle - 1493, 888 - 1493, 2149 - 1493, -100, 100);
        }
        car_output.steering = map(rc_data.steering - 1488, 872 - 1488, 2113 - 1488, -100, 100);

        if (counter % 2 == 0) // check per 5 loops to save time amonge pulseIn()
        {
            // #ifdef ENABLE_GAMEPAD_MODE
            //   sendGamepadPacket();
            // #else
              Serial.printf("T%d:S%d\n", car_output.throttle, car_output.steering);  // RC => Pilot
              Serial1.printf("T%d:S%d\n", car_output.throttle, car_output.steering); // RC => Type-C
            // #endif
        }
    }

    // if (counter % 100 == 0) // check per 100 loops to save time amonge pulseIn()
    // {
    //   Serial.printf("M%d:P%d\n", car_output.mode, car_output.park); // RC => Pilot
    //   Serial1.printf("M%d:P%d\n", car_output.mode, car_output.park); // RC => Type-C
    // }

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
    // 状态机逻辑已整合到emergencyStop函数中

    // CAR => PWM
    // if (isEmergencyStopping == false)
    // {
    //     car_output.throttle = constrain(car_output.throttle, -40, 100);
    // }

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