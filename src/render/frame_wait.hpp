#pragma once
#include <chrono>
#include <optional>

namespace mousebox::render
{
class FrameWait
{
    using Clock = std::chrono::steady_clock;
    std::optional<Clock::time_point> started_;

  public:
    static constexpr auto MaximumWait = std::chrono::milliseconds(1000);

    void blocked(Clock::time_point now) noexcept
    {
        if (!started_)
            started_ = now;
    }
    void reset() noexcept
    {
        started_.reset();
    }
    std::optional<std::chrono::milliseconds> remaining(Clock::time_point now) const noexcept
    {
        if (!started_)
            return {};
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - *started_);
        return elapsed >= MaximumWait ? std::chrono::milliseconds::zero() : MaximumWait - elapsed;
    }
};
} // namespace mousebox::render
