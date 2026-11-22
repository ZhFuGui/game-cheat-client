#pragma once
#include "application/json.hpp"

namespace mousebox::games::cs2
{
inline constexpr auto HitboxInsetSetting = "cs2HitboxInsetPercent";
inline constexpr double DefaultHitboxInsetPercent = 10;

inline double readHitboxInsetPercent(const application::Json &values)
{
    return values.value(HitboxInsetSetting, DefaultHitboxInsetPercent);
}
} // namespace mousebox::games::cs2
