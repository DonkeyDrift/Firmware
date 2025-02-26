#include "ina219.h"
#include <memory.h>

INA219::INA219(I2C* channel, int addr) {
  this->Channel = channel->CreateDevice(addr);
}

void INA219::Init() {
  int mode = 7;
  int adc = 8;
  int pg = 3;

  this->Channel->WriteWord(0,  mode | (adc << 3) | (adc << 7) | (pg << 11));
}

void INA219::Calibrate(float a, float r) {
  auto v = 0.04096 * 32768/ r / a; 

  this->Channel->WriteWord(5, (short)v);
}

void INA219::Update() {
  this->V = this->Channel->ReadWord(2) / 2 / 1000.0;
  this->A = this->Channel->ReadWord(1) / 100.0 / 0.01;

/*
  printf("%d: %.4x\n", 0, this->Channel->ReadWord(0));
  printf("%d: %.2f mA\n", 1, A);
  printf("%d: %.2f V\n", 2, V);
  printf("%d: %d\n", 3, this->Channel->ReadWord(3));
  printf("%d: %d\n", 3, this->Channel->ReadWord(4));
*/
}