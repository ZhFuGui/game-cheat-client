#include "control/hold_plan.hpp"
#include <stdexcept>

namespace mousebox
{
HoldPlan::HoldPlan(std::uint32_t durationMs) : durationMs_(durationMs)
{
    if (!durationMs || durationMs > 1000)
        throw std::invalid_argument("Hold duration must be 1..1000 ms");
}
HoldAction HoldPlan::poll(bool down, std::uint64_t nowMs)
{
    if (active_ && (!down || nowMs >= deadline_))
    {
        active_ = false;
        released_ = !down;
        return HoldAction::Stop;
    }
    if (!down)
    {
        released_ = true;
        return HoldAction::None;
    }
    if (!active_ && released_)
    {
        released_ = false;
        active_ = true;
        deadline_ = nowMs + durationMs_;
        nextRefresh_ = nowMs + 10;
        return HoldAction::Begin;
    }
    if (active_ && nowMs >= nextRefresh_)
    {
        nextRefresh_ = nowMs + 10;
        return HoldAction::Refresh;
    }
    return HoldAction::None;
}
} // namespace mousebox
