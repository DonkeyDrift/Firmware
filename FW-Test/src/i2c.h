#pragma once

#include <driver/i2c_master.h>

#define GET_RINT(p, i) (short)(((unsigned short)p[i]) << 8 | p[i + 1]);
#define GET_INT(p, i) (short)(((unsigned short)p[i + 1]) << 8 | p[i]);

class I2CDevice;

class I2C {
public:
  I2C(int scl, int sda);

  I2CDevice* CreateDevice(int addr);
  void Scan();

private:
  const i2c_port_t CHANNEL_NUM = I2C_NUM_0;
  const bool ACK_CHECK_EN = true;

  i2c_master_bus_handle_t Handle;
};

class I2CDevice {
public:
  I2CDevice(i2c_master_dev_handle_t handle);

  bool ReadRegs(int Register, int Count, unsigned char* Data);

  bool WriteByte(int Register, unsigned char Data);
  bool WriteWord(int Register, unsigned short Data);
  short ReadWord(int Register);
  unsigned char ReadByte(int Register);

private:
  i2c_master_dev_handle_t Handle;
};
