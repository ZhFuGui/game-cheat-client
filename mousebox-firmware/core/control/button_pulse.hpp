#pragma once
#include <control_wire.h>
namespace mousebox_firmware
{
// One finite left-button pulse. Its release never depends on another UART packet.
class ButtonPulse
{
    uint32_t previous = 0, deadline = 0;
    uint16_t holdMs = 20;
    bool pending = false, held = false, releasing = false;
  public:
    bool accept(const mousebox_wire::Command &c, uint32_t now)
    {
        if (c.type != mousebox_wire::ButtonPulse || !c.session || c.session == previous || c.sequence != 1 ||
            c.dx || c.dy || c.intervalMs < 5 || c.intervalMs > 40 || c.leaseMs || c.budget ||
            pending || held || releasing)
            return false;
        previous = c.session;
        holdMs = c.intervalMs;
        deadline = now + 20000UL;
        pending = true;
        return true;
    }
    void tick(uint32_t now)
    {
        if (int32_t(now - deadline) < 0) return;
        pending = false;
        if (held) { held = false; releasing = true; }
    }
    void cancel()
    {
        pending = false;
        if (held) { held = false; releasing = true; }
    }
    bool apply(uint32_t now, bool physicalDown)
    {
        tick(now);
        // A real press wins. Never turn a held physical button into a synthetic click.
        if (pending && physicalDown) pending = false;
        return pending || held;
    }
    bool needsReport() const { return pending || releasing; }
    bool needsRelease() const { return releasing; }
    void submitted(uint32_t now)
    {
        if (pending) { pending = false; held = true; deadline = now + uint32_t(holdMs) * 1000; }
        else if (releasing) releasing = false;
    }
};
}
