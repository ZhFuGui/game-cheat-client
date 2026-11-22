#pragma once
#include <cstdint>

namespace mousebox
{
enum class HoldAction
{
    None,
    Begin,
    Refresh,
    Stop
};
// Pure input state machine; require key-up before first activation and after any stop.
class HoldPlan
{
    bool released_ = false, active_ = false;
    std::uint64_t deadline_ = 0, nextRefresh_ = 0;
    std::uint32_t durationMs_;

  public:
    explicit HoldPlan(std::uint32_t durationMs);
    HoldAction poll(bool down, std::uint64_t nowMs);
    bool active() const
    {
        return active_;
    }
};
} // namespace mousebox
