#include "extension.hpp"
#include "games/cs2/extensions/frame_input.hpp"
namespace mousebox::games::cs2::extensions
{
HeadDamageExtension::HeadDamageExtension(std::shared_ptr<application::ports::BallisticsProvider> provider)
    : scene_(std::move(provider))
{
}
const application::extensions::Definition &HeadDamageExtension::definition() const noexcept
{
    return headDamageDefinition();
}
void HeadDamageExtension::reset() noexcept
{
    scene_.reset();
}
application::extensions::Contribution HeadDamageExtension::evaluate(const application::extensions::Input<Snapshot> &in,
                                                                    std::stop_token stop)
{
    application::extensions::Contribution out;
    const auto wait = [&](const char *code, const char *text) {
        out.state.phase = "waiting";
        out.state.code = code;
        out.state.message = text;
        return out;
    };
    if (!in.frame || !in.frame->directoryVerified)
        return wait("waiting-data", "等待当前CS2数据");
    const auto &f = *in.frame;
    scene_.retainScope(in.scope, f.build);
    if (in.scope != scope(f) || in.capture != f.sequence || !f.map || !f.map->available())
        return wait("waiting-map", "等待Reader正式地图身份");
    const auto self = shotContext(f);
    if (!self)
        return wait("waiting-weapon", "等待本批本地视点、队伍和手持武器");
    if (!scene_.available())
        return wait("dependency-pending", "等待头部伤害运行库");
    const auto scene = scene_.acquire(in.scope, *f.map, f.build, in.observedAtMs);
    if (!scene)
    {
        out.state.phase = "error";
        out.state.code = "scene-unavailable";
        out.state.message = scene_.error();
        return out;
    }
    auto result = std::make_shared<HeadDamageResult>();
    result->scope = in.scope;
    result->capture = in.capture;
    result->shooter = *self;
    for (const auto &p : f.players)
    {
        if (stop.stop_requested())
            return wait("cancelled", "旧头部伤害分析已取消");
        if (!livingEnemy(p, *self))
            continue;
        if (result->actors.size() == 64)
        {
            ++out.state.unknown;
            break;
        }
        ActorHeadDamage actor;
        actor.controller = p.controller;
        actor.pawn = p.body->entity.id;
        actor.target = damageTarget(p);
        actor.model = p.body->skeletonModel;
        actor.profile = p.body->skeletonProfile;
        if (!actor.target)
            actor.prediction.code = "target-state-missing";
        else if (const auto impact = headImpact(f, *p.body, self->origin))
        {
            actor.prediction =
                scene->predictHead(self->definition, self->team, {self->origin.x, self->origin.y, self->origin.z},
                                   {impact->x, impact->y, impact->z}, *actor.target);
            ++out.state.queries;
        }
        else
            actor.prediction.code = "head-impact-unavailable";
        if (!actor.prediction.healthDamage)
            ++out.state.unknown;
        result->actors.push_back(std::move(actor));
    }
    out.state.phase = "degraded";
    out.state.code = out.state.unknown ? "partial-head-paths" : "assumed-damage-rules";
    out.state.message = out.state.unknown ? "部分头部命中路径或护甲数据不完整，伤害显示为未知"
                                          : "默认规则且目标非无敌条件下的预计头部伤害";
    out.attachment = std::move(result);
    return out;
}
} // namespace mousebox::games::cs2::extensions
