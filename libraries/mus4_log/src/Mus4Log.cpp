#include "Mus4Log.h"

#include <stdarg.h>

uint8_t mus4LogTarget = MUS4_LOG_TARGET;
static Mus4LogSink mus4WebLogSink = nullptr;

void mus4SetWebLogSink(Mus4LogSink sink)
{
    mus4WebLogSink = sink;
}

void setMus4LogTargetWeb()
{
#if defined(ENABLE_WIFI_CONSOLE)
    mus4LogTarget = MUS4_LOG_TARGET_WEB;
#else
    mus4LogTarget = MUS4_LOG_TARGET_SERIAL;
#endif
}

void mus4LogLine(const char* source, const String& line)
{
#if defined(ENABLE_WIFI_CONSOLE)
    if (mus4LogTarget == MUS4_LOG_TARGET_WEB && mus4WebLogSink != nullptr) {
        mus4WebLogSink(source, line);
        return;
    }
#endif
    Serial.println("[" + String(source) + "] " + line);
}

void mus4Logf(const char* source, const char* fmt, ...)
{
    char buf[192];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    mus4LogLine(source, String(buf));
}
