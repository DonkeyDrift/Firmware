#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE

// Initialize the web log ring buffer. Call once before any append/read.
void webLogBufferInit();

// Append a single log line to the web log ring buffer.
void appendWebLog(const char* source, const String& line);

// Split a multi-line text and append each non-empty line to the buffer.
void appendWebLogLines(const char* source, const String& text);

// Return the number of dropped entries due to buffer overflow.
uint32_t webLogBufferDropped();

// Append a JSON representation of the log buffer (entries newer than `since`)
// to the provided String. The output includes a "dropped" counter and an
// "entries" array.
void writeWebLogsJson(String& response, uint32_t since);

#endif
