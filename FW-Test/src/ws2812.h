#pragma once

#include "driver/spi_master.h"

class WS2812 {
public:
  WS2812();

  void Init(int pin);
  void Reset();
  void Send(int r, int g, int b);

private:
  const unsigned char CODE_BIT0 = 0xc0;
  const unsigned char CODE_BIT1 = 0xf8;

  spi_device_handle_t Handle;

  unsigned char* SPIData(int r, int g, int b);
  void SPISend(unsigned char* data, int len);
};