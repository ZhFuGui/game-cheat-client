#pragma once
#include <array>
#include <optional>
#include <string_view>

namespace mousebox::control
{
enum class BoneRole : unsigned
{
    Pelvis = 1,
    Spine01,
    Spine02,
    Spine03,
    Neck,
    Head,
    ClavicleLeft,
    UpperArmLeft,
    LowerArmLeft,
    HandLeft,
    ClavicleRight,
    UpperArmRight,
    LowerArmRight,
    HandRight,
    ThighLeft,
    CalfLeft,
    FootLeft,
    ThighRight,
    CalfRight,
    FootRight
};
inline constexpr std::array<std::string_view, 20> BoneRoleNames{
    "pelvis",     "spine_01",   "spine_02", "spine_03",   "neck_01",    "head",       "clavicle_l",
    "upperarm_l", "lowerarm_l", "hand_l",   "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r",
    "thigh_l",    "calf_l",     "foot_l",   "thigh_r",    "calf_r",     "foot_r"};
inline std::string_view boneRoleName(BoneRole value) noexcept
{
    const auto role = static_cast<unsigned>(value);
    return role > 0 && role <= BoneRoleNames.size() ? BoneRoleNames[role - 1] : std::string_view{};
}
inline std::optional<BoneRole> boneRole(unsigned value) noexcept
{
    return value > 0 && value <= BoneRoleNames.size() ? std::optional(static_cast<BoneRole>(value)) : std::nullopt;
}
} // namespace mousebox::control
