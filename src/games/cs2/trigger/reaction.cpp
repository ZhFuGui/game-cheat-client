#include "reaction.hpp"
#include "games/cs2/control/pipeline/frame_context.hpp"
#include "games/cs2/control/pipeline/angles.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace mousebox::games::cs2::trigger
{
std::vector<Exposure> scanTargets(const Snapshot &f, const application::ports::VisibilityScene &scene,
                                  TargetFilter targets, std::stop_token stop)
{
    using application::ports::Visibility;
    std::vector<Exposure> result;
    const auto self = localBody(f);
    if (!self || !f.local.eye || !pipeline::finite(*f.local.eye) || !f.directoryVerified)
        return result;
    const auto local = std::find_if(f.players.begin(), f.players.end(),
                                    [&](const Player &p) { return p.controller == f.local.controller; });
    const auto team = local->team ? local->team : self->entity.team;
    if (!team || (*team != 2 && *team != 3))
        return result;
    for (const auto &p : f.players)
    {
        if (stop.stop_requested())
            break;
        if (!p.controller || p.controller == *f.local.controller || !p.body || !p.body->entity.id || !alive(p, *p.body))
            continue;
        const auto &b = *p.body;
        const auto otherTeam = p.team ? p.team : b.entity.team;
        if (!otherTeam || !targets.allows(*team, *otherTeam))
            continue;
        Exposure e{{p.controller, b.entity.id}};
        if ((b.skeletonStatus == SkeletonStatus::VerifiedComplete ||
             b.skeletonStatus == SkeletonStatus::VerifiedPartial) &&
            !b.bones.empty())
        {
            bool unknown = b.skeletonStatus != SkeletonStatus::VerifiedComplete;
            for (const auto &bone : b.bones)
            {
                if (stop.stop_requested())
                    return {};
                if (!pipeline::finite(bone.position))
                {
                    unknown = true;
                    continue;
                }
                const auto &eye = *f.local.eye;
                const auto v = scene.query({eye.x, eye.y, eye.z}, {bone.position.x, bone.position.y, bone.position.z});
                if (v == Visibility::Visible)
                {
                    e.visibility = v;
                    e.visibleBone = bone.index;
                    break; // At least one is enough; no need to count the rest.
                }
                unknown |= v == Visibility::Unknown;
            }
            if (e.visibility != Visibility::Visible)
                e.visibility = unknown ? Visibility::Unknown : Visibility::Occluded;
        }
        result.push_back(e);
    }
    return result;
}
bool remainsVisible(const Snapshot &frame, const application::ports::VisibilityScene &scene, TargetKey target,
                    unsigned bone)
{
    if (!frame.local.eye || !pipeline::finite(*frame.local.eye))
        return false;
    const auto player = std::find_if(frame.players.begin(), frame.players.end(), [&](const Player &p) {
        return p.controller == target.controller && p.body && p.body->entity.id == target.pawn;
    });
    if (player == frame.players.end() || !alive(*player, *player->body))
        return false;
    const auto &body = *player->body;
    if (body.skeletonStatus != SkeletonStatus::VerifiedComplete &&
        body.skeletonStatus != SkeletonStatus::VerifiedPartial)
        return false;
    const auto point = std::find_if(body.bones.begin(), body.bones.end(),
                                    [&](const Bone &candidate) { return candidate.index == bone; });
    if (point == body.bones.end() || !pipeline::finite(point->position))
        return false;
    const auto &eye = *frame.local.eye;
    const auto &position = point->position;
    return scene.query({eye.x, eye.y, eye.z}, {position.x, position.y, position.z}) ==
           application::ports::Visibility::Visible;
}
void ReactionTimers::reset()
{
    starts_.clear();
    order_ = 0;
    sourceNs_ = 0;
}
void ReactionTimers::observe(std::vector<Exposure> &values, std::int64_t start, std::uint64_t order)
{
    if (start <= 0 || !order)
    {
        reset();
        for (auto &e : values)
            e.visibleSinceNs.reset();
        return;
    }
    if (order_ && (order < order_ || order > order_ + 1 || start < sourceNs_))
        reset();
    decltype(starts_) next;
    for (auto &e : values)
    {
        e.visibleSinceNs.reset();
        if (e.visibility != application::ports::Visibility::Visible)
            continue;
        const auto it = starts_.find(e.id);
        e.visibleSinceNs = it == starts_.end() ? start : it->second;
        next.emplace(e.id, *e.visibleSinceNs);
    }
    starts_ = std::move(next);
    order_ = order;
    sourceNs_ = start;
}
double remainingReactionMs(std::int64_t start, const control::SourceClockEstimate &clock, std::uint64_t now,
                           unsigned delay) noexcept
{
    const long double first = clock.localAnchorMs + (static_cast<long double>(start) - clock.sourceAnchorNs) / 1e6L;
    if (start <= 0 || clock.sourceAnchorNs <= 0 || !std::isfinite(first))
        return std::numeric_limits<double>::infinity();
    return static_cast<double>(std::max(0.L, static_cast<long double>(delay) - (now - first)));
}
} // namespace mousebox::games::cs2::trigger
