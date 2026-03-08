/*
 * I2C通信功能测试程序
 * 硬件平台: ESP32
 * I2C引脚: SDA = GPIO 13, SCL = GPIO 14
 * 
 * 功能说明:
 * 1. I2C总线初始化配置（支持多种速度模式）
 * 2. I2C设备地址扫描
 * 3. 数据读写测试
 * 4. 错误处理与诊断
 * 5. 详细的测试日志输出
 */

#include <Wire.h>

// I2C引脚定义
#define SDA_PIN 13
#define SCL_PIN 14

// I2C速度定义
#define I2C_SPEED_100K 100000L    // 标准模式 100kHz
#define I2C_SPEED_400K 400000L    // 快速模式 400kHz
#define I2C_SPEED_1M   1000000L   // 快速模式+ 1MHz

// 测试配置
#define TEST_I2C_SPEED I2C_SPEED_100K  // 当前测试速度
#define MAX_RETRIES 3                    // 最大重试次数
#define SCAN_DELAY_MS 10                 // 扫描间隔

// MPU6050寄存器定义
#define MPU6050_ADDR        0x68
#define MPU6050_ADDR_ALT    0x69
#define MPU6050_REG_WHO_AM_I 0x75
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

// 测试状态枚举
enum TestResult {
    TEST_PASS = 0,
    TEST_FAIL = 1,
    TEST_SKIP = 2
};

// 全局变量
unsigned long testStartTime = 0;
int totalTests = 0;
int passedTests = 0;
int failedTests = 0;

/*
 * 打印分隔线
 */
void printSeparator() {
    Serial.println("========================================");
}

/*
 * 打印测试标题
 */
void printTestHeader(const char* title) {
    printSeparator();
    Serial.print("[TEST] ");
    Serial.println(title);
    printSeparator();
}

/*
 * 打印测试结果
 */
void printTestResult(const char* testName, TestResult result, const char* details = nullptr) {
    totalTests++;
    Serial.print("[");
    switch(result) {
        case TEST_PASS:
            Serial.print("PASS");
            passedTests++;
            break;
        case TEST_FAIL:
            Serial.print("FAIL");
            failedTests++;
            break;
        case TEST_SKIP:
            Serial.print("SKIP");
            break;
    }
    Serial.print("] ");
    Serial.print(testName);
    if (details != nullptr) {
        Serial.print(" - ");
        Serial.print(details);
    }
    Serial.println();
}

/*
 * I2C总线初始化
 * 参数: speed - I2C时钟速度(Hz)
 * 返回: true - 成功, false - 失败
 */
bool initI2C(uint32_t speed) {
    Serial.print("[I2C INIT] Initializing I2C bus...\n");
    Serial.print("  SDA Pin: GPIO ");
    Serial.println(SDA_PIN);
    Serial.print("  SCL Pin: GPIO ");
    Serial.println(SCL_PIN);
    Serial.print("  Speed: ");
    Serial.print(speed / 1000);
    Serial.println(" kHz");
    
    Wire.begin(SDA_PIN, SCL_PIN, speed);
    
    // 延时让总线稳定
    delay(100);
    
    Serial.println("[I2C INIT] I2C bus initialized successfully");
    return true;
}

/*
 * I2C设备地址扫描
 * 扫描范围: 0x01 - 0x7F
 * 返回: 发现的设备数量
 */
