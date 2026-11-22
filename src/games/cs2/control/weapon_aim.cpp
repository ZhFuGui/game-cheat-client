#include "weapon_aim.hpp"
namespace mousebox::games::cs2
{
int WeaponAimParts::forCategory(WeaponCategory category) const noexcept
{
    switch (category)
    {
    case WeaponCategory::Pistol:
        return pistol;
    case WeaponCategory::Rifle:
        return rifle;
    case WeaponCategory::Smg:
        return smg;
    case WeaponCategory::Sniper:
        return sniper;
    default:
        return -1;
    }
}
unsigned resolveAimPart(const Snapshot &frame, const Body &self, unsigned fallback,
                        const WeaponAimParts &parts) noexcept
{
    if (!self.activeWeapon || *self.activeWeapon == 0 || *self.activeWeapon == 0xffffffffU)
        return fallback;
    const Weapon *held = nullptr;
    for (const auto &weapon : frame.weapons)
        if (weapon.entity.id == *self.activeWeapon)
        {
            // Match the full handle from this batch. Ambiguous/missing metadata
            // uses the general choice, never a cached weapon or a guessed class.
            if (held)
                return fallback;
            held = &weapon;
        }
    if (!held || !held->definition)
        return fallback;
    const auto part = parts.forCategory(weaponCategory(*held->definition));
    return part >= 0 && part <= 4 ? static_cast<unsigned>(part) : fallback;
}
} // namespace mousebox::games::cs2
