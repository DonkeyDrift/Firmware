#pragma once
#include <Arduino.h>

uint8_t parseHex2(const char* s);
uint8_t calcChecksum(const char* s, int n);
bool parsePilotCommandLine(const String& line, int* throttle, int* steering, int* seq);
bool parseAndValidateCommand(String cmd, int* throttle, int* steering);