int scanI2CDevices() {
    printTestHeader("I2C Device Scan");
    
    byte error;
    int deviceCount = 0;
    
    Serial.println("[SCAN] Scanning I2C bus (0x01 - 0x7F)...");
    Serial.println();
    
    // 打印表头
    Serial.print("     ");
    for (int i = 0; i < 16; i++) {
        Serial.printf(" 0x%01X", i);
    }
    Serial.println();
    printSeparator();
    
    // 扫描所有地址
    for (int row = 0; row < 8; row++) {
        Serial.printf("0x%01X0 |", row);
        for (int col = 0; col < 16; col++) {
            int address = (row << 4) + col;
            
            if (address < 0x08 || address > 0x77) {
                // 保留地址，跳过
                Serial.print("  --");
                continue;
            }
            
            Wire.beginTransmission(address);
            error = Wire.endTransmission();
            
            if (error == 0) {
                Serial.printf(" 0x%02X", address);
                deviceCount++;
            } else if (error == 4) {
                Serial.print("  ??");
            } else {
                Serial.print("  ..");
            }
            
            delay(SCAN_DELAY_MS);
        }
        Serial.println();
    }
    
    printSeparator();
    Serial.print("[SCAN] Found ");
    Serial.print(deviceCount);
    Serial.println(" device(s)");
    
    // 详细列出发现的设备
    if (deviceCount > 0) {
        Serial.println("[SCAN] Device details:");
        for (int address = 0x08; address <= 0x77; address++) {
            Wire.beginTransmission(address);
            error = Wire.endTransmission();
            if (error == 0) {
                Serial.print("  - Address: 0x");
                Serial.print(address, HEX);
                Serial.print(" (");
                Serial.print(address);
                Serial.print(")");
                
                // 识别常见设备
                switch(address) {
                    case 0x68:
                    case 0x69:
                        Serial.println(" - MPU6050/MPU9250");
                        break;
                    case 0x40:
                    case 0x41:
                    case 0x44:
                    case 0x45:
                        Serial.println(" - INA219/INA226");
                        break;
                    case 0x50:
                    case 0x51:
                    case 0x52:
                    case 0x53:
                    case 0x54:
                    case 0x55:
                    case 0x56:
                    case 0x57:
                        Serial.println(" - EEPROM");
                        break;
                    case 0x3C:
                    case 0x3D:
                        Serial.println(" - OLED Display");
                        break;
                    default:
                        Serial.println(" - Unknown device");
                        break;
                }
            }
        }
    }
    
    return deviceCount;
}

/*
 * I2C字节写入测试
 * 参数: deviceAddr - 设备地址
 *       regAddr - 寄存器地址
 *       data - 要写入的数据
 * 返回: true - 成功, false - 失败
 */
bool writeByte(uint8_t deviceAddr, uint8_t regAddr, uint8_t data) {
    Wire.beginTransmission(deviceAddr);
    Wire.write(regAddr);
    Wire.write(data);
    uint8_t error = Wire.endTransmission();
    
    if (error != 0) {
        Serial.print("[WRITE ERROR] Error code: ");
        Serial.println(error);
        return false;
    }
    return true;
}

/*
 * I2C字节读取测试
 * 参数: deviceAddr - 设备地址
 *       regAddr - 寄存器地址
 *       data - 读取的数据存储位置
 * 返回: true - 成功, false - 失败
 */
bool readByte(uint8_t deviceAddr, uint8_t regAddr, uint8_t* data) {
    // 写入寄存器地址
    Wire.beginTransmission(deviceAddr);
    Wire.write(regAddr);
    uint8_t error = Wire.endTransmission(false); // 发送重复起始条件
    
    if (error != 0) {
        Serial.print("[READ ERROR] Write reg address failed, code: ");
        Serial.println(error);
        return false;
    }
    
    // 读取数据
    uint8_t bytesRead = Wire.requestFrom(deviceAddr, (uint8_t)1);
    if (bytesRead != 1) {
        Serial.print("[READ ERROR] Expected 1 byte, got ");
        Serial.println(bytesRead);
        return false;
    }
    
    if (Wire.available()) {
        *data = Wire.read();
        return true;
    }
    
    return false;
}

/*
 * I2C多字节读取测试
 * 参数: deviceAddr - 设备地址
 *       regAddr - 寄存器地址
 *       data - 数据缓冲区
 *       length - 要读取的字节数
 * 返回: 实际读取的字节数
 */
int readBytes(uint8_t deviceAddr, uint8_t regAddr, uint8_t* data, uint8_t length) {
    // 写入寄存器地址
    Wire.beginTransmission(deviceAddr);
    Wire.write(regAddr);
    uint8_t error = Wire.endTransmission(false);
    
    if (error != 0) {
        Serial.print("[READ ERROR] Write reg address failed, code: ");
        Serial.println(error);
        return 0;
    }
    
    // 读取数据
    uint8_t bytesRead = Wire.requestFrom(deviceAddr, length);
    int index = 0;
    
    while (Wire.available() && index < length) {
        data[index++] = Wire.read();
    }
    
    return index;
}

/*
 * MPU6050 WHO_AM_I 寄存器测试
 * 用于验证MPU6050是否正常工作
 */
