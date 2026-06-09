#pragma once
#include <Arduino.h>

uint16_t medianFilter(uint16_t* buf, int size);
bool isAuxiliaryRcChannel(int ch);
bool isPrimaryRcChannel(int ch);
uint16_t smoothPrimaryPWM(int ch, uint16_t value, bool valid);
uint16_t stabilizeAuxiliaryPWM(int ch, uint16_t value, bool valid);
