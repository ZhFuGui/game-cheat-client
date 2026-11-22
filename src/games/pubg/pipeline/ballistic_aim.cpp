#include "games/pubg/pipeline/ballistic_aim.hpp"
#include "games/pubg/pipeline/latency_prediction.hpp"
#include "games/pubg/pipeline/target_selector.hpp"
#include "games/pubg/model/ballistics_settings.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <numbers>
namespace mousebox::control
{
static_assert(CurrentBallisticsAssumptions.inheritedShooterVelocity == 0,
              "This reference solver implements the user's no-inheritance rule only.");
static_assert(CurrentBallisticsAssumptions.origin == BallisticOriginRule::Camera,
              "Live ballistics use the user-selected camera origin.");
namespace
{
Vec3 world(Vec3 p, Vec3 o)
{
    return {(p.x + o.x) / WorldUnitsPerMeter, (p.y + o.y) / WorldUnitsPerMeter, (p.z + o.z) / WorldUnitsPerMeter};
}
Vec3 plus(Vec3 a, Vec3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
Vec3 times(Vec3 a, double t)
{
    return {a.x * t, a.y * t, a.z * t};
}
bool proven(const std::optional<BodyWorldVelocity> &v, Identity owner)
{
    return v && v->owner == owner && v->source == BodyVelocitySource::ReaderTotalWorld && finite(v->value);
}
std::string gravityIdentity(double value)
{
    char text[64];
    const auto end = std::to_chars(text, text + sizeof(text), value);
    return std::string(text, end.ptr);
}
} // namespace
BallisticDiagnostic prepareBallisticAim(const Frame &living, const Frame &view, const Target &target,
                                        const BallisticCatalog *catalog, unsigned zeroing, std::uint64_t now,
                                        double gravity, double livingLocalSkewMs)
{
    BallisticDiagnostic result;
    result.gravityMetersPerSecondSquared = gravity;
    if (!validGravity(gravity) || !validZeroingDistance(zeroing) || !validLivingLocalSkew(livingLocalSkewMs))
    {
        result.block = BallisticBlock::InvalidConfiguration;
        return result;
    }
    if (!catalog)
    {
        result.block = BallisticBlock::CatalogMissing;
        return result;
    }
    result.revision = catalog->revision;
    result.sourcePath = catalog->sourcePath;
    result.sourceBuild = catalog->sourceBuild;
    if (!living.self || !living.activeWeapon || living.activeWeapon->owner != *living.self ||
        !hasLifecycle(living.activeWeapon->instance) || living.activeWeapon->slot < 0)
    {
        result.block = BallisticBlock::WeaponMissing;
        return result;
    }
    const auto &weapon = *living.activeWeapon;
    result.weaponClass = weapon.actorClass;
    result.weapon = weapon.instance;
    const auto matched = catalog->find(weapon.actorClass, weapon.itemId);
    if (matched.status != ProfileMatchStatus::Matched)
    {
        result.block = matched.status == ProfileMatchStatus::Unrecognized ? BallisticBlock::UnknownWeapon
                                                                          : BallisticBlock::UnsupportedWeapon;
        return result;
    }
    const auto &entry = *matched.entry;
    result.profileId = entry.profile.id;
    result.profileName = entry.profile.name;
    result.initialSpeed = entry.curve.initialSpeed();
    if (!proven(target.bodyWorldVelocity, target.identity))
    {
        result.block = BallisticBlock::TargetVelocityMissing;
        return result;
    }
    const auto own = std::find_if(living.characters.begin(), living.characters.end(),
                                  [&](const auto &c) { return c.identity == *living.self; });
    if (own == living.characters.end() || !proven(own->bodyWorldVelocity, *living.self))
    {
        result.block = BallisticBlock::SelfVelocityMissing;
        return result;
    }
    if (!view.camera || !view.cameraOrigin || !view.rotation || !finite(*view.camera) || !finite(*view.cameraOrigin) ||
        !finite(*view.rotation))
    {
        result.block = BallisticBlock::CameraMissing;
        return result;
    }
    const auto enemy = predictPosition(target, now);
    const auto camera = predictCameraPosition(living, view, now, livingLocalSkewMs);
    if (!enemy.applied() || !camera.applied() || !target.origin)
    {
        result.block = BallisticBlock::TimingUnavailable;
        result.timingPart = !enemy.applied() || !target.origin ? BallisticDiagnostic::TimingPart::Target
                                                               : BallisticDiagnostic::TimingPart::Camera;
        if (!enemy.applied())
            result.timingReason = enemy.reason;
        else if (!camera.applied())
            result.timingReason = camera.reason;
        return result;
    }
    result.targetAgeMs = enemy.ageMs;
    result.cameraAgeMs = camera.ageMs;
    result.uncertaintyMs = std::max(enemy.uncertaintyMs, camera.uncertaintyMs);
    const auto rotation = *view.rotation;
    constexpr double radians = std::numbers::pi / 180;
    BallisticRequest request;
    request.target = world(enemy.position, *target.origin);
    request.camera = world(camera.position, *view.cameraOrigin);
    request.origin = request.camera;
    result.originWorldMeters = request.origin;
    request.targetVelocity = times(target.bodyWorldVelocity->value, 1 / WorldUnitsPerMeter);
    request.zeroingMeters = zeroing;
    request.gravity = gravity;
    request.sightDirection = {std::cos(rotation.x * radians) * std::cos(rotation.y * radians),
                              std::cos(rotation.x * radians) * std::sin(rotation.y * radians),
                              std::sin(rotation.x * radians)};
    result.solution = solveBallistic(entry.profile, entry.curve, request);
    if (!result.solution.ready())
    {
        result.block = result.solution.status == BallisticStatus::ZeroingUnreachable
                           ? BallisticBlock::ZeroingUnreachable
                           : BallisticBlock::NoSolution;
        return result;
    }
    ResolvedAimPoint aim;
    aim.weapon = weapon.instance;
    aim.modelRevision = entry.profile.id + "/" + catalog->revision + "/" +
                        std::string(CurrentBallisticsAssumptions.revision) + "/g=" + gravityIdentity(gravity) +
                        "/Z=" + std::to_string(zeroing);
    aim.origin = *view.cameraOrigin;
    // A virtual sight point lets the existing camera-feedback controller consume
    // the resolved angle without reapplying target-age or bullet-flight offsets.
    const auto virtualPoint = [&](Vec3 direction) {
        return plus(*view.camera, times(direction, std::max(100.0, target.error.distanceUnits)));
    };
    aim.position = virtualPoint(result.solution.aimDirection);
    aim.launchDirection = result.solution.launchDirection;
    const auto ownVelocity = times(own->bodyWorldVelocity->value, 1 / WorldUnitsPerMeter);
    request.target = plus(request.target, times(request.targetVelocity, 0.02));
    request.camera = plus(request.camera, times(ownVelocity, 0.02));
    request.origin = request.camera;
    const auto next = solveBallistic(entry.profile, entry.curve, request);
    if (next.ready())
    {
        aim.nextPosition = virtualPoint(next.aimDirection);
        aim.nextLaunchDirection = next.launchDirection;
    }
    result.aim = std::move(aim);
    result.block = BallisticBlock::Ready;
    return result;
}
} // namespace mousebox::control
