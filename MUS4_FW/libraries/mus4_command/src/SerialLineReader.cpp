#include "SerialLineReader.h"

#include "FirmwareConfig.h"
#include "CommandDispatcher.h"
#include "SerialRole.h"
#include "StringPrint.h"

#ifdef ENABLE_WIFI_CONSOLE
#include "WebLogBuffer.h"
#include "WirelessConsole.h"
#endif

// WebLog 源标签按角色归类：主遥测端口恒为 "serial1"（命中专用高吞吐环形缓冲），
// 控制台端口恒为 "serial"。物理口被 MUS4_SWAP_SERIAL0_SERIAL1 对调后，
// 标签不随物理口漂移，避免高速遥测/控制帧涌入通用日志环。
static const char* serialSourceFor(HardwareSerial& ser)
{
    return serialRoleSourceFor(ser);
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
            // Pilot throttle/steering packets are sent at high rate by host
            // software and should not generate ACK replies, otherwise the host
            // logs them as unrecognized data.
            dispatchCommandLine(line, out, sb, /*pilotSilent=*/true);
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
