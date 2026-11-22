#include "games/pubg/pipeline/target_selector.hpp"
#include "games/pubg/pipeline/latency_prediction.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace mousebox::control
{
bool validAimDistance(double meters) noexcept
{
    return std::isfinite(meters) && meters > 0 && meters <= MaximumAimDistanceSettingMeters;
}
bool validTargetPriority(TargetPriority priority) noexcept
{
    return priority == TargetPriority::NearestDistance || priority == TargetPriority::ClosestToCrosshair;
}
std::string_view targetPriorityKey(TargetPriority priority) noexcept
{
    switch (priority)
    {
    case TargetPriority::NearestDistance:
        return "nearest_distance";
    case TargetPriority::ClosestToCrosshair:
        return "closest_crosshair";
    }
    return {};
}
namespace
{
struct AimPoint
{
    Vec3 position, origin;
    std::int64_t sourceStartNs = 0, sourceFinishNs = 0;
};
std::optional<AimPoint> aimPoint(const Character &character, AimPart part)
{
    const auto name = aimBoneName(part);
    std::optional<AimPoint> found;
    for (const auto &bone : character.bones)
        if (bone.name == name)
        {
            if (found)
                return {}; // Ambiguous semantic names cannot select a guessed native index.
            found = AimPoint{bone.position, bone.origin, bone.sourceStartNs, bone.sourceFinishNs};
        }
    if (!found && part == AimPart::Head && character.head && character.headOrigin)
        found = AimPoint{*character.head, *character.headOrigin}; // Existing head-only offline fixtures.
    return found;
}
} // namespace
std::optional<Target> TargetSelector::select(const Frame &l, const Frame &v, PairVerdict pair, TargetPolicy p,
                                             std::uint64_t now)
{
    stats_ = {};
    if (!std::isfinite(p.maximumConeDegrees) || p.maximumConeDegrees <= 0 || p.maximumConeDegrees >= 90 ||
        !validAimDistance(p.maximumDistanceMeters) || aimBoneName(p.part).empty() || !validTargetPriority(p.priority) ||
        !validLivingLocalSkew(p.livingLocalSkewMs))
        throw std::invalid_argument("Invalid target policy");
    if (!pair.usable())
    {
        clear();
        return {};
    }
    std::optional<Target> best;
    const auto camera = predictCameraPosition(l, v, now, p.livingLocalSkewMs);
    Frame alignedView;
    alignedView.camera = camera.applied() ? std::optional(camera.position) : v.camera;
    alignedView.cameraOrigin = v.cameraOrigin;
    alignedView.rotation = v.rotation;
    for (const auto &c : l.characters)
    {
        if (p.requiredIdentity && c.identity != *p.requiredIdentity)
            continue;
        if (!l.self || c.identity == *l.self || sameNativeAddress(c.identity, *l.self))
        {
            ++stats_.self;
            continue;
        }
        const auto point = aimPoint(c, p.part);
        if (!point)
        {
            ++stats_.missingAimPoint;
            continue;
        }
        if (p.requireTeam && (!l.selfTeam || !c.team))
        {
            ++stats_.missingTeam;
            continue;
        }
        if (l.selfTeam && c.team && l.selfTeam == c.team)
        {
            ++stats_.sameTeam;
            continue;
        }
        Target candidate;
        candidate.identity = c.identity;
        candidate.position = point->position;
        candidate.origin = point->origin;
        candidate.sourceStartNs = point->sourceStartNs ? point->sourceStartNs : l.header.sourceStartNs;
        candidate.sourceFinishNs = point->sourceFinishNs ? point->sourceFinishNs : l.header.sourceFinishNs;
        candidate.bodyWorldVelocity = c.bodyWorldVelocity;
        candidate.sourceClock = l.header.sourceClock;
        candidate.nativeMovementExpected = c.nativeMovementExpected;
        const auto prediction = predictPosition(candidate, now);
        // Translating only the enemy against an old camera invents an angular
        // offset when either player rides a vehicle. Keep the existing fallback
        // when self timing is absent; final ballistics still enforces all gates.
        const auto error = angleTo(prediction.applied() ? alignedView : v,
                                   prediction.applied() ? prediction.position : point->position, point->origin);
        if (!error)
        {
            ++stats_.invalidGeometry;
            continue;
        }
        if (error->cone > p.maximumConeDegrees)
        {
            ++stats_.outsideCone;
            continue;
        }
        if (error->distanceUnits > p.maximumDistanceMeters * WorldUnitsPerMeter + 1e-6)
        {
            ++stats_.outsideDistance;
            continue;
        }
        ++stats_.candidates;
        candidate.error = *error;
        // Sorting only ranks eligible candidates. A held identity is filtered
        // above, so neither priority can transfer the lock to another person.
        const double difference = best ? candidate.error.distanceUnits - best->error.distanceUnits : 0;
        const bool sameDistance = std::abs(difference) <= 1e-6;
        const bool angleWins =
            best &&
            (candidate.error.cone < best->error.cone - 1e-9 ||
             (std::abs(candidate.error.cone - best->error.cone) <= 1e-9 && candidate.identity.id < best->identity.id));
        const double angleDifference = best ? candidate.error.cone - best->error.cone : 0;
        const bool crosshairWins =
            best && (angleDifference < -1e-9 ||
                     (std::abs(angleDifference) <= 1e-9 &&
                      (difference < -1e-6 || (sameDistance && candidate.identity.id < best->identity.id))));
        if (!best ||
            (p.priority == TargetPriority::ClosestToCrosshair ? crosshairWins
                                                              : difference < -1e-6 || (sameDistance && angleWins)))
            best = candidate;
    }
    return best;
}
} // namespace mousebox::control
