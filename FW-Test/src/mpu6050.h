#pragma once

#include "i2c.h"

class MPU6050 {
public:
  MPU6050(I2C* channel, int addr);

  void Init();

  void Reset();
  void ReadID();
  void Update();

  short Data[7];

private:
  void Read();

  I2CDevice* Channel;
};