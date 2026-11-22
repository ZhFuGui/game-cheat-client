#pragma once
#include <Arduino.h>
extern int USB_TrySendPacket(uint8_t endpoint, const void *data, uint8_t length);
extern uint8_t USB_NativeEpoch();
extern uint8_t USBGetConfiguration();
extern void USB_BridgeService();

namespace mousebox_avr
{
struct Device
{
    static bool configured()
    {
        return USBGetConfiguration() != 0;
    }
    static uint8_t epoch()
    {
        return USB_NativeEpoch();
    }
    static int trySend(uint8_t endpoint, const void *bytes, uint16_t length)
    {
        if (length > 64)
            return -1;
        return USB_TrySendPacket(endpoint, bytes, uint8_t(length));
    }
    static void serviceControl()
    {
        USB_BridgeService();
    }
};
} // namespace mousebox_avr
