#pragma once
#include "report_codec.hpp"

namespace mousebox_firmware
{
struct C54dProfile
{
    typedef C54dReportCodec Codec;
    enum
    {
        MouseEndpoint = 1,
        AuxiliaryEndpoint = 2,
        VendorEndpoint = 3,
        MouseReportSize = 13,
        AuxiliaryReportSize = 17,
        VendorReportSize = 20
    };
    static bool matches(uint16_t vid, uint16_t pid)
    {
        return vid == 0x046D && pid == 0xC54D;
    }
    static bool endpoint(uint8_t ep)
    {
        return ep >= MouseEndpoint && ep <= VendorEndpoint;
    }
    static uint8_t index(uint8_t ep)
    {
        return ep - MouseEndpoint;
    }
    static uint8_t endpointAt(uint8_t index)
    {
        return index + MouseEndpoint;
    }
    static uint32_t pollIntervalUs(uint8_t ep)
    {
        return ep == MouseEndpoint ? 1000UL : 10000UL;
    }
    static bool valid(uint8_t ep, uint16_t length, const uint8_t *bytes)
    {
        if (!endpoint(ep) || !length)
            return false;
        return ep == MouseEndpoint       ? length == MouseReportSize
               : ep == AuxiliaryEndpoint ? length <= AuxiliaryReportSize
                                         : ((bytes[0] == 0x10 && length == 7) || (bytes[0] == 0x11 && length == 20));
    }
    static uint16_t buttons(const uint8_t *bytes)
    {
        return mousebox_wire::read16(bytes);
    }
    static bool leftDown(uint16_t buttons) { return (buttons & 1) != 0; }
    static void overlayLeftButton(uint8_t *bytes, bool down)
    {
        if (down) mousebox_wire::write16(bytes, uint16_t(buttons(bytes) | 1));
    }
    static bool moved(const uint8_t *bytes)
    {
        return bytes[2] || bytes[3] || bytes[4] || bytes[5];
    }
    static bool acceptsRequest(const ControlRequest &request)
    {
        return (request.bmRequestType == 0x21 || request.bmRequestType == 0xA1) && request.wIndex < 3;
    }
};
} // namespace mousebox_firmware
