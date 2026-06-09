#pragma once
#include <Arduino.h>
#include "FirmwareConfig.h"

// Sensor Data Structure
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
};

// Control Data Structure (matching struct_message in mus4.ino logic)
struct ControlData {
    int throttle;
    int steering;
    int mode;
    bool park;
};

