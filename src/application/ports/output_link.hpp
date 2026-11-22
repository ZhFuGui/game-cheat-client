#pragma once
#include "output_device.hpp"
#include "control/button_feedback.hpp"
#include <chrono>

namespace mousebox::application::ports
{
// A live link is borrowed by its sole output worker. The transport owns
// framing, pacing commands, and writes; callers supply counts and a session.
class OutputLink : public OutputDevice
{
  public:
    virtual control::ButtonObservation buttons() = 0;
    virtual void check() = 0;
    virtual void gate(unsigned button, bool enabled) = 0;
    virtual void beginPaced(std::uint32_t session, std::uint16_t countBudget) = 0;
    virtual void refreshPaced(std::uint32_t session, std::uint32_t sequence, std::int16_t dx, std::int16_t dy,
                              std::uint16_t intervalMs, std::uint16_t leaseMs) = 0;
    virtual void drain(std::chrono::milliseconds timeout) = 0;
};
} // namespace mousebox::application::ports
