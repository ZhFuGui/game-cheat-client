#pragma once
#include "games/cs2/extensions/best_damage/result.hpp"
#include "render/primitives.hpp"
namespace mousebox::games::cs2
{
inline constexpr std::uint32_t bestDamageColor = 0x38d5ee;
const extensions::ActorBestDamage *bestDamageActor(const extensions::BestDamageResult *, const Snapshot &,
                                                   const Player &);
std::wstring bestDamageLabel(const extensions::ActorBestDamage &, bool english);
std::optional<Vec3> bestDamagePoint(const extensions::ActorBestDamage &, const Snapshot &, const Player &);
template <class Project, class Scale>
void appendBestDamage(render::EspPrimitives &out, const extensions::ActorBestDamage &actor, Vec3 point, Project project,
                      Scale scale)
{
    if (!actor.search.best || !actor.search.best->prediction.healthDamage ||
        *actor.search.best->prediction.healthDamage <= 0 ||
        actor.search.best->prediction.flight.outcome != application::ports::ShotOutcome::Reached)
        return;
    render::StyledEspGeometry geometry{bestDamageColor};
    if (const auto p = project(point))
    {
        const auto annotation = scale(point).annotation;
        const float r = 5 * annotation, w = 1.3f * annotation;
        geometry.heads.push_back({*p, r, w});
        geometry.skeletons.push_back({{p->x - r * 1.5f, p->y}, {p->x + r * 1.5f, p->y}, w, w});
        geometry.skeletons.push_back({{p->x, p->y - r * 1.5f}, {p->x, p->y + r * 1.5f}, w, w});
    }
    if (!geometry.skeletons.empty() || !geometry.heads.empty())
        out.decorations.push_back(std::move(geometry));
}
} // namespace mousebox::games::cs2