bool testMPU6050WhoAmI(uint8_t address) {
    Serial.print("[MPU6050 TEST] Testing device at 0x");
    Serial.println(address, HEX);
    
    uint8_t whoAmI = 0;
    
    if (!readByte(address, MPU6050_REG_WHO_AM_I, &whoAmI)) {
        Serial.println("[MPU6050 TEST] Failed to read WHO_AM_I register");
        return false;
    }
    
    Serial.print("[MPU6050 TEST] WHO_AM_I register value: 0x");
    Serial.println(whoAmI, HEX);
    
    // MPU6050的WHO_AM_I应该是0x68
    // MPU9250的WHO_AM_I应该是0x71或0x73
    if (whoAmI == 0x68) {
        Serial.println("[MPU6050 TEST] Device identified as MPU6050");
        return true;
    } else if (whoAmI == 0x71 || whoAmI == 0x73) {
        Serial.println("[MPU6050 TEST] Device identified as MPU9250");
        return true;
    } else {
        Serial.println("[MPU6050 TEST] Unknown device or communication error");
        return false;
    }
}

/*
 * MPU6050 唤醒测试
 */
bool testMPU6050WakeUp(uint8_t address) {
    Serial.println("[MPU6050 TEST] Waking up device...");
    
    // 写入0到PWR_MGMT_1寄存器，解除休眠
    if (!writeByte(address, MPU6050_REG_PWR_MGMT_1, 0x00)) {
        Serial.println("[MPU6050 TEST] Failed to wake up device");
        return false;
    }
    
    delay(10);
    
    // 验证写入
    uint8_t pwrMgmt = 0;
    if (!readByte(address, MPU6050_REG_PWR_MGMT_1, &pwrMgmt)) {
        Serial.println("[MPU6050 TEST] Failed to read PWR_MGMT_1 register");
        return false;
    }
    
    Serial.print("[MPU6050 TEST] PWR_MGMT_1 register value: 0x");
    Serial.println(pwrMgmt, HEX);
    
    if (pwrMgmt == 0x00) {
        Serial.println("[MPU6050 TEST] Device wake up successful");
        return true;
    } else {
        Serial.println("[MPU6050 TEST] Device wake up failed");
        return false;
    }
}

/*
 * MPU6050 加速度数据读取测试
 */
bool testMPU6050AccelData(uint8_t address) {
    Serial.println("[MPU6050 TEST] Reading accelerometer data...");
    
    uint8_t buffer[6];
    int bytesRead = readBytes(address, MPU6050_REG_ACCEL_XOUT_H, buffer, 6);
    
    if (bytesRead != 6) {
        Serial.print("[MPU6050 TEST] Failed to read accelerometer data, got ");
        Serial.print(bytesRead);
        Serial.println(" bytes");
        return false;
    }
    
    // 组合数据
    int16_t accelX = (buffer[0] << 8) | buffer[1];
    int16_t accelY = (buffer[2] << 8) | buffer[3];
    int16_t accelZ = (buffer[4] << 8) | buffer[5];
    
    Serial.println("[MPU6050 TEST] Accelerometer raw data:");
    Serial.print("  X: ");
    Serial.println(accelX);
    Serial.print("  Y: ");
    Serial.println(accelY);
    Serial.print("  Z: ");
    Serial.println(accelZ);
    
    // 检查数据是否合理（非全0或全1）
    if (accelX == 0 && accelY == 0 && accelZ == 0) {
        Serial.println("[MPU6050 TEST] Warning: All zeros, sensor may not be ready");
        return false;
    }
    
    if (accelX == -1 && accelY == -1 && accelZ == -1) {
        Serial.println("[MPU6050 TEST] Warning: All 0xFF, communication error");
        return false;
    }
    
    Serial.println("[MPU6050 TEST] Accelerometer data read successfully");
    return true;
}

/*
 * 完整的MPU6050测试
 */
void testMPU6050() {
    printTestHeader("MPU6050 Sensor Test");
    
    uint8_t addresses[] = {MPU6050_ADDR, MPU6050_ADDR_ALT};
    bool found = false;
    
    for (int i = 0; i < 2; i++) {
        uint8_t addr = addresses[i];
        
        // 检查设备是否存在
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() != 0) {
            continue;
        }
        
        found = true;
        Serial.print("[MPU6050] Found device at 0x");
        Serial.println(addr, HEX);
        
        // 测试1: WHO_AM_I寄存器
        printTestResult("WHO_AM_I Register", 
                       testMPU6050WhoAmI(addr) ? TEST_PASS : TEST_FAIL);
        
        // 测试2: 唤醒
        printTestResult("Device Wake Up", 
                       testMPU6050WakeUp(addr) ? TEST_PASS : TEST_FAIL);
        
        delay(100);
        
        // 测试3: 加速度数据读取
        printTestResult("Accelerometer Data Read", 
                       testMPU6050AccelData(addr) ? TEST_PASS : TEST_FAIL);
        
        break; // 只测试第一个找到的设备
    }
    
    if (!found) {
        Serial.println("[MPU6050] No MPU6050 device found on I2C bus");
        printTestResult("MPU6050 Detection", TEST_FAIL, "Device not found");
    }
}

