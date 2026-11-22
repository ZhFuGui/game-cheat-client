#include "pose.hpp"
#include <cmath>
#include <map>
#include <set>

namespace mousebox::games::cs2::hitboxes
{
namespace
{
bool finite(Vec3 p)
{
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
Vec3 cross(Vec3 a, Vec3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
} // namespace
Vec3 PosedHitbox::point(Vec3 local) const noexcept
{
    if (!definition->translationOnly)
    {
        const auto &q = *bone.rotation;
        local = {local.x * *bone.scale, local.y * *bone.scale, local.z * *bone.scale};
        const Vec3 axis{q[0], q[1], q[2]};
        const auto t = cross(axis, local);
        const auto u = cross(axis, t);
        local = {local.x + 2 * (q[3] * t.x + u.x), local.y + 2 * (q[3] * t.y + u.y), local.z + 2 * (q[3] * t.z + u.z)};
    }
    return {bone.position.x + local.x, bone.position.y + local.y, bone.position.z + local.z};
}
Vec3 PosedHitbox::center() const noexcept
{
    const auto &a = definition->a, &b = definition->b;
    return point(definition->shape == Shape::Sphere ? a : Vec3{(a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2});
}
std::optional<PosedHitbox> poseHitbox(const Definition &definition, BoneTransform bone)
{
    if (!finite(bone.position) || bone.index != definition.bone)
        return {};
    if (!definition.translationOnly)
    {
        if (!bone.rotation || !bone.scale || !std::isfinite(*bone.scale))
            return {};
        double norm = 0;
        for (const auto component : *bone.rotation)
            norm += component * component;
        if (!std::isfinite(norm) || std::abs(norm - 1) > .001)
            return {};
    }
    return PosedHitbox{&definition, std::move(bone)};
}
Pose prepare(const ModelAsset *model, std::span<const BoneTransform> bones)
{
    Pose result;
    result.model = model;
    if (!model)
        return result;
    std::map<unsigned, const BoneTransform *> indexed;
    std::set<unsigned> duplicates;
    for (const auto &bone : bones)
        if (!indexed.emplace(bone.index, &bone).second)
            duplicates.insert(bone.index);
    for (const auto &definition : model->definitions)
    {
        const auto found = indexed.find(definition.bone);
        if (found == indexed.end() ||
            (!definition.translationOnly && (!found->second->rotation || !found->second->scale)))
        {
            ++result.missing;
            continue;
        }
        auto posed = poseHitbox(definition, *found->second);
        if (posed && !duplicates.contains(definition.bone))
            result.hitboxes.push_back(std::move(*posed));
        else
            ++result.invalid;
    }
    return result;
}
} // namespace mousebox::games::cs2::hitboxes
