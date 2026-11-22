#pragma once
#include <control_wire.h>
#include "../../core/contracts/types.hpp"
#include <string.h>
namespace mousebox_firmware
{

class C54dReportCodec
{
    uint8_t buttons[2] = {}, vendor[5] = {};
    bool known = false;
    static int16_t addition(int16_t physical, int16_t correction)
    {
        int32_t sum = int32_t(physical) + correction;
        if (sum > 32767)
            sum = 32767;
        if (sum < -32767)
            sum = -32767;
        // Never change a physical value when it is outside the declared descriptor range.
        if (physical == -32768)
            return 0;
        return int16_t(sum - physical);
    }

  public:
    void reset()
    {
        known = false;
    }
    bool canSynthesize() const
    {
        return known;
    }
    void observeSubmitted(const uint8_t *p)
    {
        memcpy(buttons, p, 2);
        memcpy(vendor, p + 8, 5);
        known = true;
    }
    void observePhysicalButtons(uint16_t value) { mousebox_wire::write16(buttons, value); }
    void synthetic(uint8_t *p) const
    {
        memset(p, 0, 13);
        memcpy(p, buttons, 2);
        memcpy(p + 8, vendor, 5);
    }
    static MotionStep compose(const uint8_t *physical, uint8_t *output, MotionStep desired)
    {
        if (output != physical)
            memcpy(output, physical, 13);
        MotionStep used = {addition(mousebox_wire::readSigned16(physical + 2), desired.x),
                           addition(mousebox_wire::readSigned16(physical + 4), desired.y)};
        mousebox_wire::write16(output + 2, uint16_t(int32_t(mousebox_wire::readSigned16(physical + 2)) + used.x));
        mousebox_wire::write16(output + 4, uint16_t(int32_t(mousebox_wire::readSigned16(physical + 4)) + used.y));
        return used;
    }
};

} // namespace mousebox_firmware
