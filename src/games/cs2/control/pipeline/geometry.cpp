#include "geometry.hpp"
#include "angles.hpp"
#include <numbers>
namespace mousebox::games::cs2::pipeline
{
std::optional<Vec3> aimPoint(const Body &b, unsigned part)
{
    if (b.skeletonStatus != SkeletonStatus::VerifiedComplete && b.skeletonStatus != SkeletonStatus::VerifiedPartial)
        return {};
    constexpr BoneRole roles[]{BoneRole::Head, BoneRole::Neck, BoneRole::Spine3, BoneRole::Spine1, BoneRole::Pelvis};
    if (part >= std::size(roles))
        return {};
    for (const auto &bone : b.bones)
        if (bone.role == roles[part] && finite(bone.position))
            return bone.position;
    return {};
}
bool onScreen(const Snapshot &f, Vec3 p)
{
    if (!f.local.matrix)
        return false;
    const auto &m = *f.local.matrix;
    const double x = m[0] * p.x + m[1] * p.y + m[2] * p.z + m[3], y = m[4] * p.x + m[5] * p.y + m[6] * p.z + m[7],
                 w = m[12] * p.x + m[13] * p.y + m[14] * p.z + m[15];
    return std::isfinite(w) && w > .001 && std::abs(x) <= w && std::abs(y) <= w;
}
TargetGeometry predictGeometry(Vec3 point, Vec3 eye, Vec3 target, Vec3 self, double horizon, bool prediction,
                               GeometryFactors factors)
{
    if (prediction)
    {
        const double seconds = horizon / 1000;
        if (factors.targetVelocity)
        {
            point.x += target.x * seconds;
            point.y += target.y * seconds;
            point.z += target.z * seconds;
        }
        if (factors.selfVelocity)
        {
            eye.x += self.x * seconds;
            eye.y += self.y * seconds;
            eye.z += self.z * seconds;
        }
    }
    const auto dx = point.x - eye.x, dy = point.y - eye.y, dz = point.z - eye.z;
    constexpr double degrees = 180 / std::numbers::pi;
    return {point, {-std::atan2(dz, std::hypot(dx, dy)) * degrees, std::atan2(dy, dx) * degrees, 0}, eye};
}
} // namespace mousebox::games::cs2::pipeline
