#pragma once
#include "model/geometry.hpp"
#include <cstring>
#include <optional>

namespace monitor::unreal {
inline Transform UnpackTransform(const u8* bytes) noexcept {
    Transform value;
    std::memcpy(&value.rotation, bytes, 16);
    std::memcpy(&value.translation, bytes + 16, 12);
    std::memcpy(&value.scale, bytes + 32, 12);
    return value;
}
inline Quaternion Multiply(const Quaternion& a, const Quaternion& b) noexcept {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}
// UE FRotator(Pitch,Yaw,Roll)，单位度；不是XYZ欧拉角直接依次相乘。
inline Quaternion RotatorQuaternion(const Vec3& degrees) noexcept {
    constexpr double half = 3.14159265358979323846 / 360.0;
    const auto p = std::remainder(double(degrees.x), 360.0) * half;
    const auto y = std::remainder(double(degrees.y), 360.0) * half;
    const auto r = std::remainder(double(degrees.z), 360.0) * half;
    const auto sp = std::sin(p), cp = std::cos(p), sy = std::sin(y), cy = std::cos(y), sr = std::sin(r),
               cr = std::cos(r);
    return {float(cr * sp * sy - sr * cp * cy), float(-cr * sp * cy - sr * cp * sy), float(cr * cp * sy - sr * sp * cy),
            float(cr * cp * cy + sr * sp * sy)};
}
// 与UE的 Local * Parent FTransform语义一致。负缩放使用矩阵分支，不能直接相乘四元数。
inline std::optional<Transform> ComposeTransform(const Transform& local, const Transform& parent) noexcept {
    if (!ValidTransform(local) || !ValidTransform(parent))
        return {};
    Transform result{Multiply(parent.rotation, local.rotation),
                     TransformPosition(parent, local.translation),
                     {local.scale.x * parent.scale.x, local.scale.y * parent.scale.y, local.scale.z * parent.scale.z}};
    if (local.scale.x < 0 || local.scale.y < 0 || local.scale.z < 0 || parent.scale.x < 0 || parent.scale.y < 0 ||
        parent.scale.z < 0) {
        Transform a = local, b = parent;
        a.translation = b.translation = {};
        const Vec3 basis[3]{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        double matrix[3][3]{};
        const float desired[3]{result.scale.x, result.scale.y, result.scale.z};
        for (unsigned axis = 0; axis < 3; ++axis) {
            const auto v = TransformPosition(b, TransformPosition(a, basis[axis]));
            const double length = std::sqrt(double(v.x) * v.x + double(v.y) * v.y + double(v.z) * v.z);
            if (!std::isfinite(length) || length <= 1e-8 || desired[axis] == 0)
                return {}; // 退化镜像基无法可靠恢复朝向，不补造单位四元数。
            const double scale = (desired[axis] < 0 ? -1.0 : 1.0) / length;
            matrix[0][axis] = v.x * scale;
            matrix[1][axis] = v.y * scale;
            matrix[2][axis] = v.z * scale;
        }
        double q[4]{};
        const double trace = matrix[0][0] + matrix[1][1] + matrix[2][2];
        if (trace > 0) {
            const auto s = std::sqrt(trace + 1.0) * 2;
            q[3] = .25 * s;
            q[0] = (matrix[2][1] - matrix[1][2]) / s;
            q[1] = (matrix[0][2] - matrix[2][0]) / s;
            q[2] = (matrix[1][0] - matrix[0][1]) / s;
        } else {
            unsigned i = 0;
            if (matrix[1][1] > matrix[0][0])
                i = 1;
            if (matrix[2][2] > matrix[i][i])
                i = 2;
            const auto j = (i + 1) % 3, k = (i + 2) % 3;
            const auto s = std::sqrt(1 + matrix[i][i] - matrix[j][j] - matrix[k][k]) * 2;
            if (!(s > 0))
                return {};
            q[i] = .25 * s;
            q[j] = (matrix[j][i] + matrix[i][j]) / s;
            q[k] = (matrix[k][i] + matrix[i][k]) / s;
            q[3] = (matrix[k][j] - matrix[j][k]) / s;
        }
        const auto norm = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
        if (!(norm > 0) || !std::isfinite(norm))
            return {};
        result.rotation = {float(q[0] / norm), float(q[1] / norm), float(q[2] / norm), float(q[3] / norm)};
    }
    return ValidTransform(result) ? std::optional<Transform>(result) : std::nullopt;
}
} // namespace monitor::unreal
