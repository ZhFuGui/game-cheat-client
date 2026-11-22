#include "shot_gate.hpp"
#include "games/cs2/extensions/head_damage/inputs.hpp"
#include "games/cs2/hitboxes/frame.hpp"
#include "games/cs2/hitboxes/intersection.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace mousebox::games::cs2::rage
{
ShotEvaluation evaluateShot(const Snapshot &f, const Candidate &selected, TargetFilter filter, bool coverCheck,
                            bool otherPlayerCheck, double inset, const application::ports::VisibilityScene *visibility,
                            const application::ports::BallisticsScene *ballistics)
{
    using namespace application::ports;
    ShotEvaluation out;
    const auto reject = [&](std::string reason) {
        out.block = std::move(reason);
        return out;
    };
    const auto point = currentAimPoint(f, selected, filter);
    if (!selected.hitbox || !point)
        return reject("target-unavailable");
    if (!ballistics || (coverCheck && !visibility))
        return reject("scene-unavailable");
    out.hit = trigger::detect(f, {}, filter, trigger::UnknownTargetPolicy::Block);
    if (out.hit.block != "ready")
        return reject(out.hit.block);
    if (otherPlayerCheck &&
        (out.hit.controller != selected.target.controller || out.hit.target != selected.target.pawn))
        return reject("other-person-first");
    const auto actual = std::find_if(f.players.begin(), f.players.end(), [&](const Player &p) {
        return p.controller == out.hit.controller && p.body && p.body->entity.id == out.hit.target;
    });
    if (actual == f.players.end())
        return reject("target-unavailable");
    const auto &origin = out.hit.origin, &hitPoint = out.hit.point;
    const double length = std::hypot(hitPoint.x - origin.x, hitPoint.y - origin.y, hitPoint.z - origin.z);
    if (!std::isfinite(length) || length <= 1e-6)
        return reject("shot-direction-missing");
    const Vec3 end{origin.x + (hitPoint.x - origin.x) * 1000000 / length,
                   origin.y + (hitPoint.y - origin.y) * 1000000 / length,
                   origin.z + (hitPoint.z - origin.z) * 1000000 / length};
    const auto hit = hitboxes::TargetIntersector(hitboxes::fromBody(f, *actual->body)).first(origin, end, inset);
    if (!hit || hit->id != out.hit.hitbox)
        return reject("inset-rejected");
    const auto self = extensions::shotContext(f);
    const auto target = std::find_if(f.players.begin(), f.players.end(), [&](const Player &p) {
        return p.controller == selected.target.controller && p.body && p.body->entity.id == selected.target.pawn;
    });
    const auto damageTarget = target != f.players.end() ? extensions::damageTarget(*target) : std::nullopt;
    if (!self || !damageTarget)
        return reject("damage-unavailable");
    try
    {
        const domain::Vec3 a{origin.x, origin.y, origin.z}, b{point->x, point->y, point->z};
        if (coverCheck && (visibility->query(a, b) != Visibility::Visible ||
                           visibility->query(a, {hitPoint.x, hitPoint.y, hitPoint.z}) != Visibility::Visible))
            return reject("cover-occluded");
        const std::array<DamageImpact, 1> impacts{{{b, selected.group}}};
        const auto predictions =
            damageTarget->team == self->team
                ? ballistics->predictImpactsWithRules(self->definition, self->team, a, impacts, *damageTarget, {true})
                : ballistics->predictImpacts(self->definition, self->team, a, impacts, *damageTarget);
        if (predictions.size() != 1)
            return reject("damage-unavailable");
        const auto &prediction = predictions.front();
        if (!prediction.flight.buildMatches || prediction.flight.outcome == ShotOutcome::Unknown)
            return reject("damage-unavailable");
        if (prediction.flight.outcome == ShotOutcome::Stopped)
            return reject("shot-no-damage");
        if (prediction.flight.outcome != ShotOutcome::Reached || !prediction.healthDamage ||
            !std::isfinite(*prediction.healthDamage))
            return reject("damage-unavailable");
        out.damage = prediction.healthDamage;
        if (*out.damage <= 0)
            return reject("shot-no-damage");
    }
    catch (...)
    {
        return reject("damage-unavailable");
    }
    out.block = "ready";
    return out;
}
} // namespace mousebox::games::cs2::rage
