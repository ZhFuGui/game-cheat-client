#include "result.hpp"
#include "games/cs2/extensions/frame_input.hpp"
#include <algorithm>
namespace mousebox::games::cs2::extensions
{
bool matchesCurrent(const RemainingDamageResult &result, const Snapshot &frame)
{
    const auto self = shotContext(frame);
    if (!self || result.scope != scope(frame) || self->controller != result.shooter.controller ||
        self->pawn != result.shooter.pawn || self->weapon != result.shooter.weapon ||
        self->definition != result.shooter.definition || self->team != result.shooter.team)
        return false;
    for (const auto &actor : result.actors)
    {
        const auto player = std::find_if(frame.players.begin(), frame.players.end(), [&](const Player &p) {
            return p.controller == actor.controller && p.body && p.body->entity.id == actor.pawn;
        });
        if (player == frame.players.end() || !livingEnemy(*player, *self) ||
            (actor.prediction.code != "head-unavailable" && !headEndpoint(frame, *player->body)))
            return false;
    }
    return true;
}
} // namespace mousebox::games::cs2::extensions