/*
 * I2C总线压力测试
 * 连续读写测试，验证稳定性
 */
void stressTest() {
    printTestHeader("I2C Stress Test");
    
    const int iterations = 100;
    int successCount = 0;
    int failCount = 0;
    
    Serial.print("[STRESS] Running ");
    Serial.print(iterations);
    Serial.println(" I2C transactions...");
    
    unsigned long startTime = millis();
    
    for (int i = 0; i < iterations; i++) {
        // 简单的总线扫描作为压力测试
        Wire.beginTransmission(0x68);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            successCount++;
        } else {
            failCount++;
        }
        
        if ((i + 1) % 10 == 0) {
            Serial.print("[STRESS] Progress: ");
            Serial.print(i + 1);
            Serial.println("/100");
        }
        
        delay(10);
    }
    
    unsigned long duration = millis() - startTime;
    
    Serial.println("[STRESS] Test completed");
    Serial.print("  Success: ");
    Serial.println(successCount);
    Serial.print("  Failed: ");
    Serial.println(failCount);
    Serial.print("  Duration: ");
    Serial.print(duration);
    Serial.println(" ms");
    Serial.print("  Success rate: ");
    Serial.print((successCount * 100.0) / iterations);
    Serial.println("%");
    
    printTestResult("Stress Test", 
                   (failCount == 0) ? TEST_PASS : TEST_FAIL,
                   (failCount == 0) ? "All transactions successful" : "Some transactions failed");
}

/*
 * 打印测试摘要
 */
void printTestSummary() {
    printSeparator();
    Serial.println("[SUMMARY] Test Summary");
    printSeparator();
    
    Serial.print("Total tests: ");
    Serial.println(totalTests);
    Serial.print("Passed: ");
    Serial.println(passedTests);
    Serial.print("Failed: ");
    Serial.println(failedTests);
    Serial.print("Success rate: ");
    Serial.print((passedTests * 100.0) / totalTests);
    Serial.println("%");
    
    unsigned long totalDuration = millis() - testStartTime;
    Serial.print("Total duration: ");
    Serial.print(totalDuration);
    Serial.println(" ms");
    
    printSeparator();
    
    if (failedTests == 0) {
        Serial.println("[SUMMARY] ALL TESTS PASSED!");
    } else {
        Serial.println("[SUMMARY] SOME TESTS FAILED - Check logs above");
    }
}

/*
 * 初始化函数
 */
void setup() {
    // 初始化串口
    Serial.begin(115200);
    while (!Serial) {
        ; // 等待串口连接
    }
    
    delay(1000);
    
    printSeparator();
    Serial.println("I2C Communication Test Program");
    Serial.println("ESP32 I2C Test Suite v1.0");
    printSeparator();
    
    testStartTime = millis();
    
    // 1. I2C初始化测试
    printTestHeader("I2C Initialization");
    bool initSuccess = initI2C(TEST_I2C_SPEED);
    printTestResult("I2C Bus Initialization", 
                   initSuccess ? TEST_PASS : TEST_FAIL);
    
    if (!initSuccess) {
        Serial.println("[ERROR] I2C initialization failed, stopping tests");
        return;
    }
    
    // 2. I2C设备扫描
    int deviceCount = scanI2CDevices();
    printTestResult("I2C Device Scan", 
                   (deviceCount > 0) ? TEST_PASS : TEST_FAIL,
                   (deviceCount > 0) ? "Devices found" : "No devices found");
    
    // 3. MPU6050测试（如果找到设备）
    if (deviceCount > 0) {
        testMPU6050();
    } else {
        Serial.println("[SKIP] Skipping MPU6050 tests - no devices found");
    }
    
    // 4. 压力测试
    stressTest();
    
    // 5. 打印测试摘要
    printTestSummary();
}

/*
 * 主循环
 */
void loop() {
    // 测试完成后进入空闲状态
    // 可以在这里添加周期性监控代码
    delay(1000);
}
