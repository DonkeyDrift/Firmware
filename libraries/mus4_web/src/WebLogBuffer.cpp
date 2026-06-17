#include "WebLogBuffer.h"

#ifdef ENABLE_WIFI_CONSOLE

#include "JsonUtil.h"
#include "WifiConsoleTypes.h"

#include <string.h>

// Compact entry for high-rate Serial1 telemetry (e.g. "T0:S0").  Kept in a
// separate ring buffer so that 500 Hz telemetry does not evict general web/
// serial/command logs from the shared 64-slot buffer.
struct Serial1WebLogEntry {
    uint32_t seq;
    unsigned long t;
    char line[16];
};

static const char* canonicalWebLogSource(const char* source)
{
    if (strcmp(source, "serial") == 0 || strcmp(source, "serial1") == 0) {
        return source;
    }
    return "web";
}

static WebLogEntry s_webLogEntries[WIFI_WEB_LOG_CAPACITY];
static Serial1WebLogEntry s_serial1LogEntries[SERIAL1_WEB_LOG_CAPACITY];
static uint32_t s_webLogSeq = 0;
static uint32_t s_webLogDropped = 0;
static uint16_t s_webLogHead = 0;
static uint16_t s_webLogCount = 0;
static uint8_t s_serial1LogHead = 0;
static uint8_t s_serial1LogCount = 0;
static WebLogSocketSink s_webLogSocketSink = nullptr;

static void appendGeneralWebLog(const char* source, const String& line)
{
    const char* src = canonicalWebLogSource(source);
    WebLogEntry& entry = s_webLogEntries[s_webLogHead];
    entry.seq = ++s_webLogSeq;
    entry.t = millis();
    snprintf(entry.source, sizeof(entry.source), "%s", src);
    snprintf(entry.line, sizeof(entry.line), "%s", line.c_str());
    s_webLogHead = (s_webLogHead + 1) % WIFI_WEB_LOG_CAPACITY;
    if (s_webLogCount < WIFI_WEB_LOG_CAPACITY) {
        s_webLogCount++;
    } else {
        s_webLogDropped++;
    }
    if (s_webLogSocketSink) {
        s_webLogSocketSink(entry.seq, entry.t, src, entry.line);
    }
}

static void appendSerial1WebLog(const String& line)
{
    Serial1WebLogEntry& entry = s_serial1LogEntries[s_serial1LogHead];
    entry.seq = ++s_webLogSeq;
    entry.t = millis();
    snprintf(entry.line, sizeof(entry.line), "%s", line.c_str());
    s_serial1LogHead = (s_serial1LogHead + 1) % SERIAL1_WEB_LOG_CAPACITY;
    if (s_serial1LogCount < SERIAL1_WEB_LOG_CAPACITY) {
        s_serial1LogCount++;
    } else {
        s_webLogDropped++;
    }
    if (s_webLogSocketSink) {
        s_webLogSocketSink(entry.seq, entry.t, "serial1", entry.line);
    }
}

void webLogBufferInit()
{
    s_webLogSeq = 0;
    s_webLogDropped = 0;
    s_webLogHead = 0;
    s_webLogCount = 0;
    s_serial1LogHead = 0;
    s_serial1LogCount = 0;
    s_webLogSocketSink = nullptr;
    for (uint16_t i = 0; i < WIFI_WEB_LOG_CAPACITY; i++) {
        s_webLogEntries[i] = WebLogEntry{};
    }
    for (uint8_t i = 0; i < SERIAL1_WEB_LOG_CAPACITY; i++) {
        s_serial1LogEntries[i] = Serial1WebLogEntry{};
    }
}

void webLogBufferSetSocketSink(WebLogSocketSink sink)
{
    s_webLogSocketSink = sink;
}

void appendWebLog(const char* source, const String& line)
{
    if (strcmp(canonicalWebLogSource(source), "serial1") == 0) {
        appendSerial1WebLog(line);
    } else {
        appendGeneralWebLog(source, line);
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

static void appendGeneralEntryJson(String& response, WebLogEntry& entry, bool& first)
{
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

static void appendSerial1EntryJson(String& response, Serial1WebLogEntry& entry, bool& first)
{
    if (!first) response += ',';
    first = false;
    response += "{\"seq\":";
    response += entry.seq;
    response += ",\"t\":";
    response += entry.t;
    response += ",\"src\":\"serial1\",\"line\":";
    appendJsonString(response, entry.line);
    response += '}';
}

void writeWebLogsJson(String& response, uint32_t since)
{
    response += "{\"dropped\":";
    response += s_webLogDropped;
    response += ',';
    response += '"';
    response += "entries";
    response += '"';
    response += ':';
    response += '[';

    uint16_t genPos = 0;
    uint8_t s1Pos = 0;
    bool first = true;

    while (genPos < s_webLogCount || s1Pos < s_serial1LogCount) {
        bool useGeneral;
        if (genPos >= s_webLogCount) {
            useGeneral = false;
        } else if (s1Pos >= s_serial1LogCount) {
            useGeneral = true;
        } else {
            uint16_t genIdx = (s_webLogHead + WIFI_WEB_LOG_CAPACITY - s_webLogCount + genPos) % WIFI_WEB_LOG_CAPACITY;
            uint8_t s1Idx = (s_serial1LogHead + SERIAL1_WEB_LOG_CAPACITY - s_serial1LogCount + s1Pos) % SERIAL1_WEB_LOG_CAPACITY;
            useGeneral = s_webLogEntries[genIdx].seq <= s_serial1LogEntries[s1Idx].seq;
        }

        if (useGeneral) {
            uint16_t genIdx = (s_webLogHead + WIFI_WEB_LOG_CAPACITY - s_webLogCount + genPos) % WIFI_WEB_LOG_CAPACITY;
            WebLogEntry& entry = s_webLogEntries[genIdx];
            genPos++;
            if (entry.seq <= since) continue;
            appendGeneralEntryJson(response, entry, first);
        } else {
            uint8_t s1Idx = (s_serial1LogHead + SERIAL1_WEB_LOG_CAPACITY - s_serial1LogCount + s1Pos) % SERIAL1_WEB_LOG_CAPACITY;
            Serial1WebLogEntry& entry = s_serial1LogEntries[s1Idx];
            s1Pos++;
            if (entry.seq <= since) continue;
            appendSerial1EntryJson(response, entry, first);
        }
    }

    response += "]}";
}

#endif
