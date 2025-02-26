#include "I2C.h"

#include "freertos/FreeRTOS.h"
#include "esp_err.h"

I2C::I2C(int scl, int sda) {
  i2c_master_bus_config_t config = {
    .i2c_port = CHANNEL_NUM,
    .sda_io_num = (gpio_num_t)sda,
    .scl_io_num = (gpio_num_t)scl,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
  };
  config.flags.enable_internal_pullup = true;

  ESP_ERROR_CHECK(i2c_new_master_bus(&config, &this->Handle));
}

I2CDevice* I2C::CreateDevice(int addr) {
  i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = (unsigned short)addr,
    .scl_speed_hz = 400000,
  };

  i2c_master_dev_handle_t dev_handle;
  ESP_ERROR_CHECK(i2c_master_bus_add_device(this->Handle, &dev_cfg, &dev_handle));

  return new I2CDevice(dev_handle);
}

void I2C::Scan() {
  printf("I2C Scan\n");
  printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
  printf("00:            ");

  for (uint8_t i = 4; i < 0x78; i++) {
    if (i % 16 == 0) printf("\n%.2x:", i);

    auto res = i2c_master_probe(this->Handle, i, 10);

    if (res == ESP_OK)
      printf(" %.2x", i);
    else
      printf(" --");

    vTaskDelay(pdMS_TO_TICKS(10));
  }

  printf("\n\n");
}


I2CDevice::I2CDevice(i2c_master_dev_handle_t handle) {
  this->Handle = handle;
}

bool I2CDevice::ReadRegs(int Register, int Count, unsigned char* Data) {
  bool ret = true;

  uint8_t reg_addr = (uint8_t)Register;

  auto res = i2c_master_transmit_receive(this->Handle, &reg_addr, 1, Data, Count, 10);
  if(res != ESP_OK) {
    printf("I2C Read Regs Error\n");
    ret = false;
  }

  return ret;
}

bool I2CDevice::WriteByte(int Register, unsigned char Data) {
  bool ret = true;

  uint8_t buf[2];
  buf[0] = Register;
  buf[1] = Data;
  auto res = i2c_master_transmit(this->Handle, buf, 2, 10);
  if(res != ESP_OK) {
    printf("I2C Write Byte Error\n");
    ret = false;
  }

  return ret;
}

bool I2CDevice::WriteWord(int Register, unsigned short Data) {
  bool ret = true;

  uint8_t buf[3];
  buf[0] = Register;
  buf[1] = (Data >> 8);
  buf[2] = (Data & 0xff);

  auto res = i2c_master_transmit(this->Handle, buf, 3, 10);
  if(res != ESP_OK) {
    printf("I2C Write Word Error\n");
    ret = false;
  }

  return ret;
}

short I2CDevice::ReadWord(int Register) {
  short ret = -1;
  uint8_t data[2];

  if(this->ReadRegs(Register, 2, data)) {
    ret = (short)(((int)data[0] << 8) + data[1]);
  }

  return ret;
}

unsigned char I2CDevice::ReadByte(int Register) {
  unsigned char ret = 0x5a;

  this->ReadRegs(Register, 1, &ret);

  return ret;  
}

