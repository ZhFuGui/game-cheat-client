#include "adapters/outputs/mousebox/serial_sender.hpp"
#include "platform/windows/input.hpp"
#include "adapters/outputs/mousebox/one_shot.hpp"
#include "adapters/outputs/mousebox/connection_probe.hpp"
#include <stdexcept>

namespace mousebox
{
SerialSender::SerialSender(const std::wstring &port, std::stop_token stop)
    : port_(port, ConnectionProbe::BaudRates.front())
{
    ConnectionProbe probe(monotonicMilliseconds());
    for (;;)
    {
        if (stop.stop_requested())
            throw std::runtime_error("Box connection cancelled");
        port_.setBaudRate(probe.baudRate());
        // These frames cannot request motion. The probe also disables any old side-button gate.
        port_.write(encodeCommand({mousebox_wire::Stop, 0, 0, 0, 0, 0, 0, 0}));
        port_.write(encodeCommand({mousebox_wire::Probe, mousebox_buttons::EnableToken, 1, 0, 0, 0, 0, 0}));
        // Optional capability negotiation: old firmware safely rejects this no-motion
        // probe; new firmware keeps legacy feedback unchanged for old PC clients.
        port_.write(encodeCommand(
            {mousebox_wire::Probe, mousebox_buttons::EnableToken, 3, mousebox_buttons::FiniteButtonPulse, 0, 0, 0, 0}));
        while (!probe.expired(monotonicMilliseconds()) && !probe.confirmed())
        {
            if (stop.stop_requested())
                throw std::runtime_error("Box connection cancelled");
            std::uint8_t bytes[128];
            const auto count = port_.readAvailable(bytes);
            const auto now = monotonicMilliseconds();
            for (std::size_t i = 0; i < count; ++i)
                probe.feed(bytes[i], now);
            if (!probe.confirmed())
                Sleep(2);
        }
        if (probe.confirmed())
            break;
        if (!probe.next(monotonicMilliseconds()))
            throw std::runtime_error("115200 / 500000 baud: no verified box feedback; check RXD/TXD and power");
    }
    baudRate_ = probe.baudRate();
    feedback_ = probe.feedback();
    worker_ = std::thread([this] { run(); });
    try
    {
        reader_ = std::jthread([this](std::stop_token token) { receive(token); });
    }
    catch (...)
    {
        {
            std::lock_guard lock(mutex_);
            closing_ = true;
        }
        wake_.notify_all();
        worker_.join();
        throw;
    }
}
SerialSender::~SerialSender()
{
    {
        std::lock_guard lock(mutex_);
        closing_ = true;
        outbox_.stop(0);
    }
    wake_.notify_all();
    if (worker_.joinable())
        worker_.join();
    reader_.request_stop();
    if (reader_.joinable())
        reader_.join();
}
void SerialSender::check()
{
    if (failed())
        throw std::runtime_error(error());
}
void SerialSender::gate(unsigned button, bool enabled)
{
    begin({mousebox_wire::Probe, mousebox_buttons::EnableToken, 2, static_cast<std::int16_t>(button),
           static_cast<std::int16_t>(enabled ? 1 : 0), 0, 0, 0});
}
void SerialSender::beginPaced(std::uint32_t session, std::uint16_t countBudget)
{
    begin({mousebox_wire::Start, session, 1, 0, 0, 0, 0, countBudget});
}
void SerialSender::refreshPaced(std::uint32_t session, std::uint32_t sequence, std::int16_t dx, std::int16_t dy,
                                std::uint16_t intervalMs, std::uint16_t leaseMs)
{
    refresh({mousebox_wire::Motion, session, sequence, dx, dy, intervalMs, leaseMs, 0});
}
void SerialSender::begin(const ControlCommand &command)
{
    validateDeviceCommand(command, motionLimits());
    std::lock_guard lock(mutex_);
    if (failed_ || closing_)
        throw std::runtime_error("Serial sender is closed");
    outbox_.begin(command);
    wake_.notify_all();
}
void SerialSender::refresh(const ControlCommand &command)
{
    validateDeviceCommand(command, motionLimits());
    std::lock_guard lock(mutex_);
    if (failed_ || closing_)
        throw std::runtime_error("Serial sender is closed");
    if (outbox_.refresh(command))
        wake_.notify_all();
}
bool SerialSender::tryOneShot(std::uint32_t session, int dx, int dy)
{
    const auto pair = oneShot(session, dx, dy, motionLimits());
    std::lock_guard lock(mutex_);
    if (failed_ || closing_)
        throw std::runtime_error("Serial sender is closed");
    if (writing_ || !outbox_.tryOneShot(pair[0], pair[1]))
        return false;
    wake_.notify_all();
    return true;
}
void SerialSender::stop(std::uint32_t session)
{
    std::lock_guard lock(mutex_);
    if (failed_ || closing_)
        return;
    outbox_.stop(session);
    wake_.notify_all();
}
bool SerialSender::supportsButtonPulses()
{
    const auto b = buttons();
    return b.received && (b.state.flags & mousebox_buttons::FiniteButtonPulse);
}
bool SerialSender::finiteLeftClick(std::uint32_t session, unsigned holdMs)
{
    const auto b = buttons();
    if (!b.fresh(monotonicMilliseconds()) || !(b.state.flags & mousebox_buttons::FiniteButtonPulse) || b.down(1))
        return false;
    if (holdMs < 5 || holdMs > 40)
        throw std::invalid_argument("Invalid button pulse duration");
    std::lock_guard lock(mutex_);
    if (failed_ || closing_ || writing_)
        return false;
    if (!outbox_.tryButtonPulse(
            {mousebox_wire::ButtonPulse, session, 1, 0, 0, static_cast<std::uint16_t>(holdMs), 0, 0}))
        return false;
    wake_.notify_all();
    return true;
}
void SerialSender::drain(std::chrono::milliseconds timeout)
{
    std::unique_lock lock(mutex_);
    if (!wake_.wait_for(lock, timeout, [this] { return failed_ || (outbox_.empty() && !writing_); }))
        throw std::runtime_error("Serial send queue did not drain in time");
    if (failed_)
        throw std::runtime_error(error_);
}
std::string SerialSender::error()
{
    std::lock_guard lock(mutex_);
    return error_;
}
control::ButtonObservation SerialSender::buttons()
{
    std::lock_guard lock(feedbackMutex_);
    return feedback_.observation();
}
control::DeviceMotionLimits SerialSender::motionLimits()
{
    return effectiveMotionLimits(buttons().motionLimits(), baudRate_);
}
void SerialSender::receive(std::stop_token token)
{
    try
    {
        std::uint8_t bytes[128];
        while (!token.stop_requested() && !failed_)
        {
            const auto size = port_.readAvailable(bytes);
            const auto now = monotonicMilliseconds();
            {
                std::lock_guard lock(feedbackMutex_);
                for (std::size_t i = 0; i < size; ++i)
                    feedback_.feed(bytes[i], now);
            }
            Sleep(2);
        }
    }
    catch (const std::exception &e)
    {
        {
            std::lock_guard lock(mutex_);
            if (!failed_)
                error_ = e.what();
            failed_ = true;
            outbox_.clear();
        }
        wake_.notify_all();
    }
}
void SerialSender::run()
{
    try
    {
        for (;;)
        {
            std::array<ControlCommand, 2> commands{};
            std::size_t count = 0;
            {
                std::unique_lock lock(mutex_);
                wake_.wait(lock, [this] { return closing_ || !outbox_.empty(); });
                count = outbox_.popBatch(commands);
                if (!count)
                    break;
                writing_ = true;
            }
            // One USB-to-UART write per finite step, with no extra waiting or batching delay.
            std::array<std::uint8_t, 2 * mousebox_wire::FrameSize> bytes{};
            const auto limits = motionLimits();
            for (std::size_t i = 0; i < count; ++i)
            {
                validateDeviceCommand(commands[i], limits);
                mousebox_wire::encode(commands[i], bytes.data() + i * mousebox_wire::FrameSize);
            }
            port_.write(std::span<const std::uint8_t>(bytes.data(), count * mousebox_wire::FrameSize));
            {
                std::lock_guard lock(mutex_);
                writing_ = false;
            }
            wake_.notify_all();
        }
    }
    catch (const std::exception &e)
    {
        {
            std::lock_guard lock(mutex_);
            error_ = e.what();
            failed_ = true;
            writing_ = false;
            outbox_.clear();
        }
        wake_.notify_all();
    }
}
} // namespace mousebox
