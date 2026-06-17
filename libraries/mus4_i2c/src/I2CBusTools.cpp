#include "I2CBusTools.h"

#include <Wire.h>

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
