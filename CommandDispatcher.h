#pragma once
#include <Arduino.h>

#include "SerialBufferTypes.h"

bool dispatchCommandLine(const String& line, Print& out, SerialBuf& sb);
