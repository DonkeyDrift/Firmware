#include "JsonUtil.h"

void appendJsonString(String& out, const char* text)
{
    out += '"';
    while (*text) {
        char c = *text++;
        if (c == '\\' || c == '"') {
            out += '\\';
            out += c;
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else if ((uint8_t)c >= 32) {
            out += c;
        }
    }
    out += '"';
}
