#include "control/command_outbox.hpp"
#include <algorithm>
#include <stdexcept>

namespace mousebox
{
void CommandOutbox::begin(const ControlCommand &command)
{
    if (command.type != mousebox_wire::Start && command.type != mousebox_wire::Probe)
        throw std::invalid_argument("Expected START or PROBE");
    if (controls_.size() >= 8)
        throw std::runtime_error("Control queue limit reached");
    controls_.push_back(command);
    if (command.type == mousebox_wire::Start)
    {
        activeSession_ = command.session;
        lastSequence_ = command.sequence;
        motion_.reset();
    }
}
bool CommandOutbox::refresh(const ControlCommand &command)
{
    if (command.type != mousebox_wire::Motion || !activeSession_ || command.session != activeSession_ ||
        command.sequence <= lastSequence_)
        return false;
    lastSequence_ = command.sequence;
    motion_ = command;
    return true;
}
bool CommandOutbox::tryOneShot(const ControlCommand &start, const ControlCommand &motion)
{
    if (start.type != mousebox_wire::Start || motion.type != mousebox_wire::Motion || !start.session ||
        start.session != motion.session || motion.sequence <= start.sequence)
        throw std::invalid_argument("Invalid one-shot pair");
    if (!empty())
        return false;
    begin(start);
    refresh(motion);
    return true;
}
void CommandOutbox::stop(std::uint32_t session)
{
    if (!session)
    {
        clear();
    }
    else
    {
        if (session == activeSession_)
        {
            motion_.reset();
            activeSession_ = 0;
        }
        controls_.erase(
            std::remove_if(controls_.begin(), controls_.end(),
                           [session](const auto &c) { return c.type == mousebox_wire::Start && c.session == session; }),
            controls_.end());
        if (controls_.size() >= 8)
        {
            clear();
            session = 0;
        }
    }
    controls_.push_front({mousebox_wire::Stop, session, 0, 0, 0, 0, 0, 0});
}
bool CommandOutbox::tryButtonPulse(const ControlCommand &c)
{
    if (c.type != mousebox_wire::ButtonPulse || !c.session || c.sequence != 1 || c.dx || c.dy || c.intervalMs < 5 ||
        c.intervalMs > 40 || c.leaseMs || c.budget)
        throw std::invalid_argument("Invalid finite button pulse");
    if (!empty())
        return false;
    controls_.push_back(c);
    return true;
}
void CommandOutbox::clear()
{
    controls_.clear();
    motion_.reset();
    activeSession_ = lastSequence_ = 0;
}
bool CommandOutbox::pop(ControlCommand &command)
{
    if (!controls_.empty())
    {
        command = controls_.front();
        controls_.pop_front();
        return true;
    }
    if (motion_)
    {
        command = *motion_;
        motion_.reset();
        return true;
    }
    return false;
}
std::size_t CommandOutbox::popBatch(std::array<ControlCommand, 2> &commands)
{
    if (!pop(commands[0]))
        return 0;
    if (commands[0].type == mousebox_wire::Start && controls_.empty() && motion_ &&
        motion_->session == commands[0].session)
    {
        commands[1] = *motion_;
        motion_.reset();
        return 2;
    }
    return 1;
}
} // namespace mousebox
