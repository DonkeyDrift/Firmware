#include "Sensors.h"

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "FirmwareConfig.h"
#include "I2CBusTools.h"
#include "Mus4Log.h"
#include "SharedTypes.h"

extern Adafruit_INA219 ina219;
extern Adafruit_MPU6050 mpu;
extern SensorData ina219Data;
extern SensorData mpu6050Data;
extern uint8_t g_mpuCandidateAddress;
extern uint8_t g_mpuWhoAmIValue;
extern uint32_t g_i2cWorkingSpeed;
extern uint8_t g_i2cScanAddresses[16];
extern uint8_t g_i2cScanCount;

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
