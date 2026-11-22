#pragma once
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include <thread>
#include "control/command_outbox.hpp"
#include "platform/windows/serial_port.hpp"
#include "control/button_feedback.hpp"
#include "application/ports/output_link.hpp"

namespace mousebox
{
// One worker owns all writes. Control transitions are reliable, movement is latest-only.
class SerialSender : public application::ports::OutputLink
{
    SerialPort port_;
    std::mutex mutex_;
    std::condition_variable wake_;
    CommandOutbox outbox_;
    bool writing_ = false;
    bool closing_ = false;
    std::atomic<bool> failed_{false};
    std::string error_;
    std::thread worker_;
    std::jthread reader_;
    std::mutex feedbackMutex_;
    control::ButtonFeedback feedback_;
    unsigned baudRate_ = 0;
    void run();
    void receive(std::stop_token);

  public:
    explicit SerialSender(const std::wstring &port, std::stop_token stop = {});
    ~SerialSender() override;
    void begin(const ControlCommand &command);
    void refresh(const ControlCommand &command);
    bool tryOneShot(std::uint32_t session, int dx, int dy);
    bool finiteCorrection(std::uint32_t session, int dx, int dy) override
    {
        return tryOneShot(session, dx, dy);
    }
    void stop(std::uint32_t session) override;
    void drain(std::chrono::milliseconds timeout) override;
    void check() override;
    void gate(unsigned button, bool enabled) override;
    void beginPaced(std::uint32_t session, std::uint16_t countBudget) override;
    void refreshPaced(std::uint32_t session, std::uint32_t sequence, std::int16_t dx, std::int16_t dy,
                      std::uint16_t intervalMs, std::uint16_t leaseMs) override;
    bool failed() const
    {
        return failed_.load();
    }
    std::string error();
    control::ButtonObservation buttons() override;
    unsigned baudRate() const noexcept
    {
        return baudRate_;
    }
    control::DeviceMotionLimits motionLimits() override;
    bool supportsButtonPulses() override;
    bool finiteLeftClick(std::uint32_t session, unsigned holdMs) override;
};
} // namespace mousebox
