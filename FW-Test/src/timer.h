#pragma once

#include <driver/gptimer.h>
#include <soc/soc.h>
#include <soc/timer_group_reg.h>

class Timer 
{
public:
    static uint32_t ReadLo();

};


inline uint32_t Timer::ReadLo()
{
    REG_WRITE(REG_TIMG_BASE(0) + 0x0c, 0x8000);
    return REG_READ(REG_TIMG_BASE(0) + 0x04);
}