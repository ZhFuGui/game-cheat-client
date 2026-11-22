#include "reaction_monitor.hpp"
#include <algorithm>

namespace mousebox::games::cs2::trigger
{
ReactionMonitor::~ReactionMonitor()
{
    worker_.request_stop();
    wake_.notify_all();
}
void ReactionMonitor::clear()
{
    ++generation_;
    frame_.reset();
    pending_.reset();
    result_.reset();
    order_ = 0;
}
void ReactionMonitor::reset()
{
    std::lock_guard lock(mutex_);
    if (frame_)
        clear();
}
void ReactionMonitor::observe(const ControlInput &in, const StaticCover::Access &access, TargetFilter filter)
{
    if (!access.scene || !in.frame || !in.sample)
    {
        reset();
        return;
    }
    std::lock_guard lock(mutex_);
    if (sceneGeneration_ != access.generation)
    {
        clear();
        sceneGeneration_ = access.generation;
    }
    if (in.frame == frame_)
        return;
    frame_ = in.frame;
    startedNs_ = in.sample->sourceStartNs;
    pending_ = Job{frame_, access.scene, filter, startedNs_, generation_, ++order_};
    if (!worker_.joinable())
        worker_ = std::jthread([this](std::stop_token stop) { run(stop); });
    wake_.notify_all();
}
std::optional<ReactionMonitor::Evidence> ReactionMonitor::evidence(const Snapshot &frame, TargetKey target) const
{
    std::lock_guard lock(mutex_);
    if (&frame != frame_.get() || !result_ || order_ - result_->order > 1 || startedNs_ < result_->startedNs)
        return {};
    const auto &items = result_->exposures;
    const auto match = std::find_if(items.begin(), items.end(), [&](const Exposure &e) { return e.id == target; });
    return Evidence{order_ == result_->order, result_->failed,
                    match == items.end() ? std::nullopt : std::optional(*match)};
}
void ReactionMonitor::run(std::stop_token stop)
{
    ReactionTimers timers;
    std::uint64_t generation = 0;
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
        if (generation != job.generation)
        {
            timers.reset();
            generation = job.generation;
        }
        Result result;
        result.order = job.order;
        result.startedNs = job.startedNs;
        try
        {
            result.exposures = scanTargets(*job.frame, *job.scene, job.filter, stop);
            timers.observe(result.exposures, job.startedNs, job.order);
        }
        catch (...)
        {
            result.exposures.clear();
            result.failed = true;
            timers.reset();
        }
        std::lock_guard lock(mutex_);
        if (!stop.stop_requested() && generation_ == job.generation)
            result_ = std::move(result);
    }
}
} // namespace mousebox::games::cs2::trigger
