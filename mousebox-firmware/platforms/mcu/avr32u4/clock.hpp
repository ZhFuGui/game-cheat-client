#pragma once
#include <Arduino.h>
namespace mousebox_avr
{
struct Clock
{
    static uint32_t nowUs()
    {
        return micros();
    }
};
} // namespace mousebox_avr
