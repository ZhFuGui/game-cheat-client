#pragma once
#include "games/pubg/model/aim_part.hpp"
#include <array>
#include <optional>

namespace mousebox::control
{
struct Frame;
enum class WeaponCategory : unsigned
{
    Sniper,
    Marksman,
    AssaultRifle,
    SubmachineGun,
    LightMachineGun,
    Shotgun,
    Handgun,
    Other,
    Count
};
struct WeaponAimCategory
{
    WeaponCategory category;
    std::string_view key, setting;
};
inline constexpr std::array WeaponAimCategories{WeaponAimCategory{WeaponCategory::Sniper, "SR", "partSR"},
                                                WeaponAimCategory{WeaponCategory::Marksman, "DMR", "partDMR"},
                                                WeaponAimCategory{WeaponCategory::AssaultRifle, "AR", "partAR"},
                                                WeaponAimCategory{WeaponCategory::SubmachineGun, "SMG", "partSMG"},
                                                WeaponAimCategory{WeaponCategory::LightMachineGun, "LMG", "partLMG"},
                                                WeaponAimCategory{WeaponCategory::Shotgun, "Shotgun", "partShotgun"},
                                                WeaponAimCategory{WeaponCategory::Handgun, "Handgun", "partHandgun"},
                                                WeaponAimCategory{WeaponCategory::Other, "Other", "partOther"}};
struct WeaponAimPolicy
{
    std::array<std::optional<AimPart>, static_cast<unsigned>(WeaponCategory::Count)> parts{};
    bool operator==(const WeaponAimPolicy &) const = default;
};
struct WeaponAimSelection
{
    AimPart part = AimPart::Head;
    std::optional<WeaponCategory> category;
    bool overridden = false;
};
struct BallisticCatalog;
bool validWeaponAimPolicy(const WeaponAimPolicy &) noexcept;
bool weaponAimSetting(std::string_view) noexcept;
std::string_view weaponCategoryKey(WeaponCategory) noexcept;
WeaponAimSelection resolveWeaponAim(const Frame &, const BallisticCatalog *, AimPart, const WeaponAimPolicy &) noexcept;
} // namespace mousebox::control
