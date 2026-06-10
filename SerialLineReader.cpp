#include "SerialLineReader.h"

#include "CommandDispatcher.h"

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
            dispatchCommandLine(String(sb.buf), ser, sb);
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
