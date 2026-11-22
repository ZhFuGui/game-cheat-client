#include "inputs.hpp"
#include "games/cs2/hitboxes/frame.hpp"
#include "games/cs2/hitboxes/intersection.hpp"
namespace mousebox::games::cs2::extensions
{
std::optional<application::ports::PlayerDamageTarget> damageTarget(const Player &player)
{
    if (!player.body || !player.body->entity.health || *player.body->entity.health <= 0 || !player.body->armor ||
        *player.body->armor < 0 || !player.body->helmet)
        return {};
    const auto team = player.team ? player.team : player.body->entity.team;
    if (!team || (*team != 2 && *team != 3))
        return {};
    return application::ports::PlayerDamageTarget{*player.body->entity.health, *player.body->armor, *team,
                                                  *player.body->helmet};
}
std::optional<Vec3> headImpact(const Snapshot &frame, const Body &body, Vec3 origin)
{
    const auto endpoint = headEndpoint(frame, body);
    if (!endpoint)
        return {};
    const auto hit = hitboxes::firstIntersection(hitboxes::fromBody(frame, body), origin, *endpoint);
    if (!hit || hit->group != 1)
        return {};
    return hit->point;
}
} // namespace mousebox::games::cs2::extensions
