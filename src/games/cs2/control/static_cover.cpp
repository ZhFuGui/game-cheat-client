#include "static_cover.hpp"
#include "pipeline/angles.hpp"
namespace mousebox::games::cs2
{
StaticCover::~StaticCover()
{
    worker_.request_stop();
    wake_.notify_all();
}
void StaticCover::reset()
{
    std::lock_guard lock(mutex_);
    if (!requested_)
        return;
    ++generation_;
    requested_.reset();
    pending_.reset();
    result_.reset();
}
StaticCover::Access StaticCover::acquire(const ControlInput &in)
{
    if (!in.frame || !in.frame->map || !in.frame->map->available())
    {
        reset();
        return {nullptr, "cover-map-missing"};
    }
    if (!provider_)
        return {nullptr, "cover-unavailable"};
    const auto &f = *in.frame;
    const auto &map = *f.map;
    const Scope scope{f.instance,
                      f.clockDomain,
                      map.name,
                      map.resourcePath,
                      f.epoch,
                      map.generation,
                      in.sample ? in.sample->session : 0,
                      in.sample ? in.sample->consumerConnection : 0,
                      f.processId,
                      f.local.controller.value_or(0),
                      f.local.pawn.value_or(0)};
    std::lock_guard lock(mutex_);
    const bool retry = result_ && !result_->scene && std::chrono::steady_clock::now() >= result_->retryAfter;
    if (requested_ != scope || retry)
    {
        requested_ = scope;
        result_.reset();
        pending_ = Job{scope, ++generation_};
        if (!worker_.joinable())
            worker_ = std::jthread([this](std::stop_token stop) { run(stop); });
        wake_.notify_all();
    }
    if (!result_)
        return {nullptr, "cover-pending", generation_};
    return {result_->scene, result_->scene ? "ready" : "cover-unavailable", generation_};
}
std::string StaticCover::Access::check(Vec3 origin, Vec3 point) const
{
    if (!scene)
        return block;
    if (!pipeline::finite(origin) || !pipeline::finite(point))
        return "cover-unknown";
    try
    {
        using application::ports::Visibility;
        const auto visible = scene->query({origin.x, origin.y, origin.z}, {point.x, point.y, point.z});
        return visible == Visibility::Visible    ? "ready"
               : visible == Visibility::Occluded ? "cover-occluded"
                                                 : "cover-unknown";
    }
    catch (...)
    {
        return "cover-unavailable";
    }
}
void StaticCover::run(std::stop_token stop)
{
    while (!stop.stop_requested())
    {
        Job job;
        {
            std::unique_lock lock(mutex_);
            if (!wake_.wait(lock, stop, [&] { return pending_.has_value(); }))
                return;
            job = std::move(*pending_);
            pending_.reset();
        }
        Result result;
        try
        {
            if (provider_->available())
                result.scene = provider_->load(job.scope.map, job.scope.resource);
        }
        catch (...)
        {
            // Unavailable resources cannot qualify as visible; retry is bounded.
        }
        result.retryAfter = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        std::lock_guard lock(mutex_);
        if (!stop.stop_requested() && job.generation == generation_ && requested_ == job.scope)
            result_ = std::move(result);
    }
}
} // namespace mousebox::games::cs2
