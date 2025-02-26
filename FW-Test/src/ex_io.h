#pragma once

#include "i2c.h"

struct IOTimer 
{
    uint32_t TriggerTime;  

};

class EXIO {
public:
  EXIO(I2C* channel, int addr);

  void Init();
  void Update();

  void Beep(bool on);
  void PowerOn();
  void PowerOff();
  void CPUPowerOn();
  void CPUPowerOoff();


private:
  const unsigned short BIT_BUZZER = 1;
  const unsigned short BIT_POWER_ON = 1 << 4;
  const unsigned short BIT_POWER_OFF = 1 << 5;
  const unsigned short BIT_CPU_POWER = 1 << 6;
  const unsigned short BIT_CPU_RESET = 1 << 7;

  unsigned short Output;
  unsigned short NewOutput;

  IOTimer BeepTimer;
  IOTimer PowerOnTimer;

  I2CDevice* Channel;

  void Write();
  void Read();
};