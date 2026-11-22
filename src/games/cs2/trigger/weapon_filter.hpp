#pragma once
#include "games/cs2/model/weapon_category.hpp"
namespace mousebox::games::cs2::trigger
{
enum class GunCategory
{
    Unsupported,
    Sniper,
    Other
};
// Valve item definitions: sniper_rifle prefabs are AWP, G3SG1, SCAR-20 and SSG 08.
// Keep the existing reviewed firearm allowlist; unknown items never become "other".
constexpr GunCategory gunCategory(unsigned definition) noexcept
{
    const auto category = weaponCategory(definition);
    return category == WeaponCategory::Unknown  ? GunCategory::Unsupported
           : category == WeaponCategory::Sniper ? GunCategory::Sniper
                                                : GunCategory::Other;
}
inline constexpr double DefaultSniperInaccuracyLimit = 0.004;
struct WeaponFilter
{
    bool sniper = true, other = true;
    bool operator==(const WeaponFilter &) const = default;
    bool allows(GunCategory category) const noexcept
    {
        return category == GunCategory::Sniper ? sniper : category == GunCategory::Other && other;
    }
};
} // namespace mousebox::games::cs2::trigger
