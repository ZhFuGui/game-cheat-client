#include "extension.hpp"
#include "sampling.hpp"
#include "games/cs2/extensions/frame_input.hpp"
#include "games/cs2/hitboxes/frame.hpp"
#include "games/cs2/hitboxes/geometry.hpp"
#include <cmath>
#include <stdexcept>

namespace mousebox::games::cs2::extensions
{
namespace
{
domain::Vec3 point(Vec3 p)
{
    return {p.x, p.y, p.z};
}
bool finite(Vec3 p)
{
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
} // namespace
CoverExtension::CoverExtension(std::shared_ptr<application::ports::VisibilityProvider> provider)
    : provider_(std::move(provider))
{
    if (!provider_)
        throw std::invalid_argument("Cover extension needs an explicit visibility provider");
}
const application::extensions::Definition &CoverExtension::definition() const noexcept
{
    return coverDefinition();
}
application::Json CoverExtension::settings(const application::Json &values) const
{
    return {{"hitboxes", values.value("cs2HitboxDisplay", "off") != "off"}};
}
void CoverExtension::configure(const application::Json &values)
{
    hitboxes_ = values.value("hitboxes", false);
}
void CoverExtension::reset() noexcept
{
    scene_.reset();
    loadedScope_.reset();
    retryAtMs_ = 0;
    loadError_.clear();
}
application::extensions::Contribution CoverExtension::evaluate(const application::extensions::Input<Snapshot> &in,
                                                               std::stop_token stop)
{
    application::extensions::Contribution out;
    if (loadedScope_ && loadedScope_ != in.scope)
        reset();
    const auto wait = [&](const char *phase, const char *message) {
        out.state.phase = std::string_view(phase) == "cancelled" ? "paused" : "waiting";
        out.state.code = phase;
        out.state.message = message;
        return out;
    };
    if (!in.frame || !in.frame->directoryVerified)
        return wait("waiting-data", "等待当前CS2数据");
    const auto &f = *in.frame;
    if (in.scope != scope(f) || in.capture != f.sequence || !f.map || !f.map->available())
        return wait("waiting-map", "等待Reader正式地图身份");
    if (!provider_->available())
        return wait("dependency-pending", "traceline编译产物和地图场景待交付");
    if (!f.local.eye || !finite(*f.local.eye))
        return wait("waiting-origin", "等待本批视点世界坐标");
    std::optional<unsigned> selfTeam;
    for (const auto &p : f.players)
        if (f.local.controller && p.controller == *f.local.controller)
            selfTeam = p.team ? p.team : p.body ? p.body->entity.team : std::nullopt;
    if (!selfTeam || (*selfTeam != 2 && *selfTeam != 3))
        return wait("waiting-team", "等待明确的本地敌我关系");
    if (loadedScope_ != in.scope)
        reset();
    if (!scene_)
    {
        loadedScope_ = in.scope;
        if (retryAtMs_ == 0 || in.observedAtMs >= retryAtMs_)
        {
            try
            {
                scene_ = provider_->load(f.map->name, f.map->resourcePath);
                loadError_.clear();
                retryAtMs_ = 0;
            }
            catch (const std::exception &error)
            {
                loadError_ = error.what();
                retryAtMs_ = in.observedAtMs + 5000;
            }
        }
        if (!loadError_.empty())
        {
            out.state.phase = "error";
            out.state.code = "scene-load";
            out.state.message = loadError_;
            return out;
        }
    }
    if (!scene_)
        return wait("scene-unavailable", "当前地图没有可用碰撞场景");
    if (stop.stop_requested())
        return wait("cancelled", "旧场景分析已取消");
    auto result = std::make_shared<CoverResult>();
    result->scope = in.scope;
    result->capture = in.capture;
    RayBudget rays(*scene_, point(*f.local.eye), 16384, stop);
    for (const auto &p : f.players)
    {
        if (stop.stop_requested())
            return wait("cancelled", "旧场景分析已取消");
        if (!p.body || (f.local.controller && p.controller == *f.local.controller) || p.alive == false)
            continue;
        const auto &body = *p.body;
        const auto team = p.team ? p.team : body.entity.team;
        if (!team || (*team != 2 && *team != 3) || *team == *selfTeam || body.entity.dormant == true ||
            (body.entity.health && *body.entity.health <= 0) ||
            (body.entity.lifeState && *body.entity.lifeState != 0) ||
            (body.skeletonStatus != SkeletonStatus::VerifiedPartial &&
             body.skeletonStatus != SkeletonStatus::VerifiedComplete))
            continue;
        if (result->actors.size() == 64)
        {
            ++rays.unknown;
            break;
        }
        ActorVisibility actor;
        actor.controller = p.controller;
        actor.pawn = body.entity.id;
        std::map<unsigned, const Bone *> bones;
        for (const auto &b : body.bones)
        {
            if (bones.size() == 128)
            {
                ++rays.unknown;
                break;
            }
            bones.emplace(b.index, &b);
            actor.bones.emplace(b.index, rays.query(point(b.position)));
        }
        for (const auto &edge : body.boneLinks)
        {
            const auto a = bones.find(edge.parent), b = bones.find(edge.child);
            if (a == bones.end() || b == bones.end() || actor.links.size() == 256)
                continue;
            actor.links.emplace(std::pair{edge.parent, edge.child},
                                sampleBone(rays, point(a->second->position), point(b->second->position),
                                           actor.bones.at(edge.parent), actor.bones.at(edge.child)));
        }
        for (const auto &[_, v] : actor.bones)
            if (v == Visibility::Visible)
                ++out.state.visible;
            else if (v == Visibility::Occluded)
                ++out.state.occluded;
        result->actors.push_back(std::move(actor));
    }
    // Preserve the existing bone/line budget and ordering before spending any
    // remaining rays on the optional Hitbox surface pass.
    if (hitboxes_)
        for (auto &actor : result->actors)
            for (const auto &player : f.players)
                if (player.controller == actor.controller && player.body && player.body->entity.id == actor.pawn)
                {
                    const auto pose = hitboxes::fromBody(f, *player.body);
                    for (const auto &box : pose.hitboxes)
                    {
                        if (stop.stop_requested())
                            return wait("cancelled", "旧场景分析已取消");
                        const auto surface = hitboxes::worldSurface(box);
                        actor.hitboxes.emplace(box.definition->id, sampleHitbox(rays, surface));
                    }
                    break;
                }
    out.state.queries = rays.queries;
    out.state.unknown = rays.unknown;
    out.state.phase = rays.unknown ? "degraded" : "ready";
    out.state.message = rays.unknown ? "部分射线未确定，未确认部分保留原色" : "敌方骨点和骨段已完成当前几何可见性采样";
    out.attachment = std::move(result);
    return out;
}
} // namespace mousebox::games::cs2::extensions
