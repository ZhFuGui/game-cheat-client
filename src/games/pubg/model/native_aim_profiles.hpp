#pragma once
#include <string_view>

namespace mousebox::control
{
// Application-side source identities only. Each branch keeps its full existing
// geometry, ownership, source-time and stage checks; unknown revisions stay out.
// This does not change the separately pinned Reader wire validator.
constexpr bool knownBarrelAimProfile(std::string_view profile) noexcept
{
    return profile == "2610.1.1.65/barrel-zeroing-v1" || profile == "2610.1.1.71/barrel-zeroing-v1";
}
constexpr bool knownViewNoTraceAimProfile(std::string_view profile) noexcept
{
    return profile == "2610.1.1.65/view-no-trace-v1" || profile == "2610.1.1.71/view-no-trace-v1";
}
} // namespace mousebox::control
