#pragma once
#include "games/cs2/model/snapshot.hpp"
#include "games/cs2/model/weapon_category.hpp"
namespace mousebox::games::cs2
{
struct WeaponAimParts
{
    // -1 inherits the general part; 0..4 are the existing semantic body parts.
    int pistol = 0, rifle = 1, smg = 1, sniper = 2;
    bool operator==(const WeaponAimParts &) const = default;
    int forCategory(WeaponCategory) const noexcept;
};
unsigned resolveAimPart(const Snapshot &, const Body &self, unsigned fallback, const WeaponAimParts &) noexcept;
} // namespace mousebox::games::cs2
