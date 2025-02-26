#include "../src/i2c.h"
#include "../src/ex_io.h"
#include "../src/ws2812.h"
#include "../src/ina219.h"
#include "../src/mpu6050.h"
#include "../src/gpio.h"
#include "../src/timer.h"
#include "freertos/FreeRTOS.h"

const int Colors[] = { 255, 0, 0, 0, 255, 0, 0, 0, 255 };
int color_index = 0;

WS2812 RGB;
I2C I2CChannel(GPIO_NUM_22, GPIO_NUM_21);
EXIO ExtendIO(&I2CChannel, 0x74);
INA219 Sensor1(&I2CChannel, 0x40);
INA219 Sensor2(&I2CChannel, 0x45);
MPU6050 IMU(&I2CChannel, 0x68);

GPIO PowerBtn(4, gpio_mode_t::GPIO_MODE_INPUT, GPIO_PULL::Up);


void setup() {

  printf("Extend IO Init\n");
  ExtendIO.Init();
  ExtendIO.PowerOn();
  
  printf("RGB Init\n");
  RGB.Init(GPIO_NUM_5);
  Sensor1.Init();
  Sensor1.Calibrate(32, 0.01);
  Sensor2.Init();
  Sensor2.Calibrate(20, 0.01);
  IMU.Init();
  IMU.ReadID();

  ExtendIO.Beep(true);
  I2CChannel.Scan();
}

void loop() {
  printf("Timer Value: %ld\n", Timer::ReadLo());

  Sensor1.Update();
  printf("Sensor1 Data: %.3fV, %.3fmA\n", Sensor1.V, Sensor1.A);
  Sensor2.Update();
  printf("Sensor2 Data: %.3fV, %.3fmA\n", Sensor2.V, Sensor2.A);
  //IMU.ReadID();
  //IMU.Update();
  ExtendIO.Update();

  RGB.Send(Colors[color_index], Colors[color_index + 1], Colors[color_index + 2]);
  color_index += 3;
  if (color_index >= 9) color_index = 0;

  printf("BTN: %d \n", PowerBtn.Get() ? 1 : 0);

  vTaskDelay(pdMS_TO_TICKS(500));
}


extern "C"
{
    void app_main();
}

void app_main()
{
    setup();
    while(true)
    {
        loop();
    }
}