#pragma once

#include "i2c.h"

class INA219 {
public:
  INA219(I2C* channel, int addr);

  void Init();
  void Calibrate(float a, float r);
  
  void Update();

  float V;
  float A;

private:
  int Address;
  I2CDevice* Channel;
};