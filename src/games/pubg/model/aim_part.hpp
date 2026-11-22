#pragma once
#include <string_view>
namespace mousebox::control
{
enum class AimPart
{
    Head,
    Neck,
    Chest,
    Abdomen,
    Pelvis
};
constexpr std::string_view aimBoneName(AimPart part) noexcept
{
    switch (part)
    {
    case AimPart::Head:
        return "head";
    case AimPart::Neck:
        return "neck_01";
    case AimPart::Chest:
        return "spine_03";
    case AimPart::Abdomen:
        return "spine_01";
    case AimPart::Pelvis:
        return "pelvis";
    }
    return {};
}
} // namespace mousebox::control
