#pragma once
#include "session.hpp"
#include "button_state.hpp"

namespace mousebox_firmware
{

template <class Session, class Channel, class Clock, class Capabilities> class ControlIngress
{
    mousebox_wire::Parser parser;
    bool feedback = false;
    bool buttonPulses = false;
    uint32_t nextFeedback = 0, earliestFeedback = 0, feedbackSequence = 0;
    uint8_t previousFlags = 255;

  public:
    void begin()
    {
        Channel::begin(Capabilities::BaudRate);
    }
    void service(Session &control, bool ready, ButtonState &buttons)
    {
        control.tick(Clock::nowUs());
        const uint32_t started = Clock::nowUs();
        mousebox_wire::Command command;
        for (uint8_t n = 0; n < 32 && Channel::available(); ++n)
        {
            // Leave queued UART bytes for the next loop; never delay physical USB service.
            // A final-frame CRC/command validation is a bounded tail after this check.
            if (uint32_t(Clock::nowUs() - started) >= 100)
                break;
            if (parser.feed(uint8_t(Channel::read()), command))
            {
                if (command.type == mousebox_wire::Probe && command.session == mousebox_buttons::EnableToken &&
                    command.sequence == 1 && !command.dx && !command.dy && !command.intervalMs && !command.leaseMs &&
                    !command.budget)
                {
                    feedback = true;
                    buttonPulses = false;
                    control.buttonPulse().cancel();
                    buttons.changed = true;
                    buttons.gate = false;
                    Channel::enableFeedback();
                }
                if (command.type == mousebox_wire::Probe && command.session == mousebox_buttons::EnableToken &&
                    command.sequence == 3 && command.dx == mousebox_buttons::FiniteButtonPulse && !command.dy &&
                    !command.intervalMs && !command.leaseMs && !command.budget)
                {
                    buttonPulses = (Capabilities::FeedbackFlags & mousebox_buttons::FiniteButtonPulse) != 0;
                    buttons.changed = true;
                    control.add(Counter::Accepted);
                    continue;
                }
                if (command.type == mousebox_wire::ButtonPulse && !buttonPulses)
                {
                    control.add(Counter::Rejected);
                    continue;
                }
                if (command.type == mousebox_wire::Probe && command.session == mousebox_buttons::EnableToken &&
                    command.sequence == 2 &&
                    (command.dx == mousebox_buttons::Back || command.dx == mousebox_buttons::Forward) &&
                    command.dy >= 0 && command.dy <= 1 && !command.intervalMs && !command.leaseMs && !command.budget)
                {
                    buttons.gate = command.dy == 1;
                    buttons.gateMask = uint16_t(command.dx);
                    control.stop();
                    control.add(Counter::Accepted);
                    continue;
                }
                if (ready || command.type == mousebox_wire::Stop || command.type == mousebox_wire::Probe)
                    control.accept(command, Clock::nowUs());
                else
                    control.add(Counter::Rejected);
            }
        }
        if (!ready)
            control.reset();
        const uint32_t now = Clock::nowUs();
        const uint8_t flags = (ready ? mousebox_buttons::Ready : 0) | (buttons.known ? mousebox_buttons::Known : 0) |
                              (Capabilities::FeedbackFlags & ~mousebox_buttons::FiniteButtonPulse) |
                              (buttonPulses ? (Capabilities::FeedbackFlags & mousebox_buttons::FiniteButtonPulse) : 0);
        if (!feedback || (!buttons.changed && flags == previousFlags && int32_t(now - nextFeedback) < 0))
            return;
        if (int32_t(now - earliestFeedback) < 0)
            return; // At most 100Hz, even if buttons chatter.
        // Check space for the whole frame: HardwareSerial must never wait inside the 1kHz input loop.
        if (Channel::availableForWrite() < mousebox_buttons::FrameSize)
            return;
        uint8_t frame[mousebox_buttons::FrameSize];
        if (!++feedbackSequence)
            ++feedbackSequence;
        mousebox_buttons::encode({feedbackSequence, buttons.buttons, buttons.releases, flags}, frame);
        Channel::write(frame, sizeof(frame));
        buttons.changed = false;
        previousFlags = flags;
        nextFeedback = now + 20000;
        earliestFeedback = now + 10000;
    }
    uint16_t errors() const
    {
        return parser.errors;
    }
    bool feedbackActive() const
    {
        return feedback;
    }
};

} // namespace mousebox_firmware
