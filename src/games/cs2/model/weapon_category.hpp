#pragma once
namespace mousebox::games::cs2
{
enum class WeaponCategory
{
    Unknown,
    Pistol,
    Rifle,
    Smg,
    Sniper,
    Shotgun,
    MachineGun
};
// Reviewed Valve item definitions, shared by consumers without guessing names.
constexpr WeaponCategory weaponCategory(unsigned definition) noexcept
{
    switch (definition)
    {
    case 1:
    case 2:
    case 3:
    case 4:
    case 30:
    case 32:
    case 36:
    case 61:
    case 63:
    case 64:
        return WeaponCategory::Pistol;
    case 7:
    case 8:
    case 10:
    case 13:
    case 16:
    case 39:
    case 60:
        return WeaponCategory::Rifle;
    case 17:
    case 19:
    case 23:
    case 24:
    case 26:
    case 33:
    case 34:
        return WeaponCategory::Smg;
    case 9:
    case 11:
    case 38:
    case 40:
        return WeaponCategory::Sniper;
    case 25:
    case 27:
    case 29:
    case 35:
        return WeaponCategory::Shotgun;
    case 14:
    case 28:
        return WeaponCategory::MachineGun;
    default:
        return WeaponCategory::Unknown;
    }
}
} // namespace mousebox::games::cs2
