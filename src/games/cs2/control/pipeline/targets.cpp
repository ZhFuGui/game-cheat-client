#include "targets.hpp"
#include "angles.hpp"
#include <limits>
namespace mousebox::games::cs2::pipeline
{
namespace
{
struct RankedPoint
{
    TargetGeometry geometry;
    Angles error;
    double distance, score;
};
} // namespace

void selectTarget(AimSolution &r, const Snapshot &f, const Body &self, const AimSettings &s,
                  std::optional<TargetKey> held, std::optional<TargetPoint> supplied, const AimPointCheck &check,
                  const AimPointResolver &resolvePoint)
{
    std::optional<unsigned> team;
    for (const auto &p : f.players)
        if (p.controller == *f.local.controller)
            team = p.team ? p.team : self.entity.team;
    if (!team || *team < 2 || *team > 3)
    {
        r.block = "team-unknown";
        return;
    }
    if (s.prediction && (!self.entity.velocity || !finite(*self.entity.velocity)))
    {
        r.block = "local-velocity-missing";
        return;
    }
    double best = std::numeric_limits<double>::infinity();
    const auto part = resolveAimPart(f, self, s.part, s.weaponParts);
    r.block = held ? "held-target-missing" : "no-target";
    for (const auto &p : f.players)
    {
        if (!p.body || p.controller == *f.local.controller)
            continue;
        const auto &b = *p.body;
        const TargetKey key{p.controller, b.entity.id};
        if (held && key != *held)
            continue;
        if (held && (p.alive == false || (b.entity.health && *b.entity.health <= 0) ||
                     (b.entity.lifeState && *b.entity.lifeState != 0)))
        {
            r.block = "held-target-dead";
            continue;
        }
        const auto targetTeam = p.team ? p.team : b.entity.team;
        if (!alive(p, b) || !targetTeam || !s.targets.allows(*team, *targetTeam))
            continue;
        if (s.prediction && (!b.entity.velocity || !finite(*b.entity.velocity)))
        {
            if (held)
                r.block = "target-velocity-missing";
            continue;
        }
        const auto eye = *f.local.eye;
        const auto project = [&](Vec3 point) -> std::optional<RankedPoint> {
            if (!held && !onScreen(f, point))
                return {};
            const auto distance = std::hypot(point.x - eye.x, point.y - eye.y, point.z - eye.z);
            if (!std::isfinite(distance) || distance < .001 ||
                (!supplied && s.maximumDistanceUnits > 0 && distance > s.maximumDistanceUnits))
                return {};
            const auto geometry = predictGeometry(point, eye, b.entity.velocity.value_or(Vec3{}),
                                                  self.entity.velocity.value_or(Vec3{}), r.horizonMs, s.prediction);
            const auto error = angularError(geometry.desired, *r.direction);
            const auto angular = std::hypot(error.x, error.y);
            if (!finite(geometry.desired) || !finite(error) || (!supplied && angular > s.coneDegrees) ||
                std::abs(geometry.desired.x) >= 89)
                return {};
            return RankedPoint{geometry, error, distance, s.priority == 0 ? distance : angular};
        };
        std::optional<RankedPoint> ranked;
        AimPointProbe probe;
        if (resolvePoint && check && !supplied)
            probe = [&](Vec3 point) -> std::string {
                const auto trial = project(point);
                if (!trial)
                    return {};
                auto status = check(trial->geometry.origin, trial->geometry.point);
                // The resolver returns the latest accepted point, including when later trials are rejected.
                if (status == "ready")
                    ranked = trial;
                return status;
            };
        std::optional<AimCandidatePoint> candidate;
        if (supplied)
            candidate = AimCandidatePoint{supplied->point, supplied->id, {}};
        else if (resolvePoint)
        {
            auto resolution = resolvePoint(b, key, part, probe);
            candidate = resolution.candidate;
            if (!candidate)
            {
                if (resolution.block == "cover-pending")
                {
                    r.block = std::move(resolution.block);
                    return;
                }
                if (!r.target && !resolution.block.empty() &&
                    (resolution.block != "aim-hitbox-unavailable" || r.block == "no-target" ||
                     r.block == "held-target-missing"))
                    r.block = std::move(resolution.block);
                continue;
            }
        }
        else if (const auto bone = aimPoint(b, part))
            candidate = AimCandidatePoint{*bone, {}, {}};
        if (!candidate)
            continue;
        if (!probe)
            ranked = project(candidate->position);
        if (!ranked)
            continue;
        const auto &[geometry, error, distance, score] = *ranked;
        if (score < best || (score == best && (!r.target || key.controller < r.target->controller)))
        {
            if (check && !probe)
            {
                const auto status = check(geometry.origin, geometry.point);
                if (status != "ready")
                {
                    // A map may finish loading between two candidates. Wait for
                    // the next solve rather than selecting a lower-priority
                    // person solely because its check happened after loading.
                    if (status == "cover-pending")
                    {
                        r.block = status;
                        return;
                    }
                    if (!r.target)
                        r.block = status;
                    continue;
                }
            }
            best = score;
            r.block = "ready";
            r.target = key;
            r.pointId = candidate->hitboxId;
            r.pointSeed = candidate->sampleSeed;
            r.sourcePoint = candidate->position;
            r.targetName = p.name.value_or(b.entity.name);
            r.point = geometry.point;
            r.origin = geometry.origin;
            r.distanceUnits = distance;
            r.desired = geometry.desired;
            r.error = error;
        }
    }
    if (!r.inputControllable)
        r.block = "eye-input-transform-unverified";
}
} // namespace mousebox::games::cs2::pipeline
