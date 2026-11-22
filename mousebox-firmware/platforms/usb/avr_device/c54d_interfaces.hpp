#pragma once
#include <PluggableUSB.h>
#include <string.h>
#define MOUSEBOX_ROM PROGMEM
#include "../../../profiles/logitech_c54d/descriptors.hpp"
#undef MOUSEBOX_ROM

namespace mousebox_avr
{

class NativeUsbInterfaces : public PluggableUSBModule
{
    uint8_t endpointTypes[3] = {EP_TYPE_INTERRUPT_IN, EP_TYPE_INTERRUPT_IN, EP_TYPE_INTERRUPT_IN};

  public:
    NativeUsbInterfaces() : PluggableUSBModule(3, 3, endpointTypes)
    {
        PluggableUSB().plug(this);
    }
    int getInterface(uint8_t *count) override
    {
        *count += 3;
        return USB_SendControl(TRANSFER_PGM, nativeConfiguration, sizeof(nativeConfiguration));
    }
    int getDescriptor(USBSetup &setup) override
    {
        if (setup.bmRequestType != 0x81 || setup.wIndex >= 3)
            return 0;
        uint8_t iface = setup.wIndex;
        if (setup.wValueH == 0x21)
            return USB_SendControl(TRANSFER_PGM, nativeConfiguration + iface * 25 + 9, 9);
        if (setup.wValueH != 0x22)
            return 0;
        if (iface == 0)
            return USB_SendControl(TRANSFER_PGM, nativeMouseDescriptor, sizeof(nativeMouseDescriptor));
        if (iface == 1)
            return USB_SendControl(TRANSFER_PGM, nativeKeyboardDescriptor, sizeof(nativeKeyboardDescriptor));
        return USB_SendControl(TRANSFER_PGM, nativeHidppDescriptor, sizeof(nativeHidppDescriptor));
    }
    bool setup(USBSetup &) override
    {
        return false;
    }
    uint8_t getShortName(char *name) override
    {
        const char serial[] = "3176385A3433";
        memcpy(name, serial, sizeof(serial) - 1);
        return sizeof(serial) - 1;
    }
};

} // namespace mousebox_avr
