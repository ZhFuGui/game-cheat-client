#include "analysis.hpp"
#include "threat.hpp"
#include "games/cs2/control/pipeline/frame_context.hpp"
#include "games/cs2/control/pipeline/angles.hpp"
#include "games/cs2/extensions/remaining_damage/inputs.hpp"
#include "games/cs2/extensions/head_damage/inputs.hpp"
#include "games/cs2/hitboxes/assets.hpp"
#include "games/cs2/hitboxes/frame.hpp"
#include "games/cs2/trigger/weapon_filter.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <numbers>

namespace mousebox::games::cs2::rage
{
namespace
{
const hitboxes::ModelAsset *asset(const Snapshot &f, const Body &b)
{
    if ((b.skeletonStatus != SkeletonStatus::VerifiedComplete && b.skeletonStatus != SkeletonStatus::VerifiedPartial) ||
        !b.skeletonModel || !b.skeletonProfile)
        return nullptr;
    return hitboxes::findModel(f.build, *b.skeletonProfile, *b.skeletonModel);
}
bool included(const Player &p, const extensions::ShotContext &self, TargetFilter filter)
{
    if (!p.body || p.controller == self.controller || p.body->entity.id == self.pawn || !alive(p, *p.body))
        return false;
    const auto team = p.team ? p.team : p.body->entity.team;
    return team && filter.allows(self.team, *team);
}
std::optional<double> threat(const Body &body, Vec3 self)
{
    const auto direction = body.eyeBasis && body.eyeBasis->value && pipeline::finite(*body.eyeBasis->value)
                               ? body.eyeBasis->value
                               : body.networkEyeAngles;
    if (!direction || !pipeline::finite(*direction) || !body.entity.position || !body.viewOffset ||
        !pipeline::finite(*body.entity.position) || !pipeline::finite(*body.viewOffset))
        return {};
    const auto &p = *body.entity.position, &v = *body.viewOffset, &a = *direction;
    const Vec3 delta{self.x - p.x - v.x, self.y - p.y - v.y, self.z - p.z - v.z};
    const double length = std::hypot(delta.x, delta.y, delta.z);
    if (length <= 1e-6)
        return {};
    const double pitch = a.x * std::numbers::pi / 180, yaw = a.y * std::numbers::pi / 180;
    const double cosine = (std::cos(pitch) * std::cos(yaw) * delta.x + std::cos(pitch) * std::sin(yaw) * delta.y -
                           std::sin(pitch) * delta.z) /
                          length;
    const double angle = std::acos(std::clamp(cosine, -1., 1.)) * 180 / std::numbers::pi;
    // An orientation estimate, never an assertion that the enemy selected us.
    return std::clamp(1 - angle / 15., 0., 1.);
}
} // namespace
const Bone *currentBone(const Snapshot &f, const Candidate &c, TargetFilter filter)
{
    const auto self = extensions::shotContext(f);
    if (!self)
        return nullptr;
    for (const auto &p : f.players)
        if (p.controller == c.target.controller && p.body && p.body->entity.id == c.target.pawn &&
            included(p, *self, filter) && asset(f, *p.body) && p.body->skeletonModel == c.model &&
            p.body->skeletonProfile == c.profile)
            for (const auto &b : p.body->bones)
                if (b.index == c.bone && pipeline::finite(b.position))
                    return &b;
    return nullptr;
}
std::optional<Vec3> currentAimPoint(const Snapshot &f, const Candidate &c, TargetFilter filter)
{
    if (!c.hitbox || !currentBone(f, c, filter))
        return {};
    for (const auto &p : f.players)
        if (p.controller == c.target.controller && p.body && p.body->entity.id == c.target.pawn)
        {
            const auto pose = hitboxes::fromBody(f, *p.body);
            for (const auto &box : pose.hitboxes)
                if (box.definition->id == *c.hitbox && box.definition->bone == c.bone &&
                    box.definition->group == c.group)
                {
                    const auto point = box.center();
                    return pipeline::finite(point) ? std::optional(point) : std::nullopt;
                }
            return {};
        }
    return {};
}
Analysis analyze(const Snapshot &f, const application::ports::VisibilityScene *visibility,
                 const application::ports::BallisticsScene &ballistics, TargetFilter filter, std::stop_token stop,
                 const ScorePolicy &policy, bool coverCheck)
{
    using namespace application::ports;
    Analysis out;
    if (!filter.any())
    {
        out.block = "targets-disabled";
        return out;
    }
    if (coverCheck && !visibility)
    {
        out.block = "scene-unavailable";
        return out;
    }
    const auto self = extensions::shotContext(f);
    if (!f.directoryVerified || !localBody(f) || !self)
    {
        out.block = "source-unavailable";
        return out;
    }
    const auto weapon =
        std::find_if(f.weapons.begin(), f.weapons.end(), [&](const Weapon &w) { return w.entity.id == self->weapon; });
    if (weapon == f.weapons.end() || trigger::gunCategory(self->definition) == trigger::GunCategory::Unsupported ||
        !weapon->clip || *weapon->clip <= 0 || weapon->reloading != false)
    {
        out.block = "weapon-not-ready";
        return out;
    }
    const domain::Vec3 origin{self->origin.x, self->origin.y, self->origin.z};
    const auto threatEvaluator =
        policy.model == ScoreModel::ThreatV2 ? std::make_unique<ThreatEvaluator>(f, ballistics) : nullptr;
    for (const auto &p : f.players)
    {
        if (stop.stop_requested())
            return {{}, out.queries, out.unknown, "cancelled"};
        if (!included(p, *self, filter))
            continue;
        const auto model = asset(f, *p.body);
        const auto target = extensions::damageTarget(p);
        if (!model || !target)
        {
            ++out.unknown;
            continue;
        }
        const auto pose = hitboxes::fromBody(f, *p.body);
        out.unknown += pose.missing + pose.invalid;
        if (!pose.model)
        {
            ++out.unknown;
            continue;
        }
        std::vector<Candidate> eligible;
        const auto danger = threat(*p.body, self->origin);
        std::vector<DamageImpact> impacts;
        std::set<unsigned> visited;
        for (const auto &box : pose.hitboxes)
        {
            if (stop.stop_requested())
                return {{}, out.queries, out.unknown, "cancelled"};
            const auto &definition = *box.definition;
            if (!definition.group || !visited.insert(definition.bone).second)
                continue;
            const auto point = box.center();
            if (!pipeline::finite(point))
            {
                ++out.unknown;
                continue;
            }
            ++out.queries;
            if (coverCheck)
            {
                const auto exposure = visibility->query(origin, {point.x, point.y, point.z});
                if (exposure == Visibility::Unknown)
                    ++out.unknown;
                if (exposure != Visibility::Visible)
                    continue;
            }
            eligible.push_back({{p.controller, p.body->entity.id},
                                definition.bone,
                                definition.group,
                                point,
                                0,
                                danger,
                                *p.body->skeletonModel,
                                *p.body->skeletonProfile});
            eligible.back().hitbox = definition.id;
            impacts.push_back({{point.x, point.y, point.z}, definition.group});
        }
        if (impacts.empty())
            continue;
        auto evidence = threatEvaluator ? threatEvaluator->evaluate(p, stop, out.queries) : ThreatEvidence{};
        evidence.legacyAim = danger;
        // Teammates selected by the user are ranked under an explicit attackable
        // target assumption. Never alter their real team or the default ESP rules.
        const auto predictions =
            target->team == self->team
                ? ballistics.predictImpactsWithRules(self->definition, self->team, origin, impacts, *target, {true})
                : ballistics.predictImpacts(self->definition, self->team, origin, impacts, *target);
        if (predictions.size() != eligible.size())
        {
            ++out.unknown;
            continue;
        }
        for (std::size_t i = 0; i < eligible.size(); ++i)
        {
            const auto &r = predictions[i];
            if (!r.flight.buildMatches || r.flight.outcome == ShotOutcome::Unknown)
            {
                ++out.unknown;
                continue;
            }
            if (r.flight.outcome == ShotOutcome::Stopped)
                continue;
            if (r.flight.outcome != ShotOutcome::Reached || !r.healthDamage || !std::isfinite(*r.healthDamage))
            {
                ++out.unknown;
                continue;
            }
            if (*r.healthDamage <= 0)
                continue;
            eligible[i].damage = *r.healthDamage;
            eligible[i].evidence = evidence;
            eligible[i].targetHealth = target->health;
            out.candidates.push_back(eligible[i]);
        }
    }
    std::stable_sort(out.candidates.begin(), out.candidates.end(), [](const Candidate &a, const Candidate &b) {
        if (a.damage != b.damage)
            return a.damage > b.damage;
        if (a.target.controller != b.target.controller)
            return a.target.controller < b.target.controller;
        return a.bone < b.bone;
    });
    out.block = !out.candidates.empty() ? "ready"
                : out.unknown           ? "analysis-unknown"
                : coverCheck            ? "no-visible-target"
                                        : "no-damage-target";
    return out;
}
const Candidate *select(const Analysis &a, double threatWeight)
{
    if (a.candidates.empty() || !std::isfinite(threatWeight) || threatWeight < 0 || threatWeight > 1)
        return nullptr;
    double maximum = 0;
    for (const auto &c : a.candidates)
        maximum = std::max(maximum, c.damage);
    if (maximum <= 0)
        return nullptr;
    const Candidate *best = nullptr;
    double score = -1;
    for (const auto &c : a.candidates)
    {
        const double value = threatWeight * c.threat.value_or(0) + (1 - threatWeight) * c.damage / maximum;
        if (value > score || (value == score && (!best || c.damage > best->damage)))
        {
            score = value;
            best = &c;
        }
    }
    return best;
}
} // namespace mousebox::games::cs2::rage
