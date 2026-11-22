#include "detection.hpp"
#include "games/cs2/control/pipeline/frame_context.hpp"
#include "games/cs2/control/pipeline/angles.hpp"
#include "games/cs2/evaluation/recoil.hpp"
#include "games/cs2/evaluation/weapon_accuracy.hpp"
#include "games/cs2/hitboxes/frame.hpp"
#include <algorithm>
#include <cmath>
namespace mousebox::games::cs2::trigger
{
namespace
{
Vec3 translated(Vec3 p, Vec3 velocity, double seconds)
{
    return {p.x + velocity.x * seconds, p.y + velocity.y * seconds, p.z + velocity.z * seconds};
}
} // namespace
DetectionScene::DetectionScene(const Snapshot &f, WeaponFilter filter, TargetFilter targets,
                               UnknownTargetPolicy unknownPolicy)
{
    if (!targets.any())
    {
        block_ = "targets-disabled";
        return;
    }
    if (!f.directoryVerified)
    {
        block_ = "source-unverified";
        return;
    }
    const auto self = localBody(f);
    if (!self || !f.local.eye)
    {
        block_ = "local-not-playing";
        return;
    }
    const auto local = std::find_if(f.players.begin(), f.players.end(),
                                    [&](const Player &p) { return p.controller == f.local.controller; });
    const auto team = local->team ? local->team : self->entity.team;
    if (!team || (*team != 2 && *team != 3))
    {
        block_ = "team-missing";
        return;
    }
    const auto weapon = std::find_if(f.weapons.begin(), f.weapons.end(),
                                     [&](const Weapon &w) { return self->activeWeapon == w.entity.id; });
    const auto category =
        weapon != f.weapons.end() && weapon->definition ? gunCategory(*weapon->definition) : GunCategory::Unsupported;
    if (category == GunCategory::Unsupported || !weapon->clip || *weapon->clip <= 0 || weapon->reloading != false)
    {
        block_ = "weapon-not-ready";
        return;
    }
    if (!filter.allows(category))
    {
        block_ = "weapon-filtered";
        return;
    }
    sniper_ = category == GunCategory::Sniper;
    const auto &accuracy = weapon->accuracy;
    if (sniper_ && accuracy && accuracy->owner == self->entity.id && accuracy->inaccuracy.value &&
        *accuracy->inaccuracy.value >= 0 && validateWeaponAccuracy(f, *accuracy) == AccuracyIssue::None)
        sniperInaccuracy_ = accuracy->inaccuracy.value;
    const auto recoil = evaluateRecoil(f);
    if (!recoil.contextDirection)
    {
        block_ = "shot-direction-missing";
        return;
    }
    origin_ = *f.local.eye;
    direction_ = *recoil.contextDirection;
    offset_ = recoil.shotOffset;
    punch_ = self->aimPunch;
    velocity_ = self->entity.velocity;
    for (const auto &p : f.players)
    {
        if (p.controller == *f.local.controller || !p.body)
            continue;
        const bool eligibleAlive = alive(p, *p.body);
        if (!eligibleAlive)
        {
            const auto &e = p.body->entity;
            const bool knownInactive = p.alive == false || (e.health && *e.health <= 0) ||
                                       (e.lifeState && *e.lifeState != 0) || e.dormant == true;
            if (unknownPolicy == UnknownTargetPolicy::Ignore || knownInactive)
                continue;
        }
        const auto otherTeam = p.team ? p.team : p.body->entity.team;
        const bool knownTeam = otherTeam && (*otherTeam == 2 || *otherTeam == 3);
        if (!knownTeam && unknownPolicy == UnknownTargetPolicy::Ignore)
            continue;
        hitboxes::TargetIntersector geometry(hitboxes::fromBody(f, *p.body));
        if (!geometry.valid())
            continue;
        const auto block = !eligibleAlive || !knownTeam        ? "target-filtered"
                           : targets.allows(*team, *otherTeam) ? "ready"
                           : *otherTeam == *team               ? "friendly-first"
                                                               : "target-filtered";
        targets_.push_back({p.controller, p.body->entity.id, p.body->entity.velocity, block, std::move(geometry)});
    }
}
Detection DetectionScene::detect(DetectionOptions options) const
{
    if (!block_.empty())
        return {block_};
    if (sniper_ && options.sniperInaccuracyLimit)
    {
        if (!options.weaponAccuracyEvaluation || !sniperInaccuracy_)
            return {"sniper-inaccuracy-unavailable"};
        if (*sniperInaccuracy_ >= *options.sniperInaccuracyLimit)
            return {"sniper-inaccuracy-too-high"};
    }
    if (!std::isfinite(options.insetFraction) || options.insetFraction < 0 || options.insetFraction >= 1 ||
        !std::isfinite(options.horizonMs) || options.horizonMs < 0 || options.horizonMs > 750)
        return {"prediction-unavailable"};
    auto origin = origin_;
    auto direction = direction_;
    const double seconds = options.horizonMs / 1000;
    if (seconds > 0)
    {
        if (!velocity_ || !pipeline::finite(*velocity_))
            return {"local-velocity-missing"};
        if (!punch_ || !offset_)
            return {"shot-direction-missing"};
        const auto future = evaluateShotOffset(*punch_, seconds);
        if (!future)
            return {"prediction-unavailable"};
        direction = {direction.x + future->x - offset_->x, direction.y + future->y - offset_->y,
                     direction.z + future->z - offset_->z};
        origin = translated(origin, *velocity_, seconds);
    }
    const double pitch = direction.x * std::acos(-1.0) / 180, yaw = direction.y * std::acos(-1.0) / 180;
    const Vec3 end{origin.x + 1000000 * std::cos(pitch) * std::cos(yaw),
                   origin.y + 1000000 * std::cos(pitch) * std::sin(yaw), origin.z - 1000000 * std::sin(pitch)};
    if (!pipeline::finite(origin) || !pipeline::finite(end))
        return {"shot-direction-missing"};
    double nearest = 1;
    Detection out;
    for (const auto &target : targets_)
    {
        Vec3 shift{};
        if (seconds > 0)
        {
            if (!target.velocity || !pipeline::finite(*target.velocity))
                return {"target-velocity-missing"};
            shift = translated({}, *target.velocity, seconds);
        }
        // Translate the ray back instead of rebuilding the sampled bone transforms.
        const auto start = translated(origin, shift, -1), finish = translated(end, shift, -1);
        const auto hit = target.geometry.first(start, finish);
        if (!hit || hit->fraction >= nearest)
            continue;
        nearest = hit->fraction;
        out = {target.block, target.pawn, hit->id, origin, translated(hit->point, shift, 1), target.controller};
        // First determine the actual unshrunk blocker. An inset must never let a
        // ray pass through the edge of a teammate into an enemy behind them.
        if (out.block == "ready" && options.insetFraction > 0)
        {
            const auto inner = target.geometry.first(start, finish, options.insetFraction);
            if (!inner || inner->id != hit->id)
                out.block = "inset-rejected";
        }
    }
    if (targets_.empty())
        out.block = "hitboxes-missing";
    return out;
}
Detection detect(const Snapshot &f, WeaponFilter filter, TargetFilter targets, UnknownTargetPolicy unknownPolicy,
                 DetectionOptions options)
{
    return DetectionScene(f, filter, targets, unknownPolicy).detect(options);
}
} // namespace mousebox::games::cs2::trigger
