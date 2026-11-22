#pragma once
#include <Arduino.h>
namespace mousebox_avr
{
struct Uart
{
    static void begin(uint32_t baud)
    {
        Serial1.begin(baud);
        // Leave TX undriven until the PC opts into side-button feedback.
        UCSR1B &= ~_BV(TXEN1);
    }
    static void enableFeedback()
    {
        UCSR1B |= _BV(TXEN1);
    }
    static int available()
    {
        return Serial1.available();
    }
    static int read()
    {
        return Serial1.read();
    }
    static int availableForWrite()
    {
        return Serial1.availableForWrite();
    }
    static void write(const uint8_t *bytes, uint16_t length)
    {
        Serial1.write(bytes, length);
    }
};
} // namespace mousebox_avr
