#pragma once
#include "games/cs2/extensions/cover/result.hpp"
#include "games/cs2/extensions/frame_input.hpp"
#include "render/primitives.hpp"

namespace mousebox::games::cs2
{
inline const extensions::ActorVisibility *coverActor(const extensions::CoverResult *cover, const Snapshot &frame,
                                                     const Player &player)
{
    if (!cover || !player.body || !frame.map || !frame.map->available() || cover->scope != extensions::scope(frame) ||
        cover->capture != frame.sequence)
        return nullptr;
    return cover->find(player.controller, player.body->entity.id);
}
inline render::StyledEspGeometry &visibleGeometry(render::EspPrimitives &out)
{
    constexpr std::uint32_t visibleColor = 0xffd400;
    for (auto &batch : out.decorations)
        if (batch.color == visibleColor)
            return batch;
    out.decorations.push_back({visibleColor});
    return out.decorations.back();
}
inline void appendBonePoint(render::EspPrimitives &out, unsigned group, render::Circle point,
                            const extensions::ActorVisibility *cover, unsigned index)
{
    const auto found = cover ? cover->bones.find(index) : std::map<unsigned, extensions::Visibility>::const_iterator{};
    if (cover && found != cover->bones.end() && found->second == extensions::Visibility::Visible)
        visibleGeometry(out).dots.push_back(point);
    else
        out.dots[group].push_back(point);
}
template <class Project, class Scale>
void appendBoneLine(render::EspPrimitives &out, unsigned group, const Bone &a, const Bone &b, render::Point screenA,
                    render::Point screenB, const extensions::ActorVisibility *cover, Project project, Scale scale)
{
    const auto original = [&] {
        out.skeletons[group].push_back({screenA, screenB, scale(a.position).boneWidth, scale(b.position).boneWidth});
    };
    if (!cover)
        return original();
    const auto link = cover->links.find({a.index, b.index});
    if (link == cover->links.end())
        return original();
    const auto between = [&](double t) {
        return Vec3{a.position.x + (b.position.x - a.position.x) * t, a.position.y + (b.position.y - a.position.y) * t,
                    a.position.z + (b.position.z - a.position.z) * t};
    };
    for (const auto &span : link->second)
    {
        const auto first = between(span.start), last = between(span.end);
        const auto p = project(first), q = project(last);
        if (!p || !q || span.end <= span.start)
            continue;
        render::ScaledLine line{*p, *q, scale(first).boneWidth, scale(last).boneWidth};
        if (span.visibility == extensions::Visibility::Visible)
            visibleGeometry(out).skeletons.push_back(line);
        else
            out.skeletons[group].push_back(line);
    }
}
} // namespace mousebox::games::cs2
