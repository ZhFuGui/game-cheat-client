#include "result.hpp"
#include "games/cs2/extensions/frame_input.hpp"
#include "games/cs2/hitboxes/frame.hpp"
#include <algorithm>
namespace mousebox::games::cs2::extensions
{
bool matchesCurrent(const BestDamageResult &result, const Snapshot &frame)
{
    const auto self = shotContext(frame);
    if (!self || result.scope != scope(frame) || self->controller != result.shooter.controller ||
        self->pawn != result.shooter.pawn || self->weapon != result.shooter.weapon ||
        self->definition != result.shooter.definition || self->team != result.shooter.team)
        return false;
    for (const auto &actor : result.actors)
    {
        const auto p = std::find_if(frame.players.begin(), frame.players.end(), [&](const Player &p) {
            return p.controller == actor.controller && p.body && p.body->entity.id == actor.pawn;
        });
        if (p == frame.players.end() || !livingEnemy(*p, *self) || damageTarget(*p) != actor.target ||
            p->body->skeletonModel != actor.model || p->body->skeletonProfile != actor.profile)
            return false;
        if (actor.search.best)
        {
            const auto pose = hitboxes::fromBody(frame, *p->body);
            if (!hitboxes::TargetIntersector(pose).valid())
                return false;
        }
    }
    return true;
}
} // namespace mousebox::games::cs2::extensions
