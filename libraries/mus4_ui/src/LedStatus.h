#pragma once
#include <Arduino.h>
#include <FastLED.h>

void setLEDColor(CRGB targetColor);
void setLEDToggle(CRGB color1, CRGB color2);
void scanLEDToggle();
