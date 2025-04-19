//=============================================================v1.1-2025-04-19
/* Note of this version:
1. 启用LED切换与闪烁

Experience
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
// #include <Adafruit_MPU6050.h>
// #include <Adafruit_Sensor.h>

#define LED_PIN 5
#define NUM_LEDS 1
#define BRIGHTNESS 64
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

#define BUAD_RATE_0 115200
#define RX_1_PIN 16
#define TX_1_PIN 17
#define BUAD_RATE_1 115200
#define SDA_PIN 13
#define SCL_PIN 14
#define I2C_SPEED 400000L

CRGB leds[NUM_LEDS]; // Define the array of leds

// 新增全局变量
bool toggleActive = false;
CRGB toggleColor1, toggleColor2;
unsigned long toggleTime = 0;
unsigned long toggleInterval = 250; // LED切换间隔为250ms

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

// Adafruit_MPU6050 mpu; // Create an MPU6050 object

// #define DEBUG // Uncomment to enable debugging output

#define CH1_PIN 27 // 接收机pwm输入CH1通道
#define CH2_PIN 26 // 接收机pwm输入CH2通道
#define CH3_PIN 35 // 接收机pwm输入CH3通道
#define CH4_PIN 34 // 接收机pwm输入CH4通道

#define STEERING_PIN 32 // PIN of Servo
#define THROTTLE_PIN 33 // PIN of ESC

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
const int SERVO_MID = 1270;  // 需要实际测试
const int SERVO_RANGE = 390; // Pulse range for Motor Throttle
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
    if (pwm_value[CH_PARK] > 1500)
    {
        rc_data.park = false;
    }
    else
    {
        rc_data.park = true;
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
    Serial.printf("%d:%f mA\r\n", 1, I2CReadValue(0x41, 1) / 100.0 / 0.01);
    Serial.printf("%d:%f V\r\n", 2, I2CReadValue(0x41, 2) / 2 / 1000.0);
}

// void read_mpu6050()
// {
//     /* Get new sensor events with the readings */
//     sensors_event_t a, g, temp;
//     mpu.getEvent(&a, &g, &temp);

//     /* Print out the values */
//     Serial.print("Acceleration X: ");
//     Serial.print(a.acceleration.x);
//     Serial.print(", Y: ");
//     Serial.print(a.acceleration.y);
//     Serial.print(", Z: ");
//     Serial.print(a.acceleration.z);
//     Serial.println(" m/s^2");

//     Serial.print("Rotation X: ");
//     Serial.print(g.gyro.x);
//     Serial.print(", Y: ");
//     Serial.print(g.gyro.y);
//     Serial.print(", Z: ");
//     Serial.print(g.gyro.z);
//     Serial.println(" rad/s");

//     Serial.print("Temperature: ");
//     Serial.print(temp.temperature);
//     Serial.println(" degC");

//     Serial.println("");
// }

// void setup_mpu6050()
// {
//     // Try to initialize!
//     if (!mpu.begin())
//     {
//         Serial.println("Failed to find MPU6050 chip");
//         while (1)
//         {
//             delay(10);
//         }
//     }
//     Serial.println("MPU6050 Found!");

//     // set accelerometer Range
//     mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
//     Serial.print("Accelerometer range set to: ");

//     switch (mpu.getAccelerometerRange())
//     {
//     case MPU6050_RANGE_2_G:
//         Serial.println("+-2G");
//         break;
//     case MPU6050_RANGE_4_G:
//         Serial.println("+-4G");
//         break;
//     case MPU6050_RANGE_8_G:
//         Serial.println("+-8G");
//         break;
//     case MPU6050_RANGE_16_G:
//         Serial.println("+-16G");
//         break;
//     }

//     // set Gyro Range
//     mpu.setGyroRange(MPU6050_RANGE_500_DEG);
//     Serial.print("Gyro range set to: ");
//     switch (mpu.getGyroRange())
//     {
//     case MPU6050_RANGE_250_DEG:
//         Serial.println("+- 250 deg/s");
//         break;
//     case MPU6050_RANGE_500_DEG:
//         Serial.println("+- 500 deg/s");
//         break;
//     case MPU6050_RANGE_1000_DEG:
//         Serial.println("+- 1000 deg/s");
//         break;
//     case MPU6050_RANGE_2000_DEG:
//         Serial.println("+- 2000 deg/s");
//         break;
//     }

//     // Set filter bandwidth
//     mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
//     Serial.print("Filter bandwidth set to: ");
//     switch (mpu.getFilterBandwidth())
//     {
//     case MPU6050_BAND_260_HZ:
//         Serial.println("260 Hz");
//         break;
//     case MPU6050_BAND_184_HZ:
//         Serial.println("184 Hz");
//         break;
//     case MPU6050_BAND_94_HZ:
//         Serial.println("94 Hz");
//         break;
//     case MPU6050_BAND_44_HZ:
//         Serial.println("44 Hz");
//         break;
//     case MPU6050_BAND_21_HZ:
//         Serial.println("21 Hz");
//         break;
//     case MPU6050_BAND_10_HZ:
//         Serial.println("10 Hz");
//         break;
//     case MPU6050_BAND_5_HZ:
//         Serial.println("5 Hz");
//         break;
//     }
// }

void setup()
{
    Serial.begin(BUAD_RATE_0);                                  // TypeC
    Serial1.begin(BUAD_RATE_1, SERIAL_8N1, RX_1_PIN, TX_1_PIN); // RS232: rx = 16, tx = 17
    Serial.println("ESP32 Receiver Serial Ready!");
    Serial1.println("ESP32 Receiver Serial1 Ready!");

    // Wire.begin(SDA_PIN, SCL_PIN, I2C_SPEED); // SDA = 13, SCL = 14, 400kHz
    // setup_mpu6050();
    // delay(100);

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
    delay(1000);
}

void loop()
{
    //   read_ina219();
    //   read_mpu6050();

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
            car_output.throttle = 0;
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
    }
    else if (car_output.mode == CAR_MODE_SEMI_AUTO)
    {
        // Controlled by both RC and Pilot
        if (car_output.park == 1)
        {
            car_output.throttle = 0;
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
            car_output.throttle = 0;
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
            Serial.printf("T%d:S%d\n", car_output.throttle, car_output.steering);  // RC => Pilot
            Serial1.printf("T%d:S%d\n", car_output.throttle, car_output.steering); // RC => Type-C
        }
    }

    // if (counter % 100 == 0) // check per 100 loops to save time amonge pulseIn()
    // {
    //   Serial.printf("M%d:P%d\n", car_output.mode, car_output.park); // RC => Pilot
    //   Serial1.printf("M%d:P%d\n", car_output.mode, car_output.park); // RC => Type-C
    // }

    // CAR => PWM
    int pwm_steering = map(car_output.steering, -100, 100, SERVO_MID - SERVO_RANGE, SERVO_MID + SERVO_RANGE);
    int pwm_throttle = map(car_output.throttle, -100, 100, MOTOR_MID - MOTOR_RANGE, MOTOR_MID + MOTOR_RANGE);

    pwm_steering = min(max(pwm_steering, PWM_MIN), PWM_MAX);
    pwm_throttle = min(max(pwm_throttle, PWM_MIN), PWM_MAX);

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

    ledcWriteChannel(CH_STEERING, pwm_steering);
    ledcWriteChannel(CH_THROTTLE, pwm_throttle);

    delay(10);
    counter += 1;

    scanLEDToggle();
}
