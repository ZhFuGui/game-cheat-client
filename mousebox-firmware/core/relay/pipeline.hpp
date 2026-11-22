#pragma once
#include "report_queue.hpp"
#include "../contracts/ports.hpp"
#include "../control/button_state.hpp"
#include "../diagnostics/counters.hpp"

namespace mousebox_firmware
{
template <class Profile, class Resources, class Session, class Device, class Clock, class Counters = NoCounters>
class ReportPipeline
{
    ReportQueue<Resources::MouseQueueDepth, Profile::MouseReportSize> mouse;
    ReportQueue<Resources::AuxiliaryQueueDepth, Profile::AuxiliaryReportSize> media;
    ReportQueue<Resources::VendorQueueDepth, Profile::VendorReportSize> vendor;
    Session *control = nullptr;
    typename Profile::Codec codec;
    ButtonState *buttons = nullptr;
    int submitMouse()
    {
        if (!control)
            return mouse.template submit<Device>(Profile::MouseEndpoint);
        if (buttons && !buttons->motionAllowed())
            control->stopMotion();
        const uint32_t now = Clock::nowUs();
        const MotionStep desired = control->peek(now);
        auto &pulse = control->buttonPulse();
        const uint8_t *physical = mouse.front();
        if (!physical && ((!desired.x && !desired.y && !pulse.needsReport()) || !codec.canSynthesize()))
            return 0;
        uint8_t output[Profile::MouseReportSize];
        if (!physical)
        {
            codec.synthetic(output);
            physical = output;
        }
        const uint16_t rawButtons = Profile::buttons(physical);
        const bool injected = pulse.apply(now, Profile::leftDown(rawButtons));
        const MotionStep used = Profile::Codec::compose(physical, output, desired);
        Profile::overlayLeftButton(output, injected);
        const int result = Device::trySend(Profile::MouseEndpoint, output, sizeof(output));
        if (result > 0)
        {
            codec.observeSubmitted(output);
            codec.observePhysicalButtons(rawButtons);
            pulse.submitted(now);
            if (mouse.size())
                mouse.pop();
            else
                stats.add(Counter::Synthetic);
            control->commit(used, now);
        }
        return result;
    }

  public:
    Counters stats;
    void attachControl(Session &state)
    {
        control = &state;
    }
    void attachButtons(ButtonState &state)
    {
        buttons = &state;
    }
    uint8_t queued(uint8_t ep) const
    {
        return ep == Profile::MouseEndpoint       ? mouse.size()
               : ep == Profile::AuxiliaryEndpoint ? media.size()
               : ep == Profile::VendorEndpoint    ? vendor.size()
                                                  : 0;
    }
    bool canReceive(uint8_t ep) const
    {
        return ep == Profile::MouseEndpoint       ? !mouse.full()
               : ep == Profile::AuxiliaryEndpoint ? !media.full()
                                                  : ep == Profile::VendorEndpoint && !vendor.full();
    }
    void receive(uint8_t ep, uint16_t length, const uint8_t *bytes)
    {
        if (!Profile::endpoint(ep) || !length)
            return;
        stats.template report<Profile, Clock>(ep, length, bytes);
        if (!Profile::valid(ep, length, bytes))
        {
            stats.add(Counter::Invalid);
            return;
        }
        const bool queued = ep == Profile::MouseEndpoint       ? mouse.push(bytes, length)
                            : ep == Profile::AuxiliaryEndpoint ? media.push(bytes, length)
                                                               : vendor.push(bytes, length);
        if (!queued)
            stats.add(Counter::Overflow);
        if (ep == Profile::MouseEndpoint && buttons)
            buttons->observe(Profile::buttons(bytes));
    }
    void service()
    {
        if (submitMouse() > 0)
            stats.add(Counter::Submitted, Profile::index(Profile::MouseEndpoint));
        if (media.template submit<Device>(Profile::AuxiliaryEndpoint) > 0)
            stats.add(Counter::Submitted, Profile::index(Profile::AuxiliaryEndpoint));
        if (vendor.template submit<Device>(Profile::VendorEndpoint) > 0)
            stats.add(Counter::Submitted, Profile::index(Profile::VendorEndpoint));
    }
    void clear()
    {
        const uint16_t removed = mouse.clear() + media.clear() + vendor.clear();
        stats.add(Counter::Discarded, 0, removed);
        if (buttons)
            buttons->reset();
        if (control)
            control->reset();
        // Keep the raw cache until a pending synthetic release reaches USB.
        if (!control || !control->buttonPulse().needsRelease())
            codec.reset();
    }
};
} // namespace mousebox_firmware
