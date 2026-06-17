#pragma once
#include <Arduino.h>

class StringPrint : public Print {
public:
    explicit StringPrint(String& target) : _target(target) {}
    size_t write(uint8_t value) override
    {
        _target += (char)value;
        return 1;
    }
    size_t write(const uint8_t* buffer, size_t size) override
    {
        for (size_t i = 0; i < size; i++) _target += (char)buffer[i];
        return size;
    }
private:
    String& _target;
};
