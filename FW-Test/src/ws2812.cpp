#include "ws2812.h"
#include <memory.h>
#include "esp_err.h"

#define SPI_CLOCK_HZ 8000000  // SPI 时钟频率
#define PIN_NO_USE -1

WS2812::WS2812() {
  this->Handle = NULL;
}

void WS2812::Init(int pin) {
  spi_bus_config_t buscfg = {
    .mosi_io_num = pin,
    .miso_io_num = PIN_NO_USE,
    .sclk_io_num = PIN_NO_USE,
    .quadwp_io_num = PIN_NO_USE,
    .quadhd_io_num = PIN_NO_USE,
    //.max_transfer_sz = 8000,
  };

  spi_device_interface_config_t devcfg = {
    .mode = 0,
    .clock_speed_hz = SPI_CLOCK_HZ,
    .spics_io_num = PIN_NO_USE,
    .queue_size = 7,
    .pre_cb = NULL,
    .post_cb = NULL,
  };

  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, 1));
  
  ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &this->Handle));

  this->Reset();
}

void WS2812::Reset() {
  unsigned char buf[64];
  memset(buf, 0, 64);

  this->SPISend(buf, 64);
}

void WS2812::Send(int r, int g, int b) {
  this->SPISend(this->SPIData(r, g, b), 24);
}

void WS2812::SPISend(unsigned char* data, int len) {
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));

  t.length = len * 8;
  t.tx_buffer = data;

  auto res = spi_device_transmit(this->Handle, &t);
  if(res != ESP_OK) {
    printf("SPI Send Error\n");
  }
}

unsigned char* WS2812::SPIData(int r, int g, int b) {
  static uint8_t spi_data[24];

  for (int i = 0; i < 8; i++) {
    spi_data[i] = (g & 0x80) ? CODE_BIT1 : CODE_BIT0;
    spi_data[i + 8] = (r & 0x80) ? CODE_BIT1 : CODE_BIT0;
    spi_data[i + 16] = (b & 0x80) ? CODE_BIT1 : CODE_BIT0;

    r <<= 1;
    g <<= 1;
    b <<= 1;
  }

  return spi_data;
}

