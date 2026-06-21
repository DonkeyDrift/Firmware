#pragma once
#include <Arduino.h>

void printLastI2CScanSummary();
void read_ina219();
void setup_ina219();
void read_mpu6050();
void scanI2CBus();
bool tryInitMPU6050OnCurrentBus(uint8_t *activeAddress, int maxRetriesPerAddress);
void setup_mpu6050();
