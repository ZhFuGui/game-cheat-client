#include "best_damage.hpp"
#include "games/cs2/extensions/frame_input.hpp"
#include "games/cs2/hitboxes/frame.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>
namespace mousebox::games::cs2
{
const extensions::ActorBestDamage *bestDamageActor(const extensions::BestDamageResult *result, const Snapshot &f,
                                                   const Player &p)
{
    if (!result || !p.body || result->scope != extensions::scope(f))
        return nullptr;
    for (const auto &actor : result->actors)
        if (actor.controller == p.controller && actor.pawn == p.body->entity.id)
            return &actor;
    return nullptr;
}
std::optional<Vec3> bestDamagePoint(const extensions::ActorBestDamage &actor, const Snapshot &frame,
                                    const Player &player)
{
    if (!actor.search.best || !player.body || actor.controller != player.controller ||
        actor.pawn != player.body->entity.id || actor.model != player.body->skeletonModel ||
        actor.profile != player.body->skeletonProfile)
        return {};
    const auto pose = hitboxes::fromBody(frame, *player.body);
    if (!pose.model || pose.missing || pose.invalid)
        return {};
    for (const auto &box : pose.hitboxes)
        if (box.definition->id == actor.search.best->seed.hitbox)
            return box.point(actor.search.best->seed.local);
    return {};
}
std::wstring bestDamageLabel(const extensions::ActorBestDamage &actor, bool english)
{
    const auto prefix = actor.search.converged ? (english ? L"Best (±0.1) " : L"已收敛(≤0.1) ")
                                               : (english ? L"Search best " : L"搜索最佳 ");
    if (!actor.search.best || !actor.search.best->prediction.healthDamage)
        return std::wstring(prefix) + L"—";
    const auto &best = *actor.search.best;
    if (!std::isfinite(*best.prediction.healthDamage) || *best.prediction.healthDamage < 0)
        return std::wstring(prefix) + L"—";
    if (best.prediction.flight.outcome == application::ports::ShotOutcome::Stopped ||
        *best.prediction.healthDamage == 0)
        return std::wstring(prefix) + (english ? L"0 (sampled paths blocked)" : L"0（已采样路径无伤害）");
    const wchar_t *part = english ? L"Body" : L"身体";
    switch (best.impact.group)
    {
    case 1:
        part = english ? L"Head" : L"头部";
        break;
    case 2:
        part = english ? L"Chest" : L"胸部";
        break;
    case 3:
        part = english ? L"Stomach" : L"腹部";
        break;
    case 4:
        part = english ? L"Left arm" : L"左臂";
        break;
    case 5:
        part = english ? L"Right arm" : L"右臂";
        break;
    case 6:
        part = english ? L"Left leg" : L"左腿";
        break;
    case 7:
        part = english ? L"Right leg" : L"右腿";
        break;
    case 8:
        part = english ? L"Neck" : L"颈部";
        break;
    }
    std::wostringstream text;
    text << prefix << part << L" ≈" << std::fixed << std::setprecision(1) << *best.prediction.healthDamage;
    return text.str();
}
} // namespace mousebox::games::cs2
