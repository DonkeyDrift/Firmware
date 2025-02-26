#include "mpu6050.h"
#include "mpu6050_reg.h"
#include "freertos/FreeRTOS.h"

MPU6050::MPU6050(I2C* channel, int addr) {
  this->Channel = channel->CreateDevice(addr);
}

void MPU6050::Read() {
  unsigned char buff[25];

  auto ID = this->Channel->ReadByte(REG_6050::PWR_MGMT_1);
  auto gf = this->Channel->ReadByte(REG_6050::GYRO_CONFIG);  
  auto af = this->Channel->ReadByte(REG_6050::ACCEL_CONFIG); 
  printf("id: %02x, af: %d, gf: %d\n", ID, af, gf);

  // Read accelerometer and gyroscope

  /*
  if (this->Channel->ReadRegs(REG_6050::ACCEL_XOUT_H, 14, buff)) {
    for(int i = 0; i < 14; i++) {
      buff[i] = this->Channel->ReadByte(REG_6050::ACCEL_XOUT_H + i);
    }

    unsigned char* p = buff;
    
    for (int i = 0; i < 3; i++) {
      // ax, ay, az
      this->Data[i] = (short)((p[i * 2]) << 8 | p[i * 2 + 1]);
      // Gyroscope
      this->Data[i + 3] = (short)((p[i * 2 + 8]) << 8 | p[i * 2 + 9]);
    }
  }
  */

  this->Data[0] = this->Channel->ReadWord(REG_6050::ACCEL_XOUT_H);
  this->Data[1] = this->Channel->ReadWord(REG_6050::ACCEL_YOUT_H);
  this->Data[2] = this->Channel->ReadWord(REG_6050::ACCEL_ZOUT_H);
  this->Data[3] = this->Channel->ReadWord(REG_6050::GYRO_XOUT_H);
  this->Data[4] = this->Channel->ReadWord(REG_6050::GYRO_YOUT_H);
  this->Data[5] = this->Channel->ReadWord(REG_6050::GYRO_ZOUT_H);
  this->Data[6] = this->Channel->ReadWord(REG_6050::TEMP_OUT_H);

}

void MPU6050::Init() {
  this->Reset();

  this->Channel->WriteByte(REG_6050::SMPLRT_DIV, 0x07);            
  this->Channel->WriteByte(REG_6050::CONFIG, REG_6050::LOW_PASS_FILTER_6);            // Set accelerometers low pass filter at 5Hz
  this->Channel->WriteByte(REG_6050::ACCEL_CONFIG2, REG_6050::LOW_PASS_FILTER_6);     // Set gyroscope low pass filter at 5Hz
  this->Channel->WriteByte(REG_6050::GYRO_CONFIG, REG_6050::GYROSCOPE_RANGE_250DPS);  // Configure gyroscope range
  this->Channel->WriteByte(REG_6050::ACCEL_CONFIG, REG_6050::ACCELEROMETER_RANGE_2G); // Configure accelerometers range
  this->Channel->WriteByte(REG_6050::INT_ENABLE, REG_6050::BOOL_FALSE);               // Disable INT
  this->Channel->WriteByte(REG_6050::FIFO_EN, REG_6050::BOOL_FALSE);                  // Disable FIFO
}

void MPU6050::Reset() {
  this->Channel->WriteByte(REG_6050::PWR_MGMT_1, 0x80);  // Reset mpu6050
  vTaskDelay(pdMS_TO_TICKS(200));
  this->Channel->WriteByte(REG_6050::SIGNAL_PATH_RESET, 0x03);  
  vTaskDelay(pdMS_TO_TICKS(200));
  this->Channel->WriteByte(REG_6050::PWR_MGMT_2, 0x00);  
  // this->Channel->WriteByte(REG_6050::PWR_MGMT_1, 0x01);  // Set clock of mpu6050
  this->Channel->WriteByte(REG_6050::SIGNAL_PATH_RESET, 0x00);  // 
}

void MPU6050::Update() {
  this->Read();
  printf("IMU Data: %d, %d, %d, %d, %d, %d, %d\n", this->Data[0], this->Data[1], this->Data[2], this->Data[3], this->Data[4], this->Data[5], this->Data[6]);
}

void MPU6050::ReadID() {
  unsigned char ID;

  ID = this->Channel->ReadByte(REG_6050::WHO_AM_I);

  printf("IMU ID: %02x\n", ID);
}
