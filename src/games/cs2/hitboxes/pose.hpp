#pragma once
#include "assets.hpp"
#include <array>
#include <optional>
#include <vector>

namespace mousebox::games::cs2::hitboxes
{
struct BoneTransform
{
    unsigned index = 0;
    Vec3 position;
    std::optional<std::array<double, 4>> rotation; // World quaternion, x/y/z/w.
    std::optional<double> scale;                   // Uniform world scale, not applied twice.
};
struct PosedHitbox
{
    const Definition *definition = nullptr;
    BoneTransform bone;
    Vec3 point(Vec3 local) const noexcept;
    // A single interior reference point, transformed with the same pose as
    // the actual hitbox. This does not sample or search the surface.
    Vec3 center() const noexcept;
};
struct Pose
{
    const ModelAsset *model = nullptr;
    std::vector<PosedHitbox> hitboxes;
    unsigned missing = 0, invalid = 0;
};
std::optional<PosedHitbox> poseHitbox(const Definition &, BoneTransform);
Pose prepare(const ModelAsset *, std::span<const BoneTransform>);
} // namespace mousebox::games::cs2::hitboxes
