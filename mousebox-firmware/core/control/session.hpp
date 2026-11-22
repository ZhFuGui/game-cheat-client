#pragma once
#include <control_wire.h>
#include "../contracts/types.hpp"
#include "../diagnostics/counters.hpp"
#include "button_pulse.hpp"

namespace mousebox_firmware
{

template <class Limits, class Counters = NoCounters> class ControlSession : public Counters
{
    uint32_t sessionId = 0, sequence = 0, deadlineUs = 0, nextStepUs = 0;
    int16_t dx = 0, dy = 0;
    uint16_t remaining = 0, intervalMs = 10;
    bool open = false, moving = false;
    ButtonPulse pulse_;
    static uint16_t magnitude(int16_t n)
    {
        return uint16_t(n < 0 ? -int32_t(n) : n);
    }
    static bool empty(const mousebox_wire::Command &c)
    {
        return !c.dx && !c.dy && !c.intervalMs && !c.leaseMs && !c.budget;
    }

  public:
    void stopMotion()
    {
        open = moving = false;
        dx = dy = 0;
        remaining = 0;
    }
    void stop() { stopMotion(); pulse_.cancel(); }
    ButtonPulse &buttonPulse() { return pulse_; }
    void reset()
    {
        stop();
    } // Keep the retired session ID: reset cannot revive delayed frames.
    void tick(uint32_t now)
    {
        pulse_.tick(now);
        if (open && int32_t(now - deadlineUs) >= 0)
        {
            stopMotion();
            this->add(Counter::Expired);
        }
    }
    bool accept(const mousebox_wire::Command &c, uint32_t now)
    {
        using namespace mousebox_wire;
        tick(now);
        if (c.type == mousebox_wire::ButtonPulse)
        {
            const bool accepted = pulse_.accept(c, now);
            this->add(accepted ? Counter::Accepted : Counter::Rejected);
            return accepted;
        }
        if (c.type == Probe && empty(c))
        {
            this->add(Counter::Accepted);
            return true;
        }
        if (c.type == Stop && empty(c) && (!c.session || c.session == sessionId))
        {
            stop();
            this->add(Counter::Stops);
            this->add(Counter::Accepted);
            return true;
        }
        if (c.type == Start && c.session && c.session != sessionId && c.sequence && !c.dx && !c.dy && !c.intervalMs &&
            !c.leaseMs && c.budget && c.budget <= Limits::MaxBudget)
        {
            stopMotion();
            sessionId = c.session;
            sequence = c.sequence;
            remaining = c.budget;
            deadlineUs = now + 100000UL;
            nextStepUs = now;
            open = true;
            this->add(Counter::Accepted);
            return true;
        }
        if (c.type == Motion && open && c.session == sessionId && c.sequence > sequence &&
            c.dx >= -Limits::MaxStepCounts && c.dx <= Limits::MaxStepCounts && c.dy >= -Limits::MaxStepCounts &&
            c.dy <= Limits::MaxStepCounts && c.intervalMs >= Limits::MinIntervalMs && c.intervalMs <= 50 &&
            c.leaseMs >= 20 && c.leaseMs <= 100 && !c.budget)
        {
            if (!moving)
                nextStepUs = now;
            sequence = c.sequence;
            dx = c.dx;
            dy = c.dy;
            intervalMs = c.intervalMs;
            deadlineUs = now + uint32_t(c.leaseMs) * 1000;
            moving = true;
            this->add(Counter::Accepted);
            return true;
        }
        this->add(Counter::Rejected);
        return false;
    }
    MotionStep peek(uint32_t now)
    {
        tick(now);
        MotionStep out = {0, 0};
        if (!moving || !remaining || int32_t(now - nextStepUs) < 0)
            return out;
        uint16_t x = magnitude(dx);
        if (x > remaining)
            x = remaining;
        uint16_t y = magnitude(dy);
        if (y > remaining - x)
            y = remaining - x;
        out.x = dx < 0 ? -int16_t(x) : int16_t(x);
        out.y = dy < 0 ? -int16_t(y) : int16_t(y);
        return out;
    }
    void commit(MotionStep used, uint32_t now)
    {
        uint16_t cost = magnitude(used.x) + magnitude(used.y);
        if (!cost)
            return;
        remaining -= cost;
        this->add(Counter::Steps);
        nextStepUs = now + uint32_t(intervalMs) * 1000; // No catch-up bursts after backpressure.
        if (!remaining)
            stopMotion();
    }
    uint32_t session() const
    {
        return sessionId;
    }
    bool active() const
    {
        return open;
    }
    uint16_t budgetLeft() const
    {
        return remaining;
    }
};

} // namespace mousebox_firmware
