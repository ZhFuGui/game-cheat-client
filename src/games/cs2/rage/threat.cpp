#include "threat.hpp"
#include "games/cs2/control/pipeline/angles.hpp"
#include "games/cs2/extensions/head_damage/inputs.hpp"
#include "games/cs2/trigger/weapon_filter.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <set>
namespace mousebox::games::cs2::rage
{
namespace
{
const Player *localPlayer(const Snapshot &f)
{
    for (const auto &p : f.players)
        if (f.local.controller == p.controller && p.body && f.local.pawn == p.body->entity.id)
            return &p;
    return nullptr;
}
hitboxes::Pose localPose(const Snapshot &f)
{
    const auto p = localPlayer(f);
    return p && p->body ? hitboxes::fromBody(f, *p->body) : hitboxes::Pose{};
}
} // namespace
ThreatEvaluator::ThreatEvaluator(const Snapshot &f, const application::ports::BallisticsScene &scene)
    : frame_(f), scene_(scene), local_(localPlayer(f)), pose_(localPose(f)), body_(pose_)
{
    if (local_)
        damageTarget_ = extensions::damageTarget(*local_);
}
ThreatEvidence ThreatEvaluator::evaluate(const Player &p, std::stop_token stop, unsigned &queries) const
{
    using namespace application::ports;
    ThreatEvidence out;
    if (local_ && local_->body && local_->body->entity.health && *local_->body->entity.health > 0)
        out.localHealth = *local_->body->entity.health;
    if (!p.body)
        return out;
    const auto &b = *p.body;
    std::optional<Vec3> origin;
    if (b.entity.position && b.viewOffset && pipeline::finite(*b.entity.position) && pipeline::finite(*b.viewOffset))
        origin = Vec3{b.entity.position->x + b.viewOffset->x, b.entity.position->y + b.viewOffset->y,
                      b.entity.position->z + b.viewOffset->z};
    const auto angles = b.eyeBasis && b.eyeBasis->value && pipeline::finite(*b.eyeBasis->value) ? b.eyeBasis->value
                                                                                                : b.networkEyeAngles;
    if (origin && angles && pipeline::finite(*angles) && body_.valid())
    {
        const auto pitch = angles->x * std::numbers::pi / 180, yaw = angles->y * std::numbers::pi / 180;
        out.aim = body_.directionProximity(*origin, {origin->x + 1000000 * std::cos(pitch) * std::cos(yaw),
                                                     origin->y + 1000000 * std::cos(pitch) * std::sin(yaw),
                                                     origin->z - 1000000 * std::sin(pitch)});
    }
    const auto weapon = std::find_if(frame_.weapons.begin(), frame_.weapons.end(),
                                     [&](const auto &w) { return b.activeWeapon == w.entity.id; });
    if (weapon == frame_.weapons.end() || !weapon->definition || !*weapon->definition)
        return out;
    out.weapon = *weapon->definition;
    if (weapon->clip && *weapon->clip >= 0)
        out.clip = weapon->clip;
    out.reloading = weapon->reloading;
    if (trigger::gunCategory(*out.weapon) == trigger::GunCategory::Unsupported)
        return out;
    const auto team = p.team ? p.team : b.entity.team;
    if (!origin || !body_.valid() || !damageTarget_ || !team || (*team != 2 && *team != 3))
        return out;
    std::vector<DamageImpact> impacts;
    bool complete = true;
    for (const auto &h : pose_.hitboxes)
    {
        if (stop.stop_requested())
            return out;
        const auto &localA = h.definition->a, &localB = h.definition->b;
        const auto center =
            h.point(h.definition->shape == hitboxes::Shape::Sphere
                        ? localA
                        : Vec3{(localA.x + localB.x) / 2, (localA.y + localB.y) / 2, (localA.z + localB.z) / 2});
        const auto hit = body_.first(*origin, center);
        if (!hit)
        {
            complete = false;
            continue;
        }
        impacts.push_back({{hit->point.x, hit->point.y, hit->point.z}, hit->group});
    }
    if (impacts.empty())
        return out;
    out.assumedFriendlyFire = *team == damageTarget_->team;
    const auto predictions =
        out.assumedFriendlyFire
            ? scene_.predictImpactsWithRules(*out.weapon, *team, {origin->x, origin->y, origin->z}, impacts,
                                             *damageTarget_, {true})
            : scene_.predictImpacts(*out.weapon, *team, {origin->x, origin->y, origin->z}, impacts, *damageTarget_);
    queries += static_cast<unsigned>(impacts.size());
    if (predictions.size() != impacts.size())
        return out;
    double maximum = 0;
    for (const auto &prediction : predictions)
    {
        if (stop.stop_requested())
            return out;
        if (!prediction.flight.buildMatches)
        {
            complete = false;
            continue;
        }
        if (prediction.flight.outcome == ShotOutcome::Stopped)
            continue;
        if (prediction.flight.outcome != ShotOutcome::Reached || !prediction.healthDamage ||
            !std::isfinite(*prediction.healthDamage) || *prediction.healthDamage < 0)
        {
            complete = false;
            continue;
        }
        maximum = std::max(maximum, *prediction.healthDamage);
    }
    if (complete)
    {
        out.incomingDamage = maximum;
        // Finite blocked samples do not prove that the entire body is safe.
        // Only complete zero upper bounds can remove all instantaneous threat.
        if (maximum == 0 && !out.assumedFriendlyFire)
        {
            const auto bounds = scene_.damageUpperBounds(*out.weapon, *team, *damageTarget_);
            out.noDamage = std::all_of(pose_.hitboxes.begin(), pose_.hitboxes.end(), [&](const auto &h) {
                bool found = false;
                for (const auto &bound : bounds)
                    if (bound.group == h.definition->group)
                    {
                        found = true;
                        if (!std::isfinite(bound.damage) || bound.damage != 0)
                            return false;
                    }
                return found;
            });
        }
    }
    return out;
}
} // namespace mousebox::games::cs2::rage
