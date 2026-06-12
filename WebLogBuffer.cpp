#include "WebLogBuffer.h"

#ifdef ENABLE_WIFI_CONSOLE

#include "JsonUtil.h"
#include "WifiConsoleTypes.h"

#include <string.h>

static const char* canonicalWebLogSource(const char* source)
{
    if (strcmp(source, "serial") == 0 || strcmp(source, "serial1") == 0) {
        return source;
    }
    return "web";
}

static WebLogEntry s_webLogEntries[WIFI_WEB_LOG_CAPACITY];
static uint32_t s_webLogSeq = 0;
static uint32_t s_webLogDropped = 0;
static uint8_t s_webLogHead = 0;
static uint8_t s_webLogCount = 0;

void webLogBufferInit()
{
    s_webLogSeq = 0;
    s_webLogDropped = 0;
    s_webLogHead = 0;
    s_webLogCount = 0;
    for (uint8_t i = 0; i < WIFI_WEB_LOG_CAPACITY; i++) {
        s_webLogEntries[i] = WebLogEntry{};
    }
}

void appendWebLog(const char* source, const String& line)
{
    WebLogEntry& entry = s_webLogEntries[s_webLogHead];
    entry.seq = ++s_webLogSeq;
    entry.t = millis();
    snprintf(entry.source, sizeof(entry.source), "%s", canonicalWebLogSource(source));
    snprintf(entry.line, sizeof(entry.line), "%s", line.c_str());
    s_webLogHead = (s_webLogHead + 1) % WIFI_WEB_LOG_CAPACITY;
    if (s_webLogCount < WIFI_WEB_LOG_CAPACITY) {
        s_webLogCount++;
    } else {
        s_webLogDropped++;
    }
}

void appendWebLogLines(const char* source, const String& text)
{
    int start = 0;
    while (start < text.length()) {
        int end = text.indexOf('\n', start);
        if (end < 0) end = text.length();
        String line = text.substring(start, end);
        line.trim();
        if (line.length() > 0) appendWebLog(source, line);
        start = end + 1;
    }
}

uint32_t webLogBufferDropped()
{
    return s_webLogDropped;
}

void writeWebLogsJson(String& response, uint32_t since)
{
    response += "{\"dropped\":";
    response += s_webLogDropped;
    response += ",\"entries\":[";
    bool first = true;
    for (uint8_t i = 0; i < s_webLogCount; i++) {
        uint8_t index = (s_webLogHead + WIFI_WEB_LOG_CAPACITY - s_webLogCount + i) % WIFI_WEB_LOG_CAPACITY;
        WebLogEntry& entry = s_webLogEntries[index];
        if (entry.seq <= since) continue;
        if (!first) response += ',';
        first = false;
        response += "{\"seq\":";
        response += entry.seq;
        response += ",\"t\":";
        response += entry.t;
        response += ",\"src\":";
        appendJsonString(response, entry.source);
        response += ",\"line\":";
        appendJsonString(response, entry.line);
        response += '}';
    }
    response += "]}";
}

#endif
