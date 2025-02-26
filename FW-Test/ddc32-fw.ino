#include "src/i2c.h"
#include "src/ex_io.h"
#include "src/ws2812.h"
#include "src/ina219.h"
#include "src/mpu6050.h"

const int Colors[] = { 255, 0, 0, 0, 255, 0, 0, 0, 255 };
int color_index = 0;

WS2812 RGB;
I2C I2CChannel(GPIO_NUM_22, GPIO_NUM_21);
EXIO ExtendIO(&I2CChannel, 0x74);
INA219 Sensor1(&I2CChannel, 0x40);
INA219 Sensor2(&I2CChannel, 0x45);
MPU6050 IMU(&I2CChannel, 0x68);

void setup() {

  printf("Extend IO Init\n");
  ExtendIO.Init();
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
  printf("Sensor1 Data:\n");
  Sensor1.Update();
  printf("Sensor2 Data:\n");
  Sensor2.Update();
  IMU.Update();

  RGB.Send(Colors[color_index], Colors[color_index + 1], Colors[color_index + 2]);
  color_index += 3;
  if (color_index >= 9) color_index = 0;

  delay(1000);
}
