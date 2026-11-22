#include "games/pubg/pipeline/native_aim.hpp"
#include "games/pubg/pipeline/latency_prediction.hpp"
#include "games/pubg/model/native_aim_profiles.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace mousebox::control
{
namespace
{
constexpr double rad = std::numbers::pi / 180, deg = 180 / std::numbers::pi;
double dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec3 cross(Vec3 a, Vec3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
Vec3 scale(Vec3 a, double t)
{
    return {a.x * t, a.y * t, a.z * t};
}
Vec3 add(Vec3 a, Vec3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
std::optional<Vec3> unit(Vec3 v)
{
    const auto n = std::hypot(v.x, v.y, v.z);
    return finite(v) && std::isfinite(n) && n > 1e-9 ? std::optional(scale(v, 1 / n)) : std::nullopt;
}
Vec3 rotate(Vec3 v, Vec3 axis, double angle)
{
    return add(add(scale(v, std::cos(angle)), scale(cross(axis, v), std::sin(angle))),
               scale(axis, dot(axis, v) * (1 - std::cos(angle))));
}
bool same(Identity a, Identity b)
{
    return sameLifecycle(a, b);
}
std::string key(Identity v)
{
    return lifecycleKey(v);
}
} // namespace
std::optional<Vec3> directionCoordinates(Vec3 value) noexcept
{
    const auto v = unit(value);
    if (!v || std::hypot(v->x, v->y) < 0.02)
        return {};
    return Vec3{std::atan2(v->z, std::hypot(v->x, v->y)) * deg, std::atan2(v->y, v->x) * deg, 0};
}
Vec3 coordinateDirection(double pitch, double yaw) noexcept
{
    return {std::cos(pitch * rad) * std::cos(yaw * rad), std::cos(pitch * rad) * std::sin(yaw * rad),
            std::sin(pitch * rad)};
}
Vec3 nativeInputEffect(Vec3 v, double yaw, double dyaw, double dpitch) noexcept
{
    const Vec3 pitchAxis{std::sin(yaw * rad), -std::cos(yaw * rad), 0};
    return rotate(rotate(v, pitchAxis, dpitch * rad), {0, 0, 1}, dyaw * rad);
}
std::optional<AngularError> nativeInputError(Vec3 forward, Vec3 desired, double yaw) noexcept
{
    const auto a = unit(forward), d = unit(desired);
    if (!a || !d || !std::isfinite(yaw))
        return {};
    const auto cosine = std::clamp(dot(*a, *d), -1.0, 1.0), angle = std::acos(cosine);
    if (cosine < -0.9999)
        return {};
    const Vec3 pitchAxis{std::sin(yaw * rad), -std::cos(yaw * rad), 0};
    const auto jy = cross({0, 0, 1}, *a), jp = cross(pitchAxis, *a);
    const double yy = dot(jy, jy), pp = dot(jp, jp), yp = dot(jy, jp), det = yy * pp - yp * yp;
    if (!std::isfinite(det) || yy < 0.0004 || pp < 0.0004 || det < 1e-5)
        return {};
    Vec3 tangent{};
    if (angle > 1e-10)
    {
        const auto sine = std::sin(angle);
        if (std::abs(sine) < 1e-10)
            return {};
        tangent = scale(add(*d, scale(*a, -cosine)), angle / sine);
    }
    const double by = dot(jy, tangent), bp = dot(jp, tangent);
    const double horizontal = (by * pp - bp * yp) / det * deg, vertical = (bp * yy - by * yp) / det * deg;
    if (!std::isfinite(horizontal) || !std::isfinite(vertical))
        return {};
    return AngularError{horizontal, vertical, angle * deg, 0};
}
std::span<const unsigned> nativeAimRequiredFields(FrameContract contract) noexcept
{
    static constexpr unsigned native[]{1, 2, 3, 4, 5, 6, 7, 8, 12, 13, 14, 15};
    static constexpr unsigned consumer[]{1, 2, 3, 6, 7, 8, 12, 13, 14, 15, 16};
    return contract == FrameContract::ConsumerV3 ? std::span<const unsigned>(consumer)
                                                 : std::span<const unsigned>(native);
}
NativeAimEvaluation qualifyNativeAim(const Frame &view, Identity expected, std::uint64_t now)
{
    NativeAimEvaluation out;
    const auto fail = [&](NativeAimBlock block) {
        out.block = block;
        return out;
    };
    if (!view.nativeAimBasis || !view.nativeAimBasis->direction)
        return out;
    const auto &b = *view.nativeAimBasis;
    if (!view.header.current || !view.header.world || !view.header.level ||
        view.header.convention != "UNREAL_LEFT_HANDED_Z_UP")
        return fail(NativeAimBlock::Context);
    if (!b.observation.current || b.observation.capture != view.header.capture || !view.self || !b.character ||
        !same(*view.self, *b.character) || !view.activeWeapon || !b.weapon || !same(expected, *b.weapon) ||
        !same(view.activeWeapon->instance, *b.weapon) || b.observation.contract != view.header.contract)
        return fail(NativeAimBlock::Identity);
    const bool consumer = view.header.contract == FrameContract::ConsumerV3;
    if (consumer)
    {
        if (view.self->kind != IdentityKind::EntityToken || expected.kind != IdentityKind::WeaponToken ||
            !hasLifecycle(*view.self) || !hasLifecycle(expected) || !b.nativeContextToken || !*b.nativeContextToken ||
            !b.controllerToken || !*b.controllerToken || b.controllerToken != view.controller)
            return fail(NativeAimBlock::Identity);
    }
    else if (!b.mesh || !b.gunData || !b.trajectoryConfig)
        return fail(NativeAimBlock::Identity);
    if (b.branch != 1 || b.stage != 1 || !b.nativeZeroingApplied || !knownBarrelAimProfile(b.nativeProfile) ||
        !b.zeroingDistanceMeters || !b.zeroingCorrectionDegrees || !std::isfinite(*b.zeroingDistanceMeters) ||
        *b.zeroingDistanceMeters <= 0 || !std::isfinite(*b.zeroingCorrectionDegrees))
        return fail(NativeAimBlock::Branch);
    for (const auto id : nativeAimRequiredFields(view.header.contract))
    {
        const auto it = b.observation.fields.find(id);
        if (it == b.observation.fields.end() || it->second.capture != view.header.capture ||
            !estimateSampleTime(it->second.startNs, it->second.finishNs, view.header.sourceClock, now).usable())
            return fail(NativeAimBlock::Timing);
    }
    if (!view.controllerRotation || !view.controller || !finite(view.controllerRotation->rotation))
        return fail(NativeAimBlock::Controller);
    const auto &c = *view.controllerRotation;
    if (consumer ? !same(c.controller, Identity::controller(*view.controller))
                 : c.controller.kind != IdentityKind::NativeLifecycle || c.controller.actor != *view.controller)
        return fail(NativeAimBlock::Controller);
    const auto source = b.observation.fields.at(6);
    const auto ct = estimateSampleTime(c.sourceStartNs, c.sourceFinishNs, view.header.sourceClock, now);
    const auto bt = estimateSampleTime(source.startNs, source.finishNs, view.header.sourceClock, now);
    // Bounded source skew is a model qualification, not an atomic-snapshot claim.
    if (!ct.usable() || !bt.usable() || std::abs(ct.localSampleMs - bt.localSampleMs) > 25)
        return fail(NativeAimBlock::Timing);
    if (!directionCoordinates(*b.direction) || !nativeInputError(*b.direction, *b.direction, c.rotation.y))
        return fail(NativeAimBlock::AxisSingular);
    auto context = b.nativeProfile + "/B=" + key(*b.character) + "/W=" + key(*b.weapon) + "/C=" + key(c.controller);
    context += consumer ? "/N=" + std::to_string(*b.nativeContextToken)
                        : "/M=" + key(*b.mesh) + "/G=" + key(*b.gunData) + "/T=" + key(*b.trajectoryConfig);
    context += "/Z=" + std::to_string(b.zeroingDistanceMeters.value_or(-1));
    out.reference =
        NativeAimReference{*unit(*b.direction), c.rotation.y, source.startNs, source.finishNs, std::move(context)};
    out.block = NativeAimBlock::Ready;
    return out;
}
} // namespace mousebox::control
