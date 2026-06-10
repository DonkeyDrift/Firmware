#pragma once
#include <Arduino.h>
#include "FirmwareConfig.h"

typedef void (*Mus4LogSink)(const char* source, const String& line);

extern uint8_t mus4LogTarget;

void mus4SetWebLogSink(Mus4LogSink sink);
void setMus4LogTargetWeb();
void mus4LogLine(const char* source, const String& line);
void mus4Logf(const char* source, const char* fmt, ...);
