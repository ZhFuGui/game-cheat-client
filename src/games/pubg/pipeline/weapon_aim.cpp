#include "games/pubg/model/weapon_aim.hpp"
#include "games/pubg/model/control_frame.hpp"
#include "control/ballistics.hpp"
#include <algorithm>

namespace mousebox::control
{
namespace
{
struct WeaponCategoryRecord
{
    std::string_view profileId;
    WeaponCategory category;
};
constexpr WeaponCategoryRecord records[]{
#include "games/pubg/data/weapon_categories.inc"
};
} // namespace
bool validWeaponAimPolicy(const WeaponAimPolicy &policy) noexcept
{
    return std::all_of(policy.parts.begin(), policy.parts.end(),
                       [](auto part) { return !part || !aimBoneName(*part).empty(); });
}
bool weaponAimSetting(std::string_view key) noexcept
{
    return std::any_of(WeaponAimCategories.begin(), WeaponAimCategories.end(),
                       [&](const auto &entry) { return entry.setting == key; });
}
std::string_view weaponCategoryKey(WeaponCategory category) noexcept
{
    for (const auto &entry : WeaponAimCategories)
        if (entry.category == category)
            return entry.key;
    return {};
}
WeaponAimSelection resolveWeaponAim(const Frame &frame, const BallisticCatalog *catalog, AimPart fallback,
                                    const WeaponAimPolicy &policy) noexcept
{
    WeaponAimSelection result{fallback};
    if (!catalog || !frame.header.current || !frame.self || !frame.activeWeapon ||
        frame.activeWeapon->owner != *frame.self || !hasLifecycle(frame.activeWeapon->instance) ||
        frame.activeWeapon->slot < 0)
        return result;
    const auto &weapon = *frame.activeWeapon;
    const auto matched = catalog->find(weapon.actorClass, weapon.itemId);
    if (!matched.entry || matched.status == ProfileMatchStatus::Ambiguous)
        return result;
    const auto &id = matched.entry->profile.id;
    const auto found = std::lower_bound(std::begin(records), std::end(records), std::string_view(id),
                                        [](const auto &entry, auto key) { return entry.profileId < key; });
    if (found == std::end(records) || found->profileId != id)
        return result; // Unknown/custom weapons keep the explicitly configured fallback.
    result.category = found->category;
    const auto part = policy.parts[static_cast<unsigned>(found->category)];
    if (part && !aimBoneName(*part).empty())
    {
        result.part = *part;
        result.overridden = true;
    }
    return result;
}
} // namespace mousebox::control
