#include "extension.hpp"
#include "games/cs2/extensions/frame_input.hpp"
namespace mousebox::games::cs2::extensions
{
RemainingDamageExtension::RemainingDamageExtension(std::shared_ptr<application::ports::BallisticsProvider> provider)
    : scene_(std::move(provider))
{
}
const application::extensions::Definition &RemainingDamageExtension::definition() const noexcept
{
    return remainingDamageDefinition();
}
void RemainingDamageExtension::reset() noexcept
{
    scene_.reset();
}
application::extensions::Contribution RemainingDamageExtension::evaluate(
    const application::extensions::Input<Snapshot> &in, std::stop_token stop)
{
    application::extensions::Contribution out;
    const auto wait = [&](const char *code, const char *message) {
        out.state.phase = "waiting";
        out.state.code = code;
        out.state.message = message;
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
        return wait("dependency-pending", "等待穿透运行库与参数包");
    const auto scene = scene_.acquire(in.scope, *f.map, f.build, in.observedAtMs);
    if (!scene)
    {
        out.state.phase = "error";
        out.state.code = "scene-unavailable";
        out.state.message = scene_.error();
        return out;
    }
    auto result = std::make_shared<RemainingDamageResult>();
    result->scope = in.scope;
    result->capture = in.capture;
    result->shooter = *self;
    for (const auto &p : f.players)
    {
        if (stop.stop_requested())
            return wait("cancelled", "旧穿透分析已取消");
        if (!livingEnemy(p, *self))
            continue;
        if (result->actors.size() == 64)
        {
            ++out.state.unknown;
            break;
        }
        ActorDamage actor{p.controller, p.body->entity.id, {}};
        if (const auto head = headEndpoint(f, *p.body))
        {
            actor.prediction = scene->predict(self->definition, {self->origin.x, self->origin.y, self->origin.z},
                                              {head->x, head->y, head->z});
            ++out.state.queries;
        }
        else
            actor.prediction.code = "head-unavailable";
        if (actor.prediction.outcome == application::ports::ShotOutcome::Unknown)
            ++out.state.unknown;
        result->actors.push_back(std::move(actor));
    }
    out.state.phase = out.state.unknown ? "degraded" : "ready";
    out.state.code = out.state.unknown ? "partial-paths" : "static-estimate";
    out.state.message =
        out.state.unknown ? "部分头部路径无法确认，余伤显示为未知" : "当前武器到敌方头部的静态穿透余伤已计算";
    out.attachment = std::move(result);
    return out;
}
} // namespace mousebox::games::cs2::extensions
