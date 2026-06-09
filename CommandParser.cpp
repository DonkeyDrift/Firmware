#include "CommandParser.h"

uint8_t parseHex2(const char* s)
{
    auto hv = [](char c)->uint8_t{ if(c>='0'&&c<='9')return c-'0'; if(c>='a'&&c<='f')return 10+(c-'a'); if(c>='A'&&c<='F')return 10+(c-'A'); return 0; };
    return (hv(s[0])<<4)|hv(s[1]);
}

uint8_t calcChecksum(const char* s, int n)
{
    uint32_t sum = 0;
    for (int i=0;i<n;i++) sum += (uint8_t)s[i];
    return (uint8_t)(sum & 0xFF);
}

bool parsePilotCommandLine(const String& line, int* throttle, int* steering, int* seq)
{
    *seq = -1;
    int star = line.lastIndexOf('*');
    if (star > 0)
    {
        String payload = line.substring(0, star);
        String cs = line.substring(star+1);
        if (cs.length()>=2)
        {
            char cs0 = cs.charAt(0);
            char cs1 = cs.charAt(1);
            char tmp[3]; tmp[0]=cs0; tmp[1]=cs1; tmp[2]=0;
            uint8_t want = parseHex2(tmp);
            int plen = payload.length();
            char buf[260]; int blen = plen; if (blen>259) blen=259;
            payload.toCharArray(buf, blen+1);
            uint8_t got = calcChecksum(buf, blen);
            if (want != got) return false;

            // Try to parse SEQ: T:S:SEQ
            int col2 = payload.lastIndexOf(':');
            int col1 = payload.indexOf(':');
            if (col2 > col1 && col1 > 0) {
                 String seqStr = payload.substring(col2+1);
                 *seq = seqStr.toInt();
                 return parseAndValidateCommand(payload.substring(0, col2), throttle, steering);
            }
            return parseAndValidateCommand(payload, throttle, steering);
        }
    }

    // No checksum; try to parse T:S:SEQ
    int col2 = line.lastIndexOf(':');
    int col1 = line.indexOf(':');
    if (col2 > col1 && col1 > 0) {
            String seqStr = line.substring(col2+1);
            *seq = seqStr.toInt();
            return parseAndValidateCommand(line.substring(0, col2), throttle, steering);
    }

    return parseAndValidateCommand(line, throttle, steering);
}

bool parseAndValidateCommand(String cmd, int* throttle, int* steering)
{
    int colonIndex = cmd.indexOf(':');
    if (colonIndex <= 0)
    {
        return false;
    }

    String throttleStr = cmd.substring(0, colonIndex);
    String steeringStr = cmd.substring(colonIndex + 1);

    int t = throttleStr.toInt();
    int s = steeringStr.toInt();

    // 校验控制范围，避免不可信输入直接影响执行器输出。
    if (t < -100 || t > 100 || s < -100 || s > 100)
    {
        // Print errors only when this is not a test command, to avoid polluting output
        // Serial.print("[CMD ERROR] Out of range: T=");
        // Serial.print(t);
        // Serial.print(" S=");
        // Serial.println(s);
        return false;
    }

    *throttle = t;
    *steering = s;
    return true;
}

#ifdef ENABLE_DIAGNOSTIC_COMMANDS
bool runUnitTests()
{
    int testsTotal = 0;
    int testsPassed = 0;
    int t, s, seq;

    // Basic format
    testsTotal++; if (parsePilotCommandLine(String("0:0"), &t, &s, &seq) && t == 0 && s == 0 && seq == -1) testsPassed++;
    testsTotal++; if (!parsePilotCommandLine(String("200:0"), &t, &s, &seq)) testsPassed++;
    // Checksum format
    char payload1[] = "10:-10";
    uint8_t cs1 = calcChecksum(payload1, sizeof(payload1)-1);
    char line1[32]; snprintf(line1, sizeof(line1), "%s*%02X", payload1, cs1);
    testsTotal++; if (parsePilotCommandLine(String(line1), &t, &s, &seq) && t == 10 && s == -10 && seq == -1) testsPassed++;
    // Seq format
    testsTotal++; if (parsePilotCommandLine(String("50:50:100"), &t, &s, &seq) && t == 50 && s == 50 && seq == 100) testsPassed++;
    // Seq + Checksum
    char payload2[] = "20:-20:255";
    uint8_t cs2 = calcChecksum(payload2, sizeof(payload2)-1);
    char line2[32]; snprintf(line2, sizeof(line2), "%s*%02X", payload2, cs2);
    testsTotal++; if (parsePilotCommandLine(String(line2), &t, &s, &seq) && t == 20 && s == -20 && seq == 255) testsPassed++;

    return testsPassed * 100 / testsTotal >= 85;
}
#endif
