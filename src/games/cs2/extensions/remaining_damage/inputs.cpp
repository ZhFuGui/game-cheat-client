#include "inputs.hpp"
#include "games/cs2/hitboxes/frame.hpp"
#include <algorithm>
#include <cmath>
namespace mousebox::games::cs2::extensions
{
std::optional<ShotContext> shotContext(const Snapshot &f)
{
    if (!f.directoryVerified || !f.local.eye || !f.local.controller || !f.local.pawn ||
        !std::isfinite(f.local.eye->x) || !std::isfinite(f.local.eye->y) || !std::isfinite(f.local.eye->z))
        return {};
    for (const auto &p : f.players)
        if (p.controller == *f.local.controller && p.body && p.body->entity.id == *f.local.pawn &&
            p.body->activeWeapon && p.alive != false && p.body->entity.dormant != true)
        {
            const auto team = p.team ? p.team : p.body->entity.team;
            if (!team || (*team != 2 && *team != 3))
                return {};
            for (const auto &weapon : f.weapons)
                if (weapon.entity.id == *p.body->activeWeapon && weapon.definition && *weapon.definition)
                    return ShotContext{p.controller, p.body->entity.id, weapon.entity.id, *weapon.definition,
                                       *team,        *f.local.eye};
        }
    return {};
}
bool livingEnemy(const Player &p, const ShotContext &self)
{
    if (!p.body || p.controller == self.controller || p.alive == false || p.body->entity.dormant == true ||
        (p.body->entity.health && *p.body->entity.health <= 0) ||
        (p.body->entity.lifeState && *p.body->entity.lifeState != 0))
        return false;
    const auto team = p.team ? p.team : p.body->entity.team;
    return team && (*team == 2 || *team == 3) && *team != self.team;
}
std::optional<Vec3> headEndpoint(const Snapshot &f, const Body &body)
{
    if (body.skeletonStatus != SkeletonStatus::VerifiedComplete &&
        body.skeletonStatus != SkeletonStatus::VerifiedPartial)
        return {};
    const auto bone =
        std::find_if(body.bones.begin(), body.bones.end(), [](const Bone &b) { return b.role == BoneRole::Head; });
    if (bone == body.bones.end())
        return {};
    for (const auto &box : hitboxes::fromBody(f, body).hitboxes)
        if (box.definition->bone == bone->index && box.definition->group == 1)
        {
            const auto &d = *box.definition;
            return box.point({(d.a.x + d.b.x) / 2, (d.a.y + d.b.y) / 2, (d.a.z + d.b.z) / 2});
        }
    return bone->position;
}
} // namespace mousebox::games::cs2::extensions
