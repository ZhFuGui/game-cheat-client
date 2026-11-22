#pragma once
#include "model/primitives.hpp"
#include <cmath>

namespace monitor {
struct Quaternion {
    float x = 0, y = 0, z = 0, w = 1;
};
// DTO 不包含 UE 的 SIMD 对齐填充；内存布局在 game 层显式解包。
struct Transform {
    Quaternion rotation;
    Vec3 translation;
    Vec3 scale{1, 1, 1};
};
inline bool Finite(const Vec3& p) noexcept {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
inline bool ValidTransform(const Transform& t) noexcept {
    const auto& q = t.rotation;
    const float norm = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
    return Finite(t.translation) && Finite(t.scale) && std::isfinite(norm) && std::abs(norm - 1.f) <= .01f;
}
// Unreal TransformPosition：先缩放，再绕四元数旋转，最后平移；保留零/负缩放。
inline Vec3 TransformPosition(const Transform& t, const Vec3& p) noexcept {
    const Vec3 v{p.x * t.scale.x, p.y * t.scale.y, p.z * t.scale.z};
    const auto& q = t.rotation;
    const Vec3 c{2 * (q.y * v.z - q.z * v.y), 2 * (q.z * v.x - q.x * v.z), 2 * (q.x * v.y - q.y * v.x)};
    return {t.translation.x + v.x + q.w * c.x + q.y * c.z - q.z * c.y,
            t.translation.y + v.y + q.w * c.y + q.z * c.x - q.x * c.z,
            t.translation.z + v.z + q.w * c.z + q.x * c.y - q.y * c.x};
}
} // namespace monitor
