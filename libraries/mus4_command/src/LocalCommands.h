#pragma once
#include <Arduino.h>

bool processLine(const String& line, int* throttle, int* steering, int* seq);
