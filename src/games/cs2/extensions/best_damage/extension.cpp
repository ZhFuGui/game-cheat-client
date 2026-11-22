#include "extension.hpp"
#include "games/cs2/extensions/frame_input.hpp"
#include "games/cs2/hitboxes/frame.hpp"
#include <algorithm>
#include <chrono>
namespace mousebox::games::cs2::extensions
{
BestDamageExtension::BestDamageExtension(std::shared_ptr<application::ports::BallisticsProvider> p)
    : scene_(std::move(p))
{
}
const application::extensions::Definition &BestDamageExtension::definition() const noexcept
{
    return bestDamageDefinition();
}
void BestDamageExtension::reset() noexcept
{
    scene_.reset();
    scope_.reset();
    shooter_.reset();
    seeds_.clear();
    nextActor_ = 0;
}
application::extensions::Contribution BestDamageExtension::evaluate(const application::extensions::Input<Snapshot> &in,
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
    if (!scope_ || *scope_ != in.scope || !shooter_ || shooter_->controller != self->controller ||
        shooter_->pawn != self->pawn || shooter_->weapon != self->weapon || shooter_->definition != self->definition ||
        shooter_->team != self->team)
    {
        seeds_.clear();
        nextActor_ = 0;
    }
    scope_ = in.scope;
    shooter_ = self;
    if (!scene_.available())
        return wait("dependency-pending", "等待伤害运行库");
    const auto scene = scene_.acquire(in.scope, *f.map, f.build, in.observedAtMs);
    if (!scene)
    {
        out.state.phase = "error";
        out.state.code = "scene-unavailable";
        out.state.message = scene_.error();
        return out;
    }
    auto result = std::make_shared<BestDamageResult>();
    result->scope = in.scope;
    result->capture = in.capture;
    result->shooter = *self;
    for (const auto &p : f.players)
        if (livingEnemy(p, *self) && result->actors.size() < 64)
            result->actors.push_back(
                {p.controller, p.body->entity.id, damageTarget(p), p.body->skeletonModel, p.body->skeletonProfile, {}});
    std::map<std::pair<std::uint32_t, std::uint32_t>, damage_search::Seed> nextSeeds;
    const auto count = result->actors.size();
    // Fair work distribution, a hard query cap and a deadline checked between batches. No Reader,
    // output or GPU thread waits here. Rotation prevents a costly first actor
    // from consuming every capture's budget.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(12);
    unsigned converged = 0;
    for (std::size_t n = 0; n < count; ++n)
    {
        if (stop.stop_requested())
            return wait("cancelled", "旧最高伤害点分析已取消");
        auto &actor = result->actors[(nextActor_ + n) % count];
        const auto key = std::pair(actor.controller, actor.pawn);
        const auto player = std::find_if(f.players.begin(), f.players.end(),
                                         [&](const Player &p) { return p.controller == actor.controller; });
        if (!actor.target)
            actor.search.code = "target-state-missing";
        else if (std::chrono::steady_clock::now() >= deadline)
            actor.search.code = "frame-budget";
        else
        {
            damage_search::Limits limits;
            limits.rays = std::min(192u, std::max(16u, 1536u / static_cast<unsigned>(count)));
            limits.deadline = deadline;
            const auto seed = seeds_.find(key);
            actor.search = damage_search::search(
                hitboxes::fromBody(f, *player->body), self->origin, self->definition, self->team, *actor.target, *scene,
                limits, seed == seeds_.end() ? std::nullopt : std::optional(seed->second), stop);
        }
        out.state.queries += actor.search.queries;
        if (!actor.search.best || actor.search.unknown)
            ++out.state.unknown;
        if (actor.search.converged)
            ++converged;
        if (actor.search.best)
            nextSeeds.emplace(key, actor.search.best->seed);
        else if (const auto prior = seeds_.find(key); prior != seeds_.end())
            nextSeeds.emplace(key, prior->second);
    }
    if (count)
        nextActor_ = (nextActor_ + 1) % count;
    seeds_ = std::move(nextSeeds);
    out.state.phase = "degraded";
    out.state.code = converged == count && count ? "bounded-best" : "budget-best";
    out.state.message = "静态掩体、默认规则、目标非无敌条件下搜索未封顶伤害；青色标出当前最佳瞄准点";
    out.attachment = std::move(result);
    return out;
}
} // namespace mousebox::games::cs2::extensions
