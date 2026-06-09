#pragma once
#include <Arduino.h>

bool I2CRead(uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t *Data);
uint16_t I2CReadValue(uint8_t addr, uint8_t reg);
void I2CWriteValue(uint8_t Address, uint8_t Register, uint16_t Data);
const char *identifyI2CDeviceByAddress(uint8_t address);
bool I2CReadRegister8(uint8_t address, uint8_t reg, uint8_t *value);
bool probeMPU6050AtAddress(uint8_t address, uint8_t *whoAmI);
