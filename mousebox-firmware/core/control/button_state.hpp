#pragma once
#include <button_wire.h>
namespace mousebox_firmware
{
class ButtonState
{
  public:
    uint16_t buttons = 0, releases = 0;
    bool known = false, changed = true;
    bool gate = false;
    uint16_t gateMask = mousebox_buttons::Back;
    bool motionAllowed() const
    {
        return !gate || (known && (buttons & gateMask));
    }
    void observe(uint16_t next)
    {
        if (known && (buttons & ~next) & mousebox_buttons::Back)
            releases = (releases & 0xFF00) | uint8_t(releases + 1);
        if (known && (buttons & ~next) & mousebox_buttons::Forward)
            releases += 0x100;
        if (!known || buttons != next)
            changed = true;
        buttons = next;
        known = true;
    }
    void reset()
    {
        buttons = 0;
        known = false;
        changed = true;
        releases = ((releases + 0x100) & 0xFF00) | uint8_t(releases + 1);
    }
};

} // namespace mousebox_firmware
