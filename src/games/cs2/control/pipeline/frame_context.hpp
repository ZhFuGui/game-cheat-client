#pragma once
#include "games/cs2/control/aim_types.hpp"
namespace mousebox::games::cs2
{
bool alive(const Player &, const Body &) noexcept;
const Body *localBody(const Snapshot &) noexcept;
bool nativeMouse(const MouseMapping &) noexcept;
namespace pipeline
{
struct FrameContext
{
    const Snapshot *frame = nullptr;
    const Body *self = nullptr;
    control::SampleTimeEstimate timing;
    std::string block = "waiting-source";
    bool ready() const noexcept
    {
        return block == "ready";
    }
};
enum class FrameUse { TargetGeometry, LocalInput };
FrameContext qualifyFrame(const ControlInput &, const AimSettings &, std::uint64_t now,
                          FrameUse = FrameUse::TargetGeometry);
} // namespace pipeline
} // namespace mousebox::games::cs2
