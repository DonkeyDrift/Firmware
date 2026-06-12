#include "SerialLineReader.h"

#include "FirmwareConfig.h"
#include "CommandDispatcher.h"
#include "StringPrint.h"

#ifdef ENABLE_WIFI_CONSOLE
#include "WebLogBuffer.h"
#include "WirelessConsole.h"
#endif

static const char* serialSourceFor(HardwareSerial& ser)
{
    if (&ser == &Serial1) return "serial1";
    return "serial";
}

void readSerialBuf(HardwareSerial& ser, SerialBuf& sb)
{
    while (ser.available())
    {
        int c = ser.read();
        if (c < 0) break;
        if (c == '\r') continue;
        if (c == '\n')
        {
            sb.buf[sb.len] = 0;
            String line = String(sb.buf);
#ifdef ENABLE_WIFI_CONSOLE
            appendWebLog(serialSourceFor(ser), String("> ") + redactWirelessConsoleLine(line));
#endif
            String response;
            StringPrint out(response);
            dispatchCommandLine(line, out, sb);
            ser.print(response);
#ifdef ENABLE_WIFI_CONSOLE
            appendWebLogLines(serialSourceFor(ser), response);
#endif
            sb.len = 0;
            sb.overflow = false;
        }
        else
        {
            if (sb.len < sizeof(sb.buf)-1)
            {
                sb.buf[sb.len++] = (char)c;
            }
            else
            {
                sb.len = 0;
                sb.overflow = true;
            }
        }
    }
}
