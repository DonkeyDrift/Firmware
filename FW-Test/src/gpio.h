#pragma once

#include <driver/gpio.h>

enum GPIO_PULL
{
    None = 0,
    Down = 1,
    Up = 2,
};

class GPIO 
{
public: 
    GPIO(int num, gpio_mode_t mode, GPIO_PULL pull);
    bool Get();
    void Set(bool level);

private:
    gpio_num_t Num;
};