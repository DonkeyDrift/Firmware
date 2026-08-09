#pragma once
#include <Arduino.h>
#include <FastLED.h>

void setLEDColor(CRGB targetColor);
void setLEDToggle(CRGB color1, CRGB color2);
void setLEDToggle(CRGB color1, CRGB color2, CRGB color3);
void scanLEDToggle();
void runLedPowerOnSelfTest();
void applyLedBlinkMask(uint8_t mask);
