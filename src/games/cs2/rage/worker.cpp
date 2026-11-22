#include "worker.hpp"
#include "games/cs2/extensions/ballistics_session.hpp"
#include "games/cs2/extensions/frame_input.hpp"

namespace mousebox::games::cs2::rage
{
Worker::Worker(std::shared_ptr<application::ports::VisibilityProvider> v,
               std::shared_ptr<application::ports::BallisticsProvider> b)
    : visibility_(std::move(v)), ballistics_(std::move(b))
{
}
Worker::~Worker()
{
    worker_.request_stop();
    wake_.notify_all();
    if (worker_.joinable())
        worker_.join();
}
void Worker::reset()
{
    std::lock_guard lock(mutex_);
    ++generation_;
    pending_.reset();
    submitted_.reset();
    result_.reset();
}
void Worker::submit(const ControlInput &in, TargetFilter filter, const ScorePolicy &policy, bool coverCheck,
                    bool otherPlayerCheck)
{
    std::lock_guard lock(mutex_);
    if (!in.frame || (submitted_ == in.frame && submittedFilter_ == filter && submittedPolicy_ == policy &&
                      submittedCoverCheck_ == coverCheck && submittedOtherPlayerCheck_ == otherPlayerCheck))
        return;
    submitted_ = in.frame;
    submittedFilter_ = filter;
    submittedPolicy_ = policy;
    submittedCoverCheck_ = coverCheck;
    submittedOtherPlayerCheck_ = otherPlayerCheck;
    pending_ = Job{in, filter, policy, coverCheck, otherPlayerCheck};
    if (!worker_.joinable())
        worker_ = std::jthread([this](std::stop_token stop) { run(stop); });
    wake_.notify_all();
}
std::shared_ptr<const Result> Worker::result() const
{
    std::lock_guard lock(mutex_);
    return result_;
}
void Worker::run(std::stop_token stop)
{
    extensions::BallisticsSession damage(ballistics_);
    std::shared_ptr<const application::ports::VisibilityScene> scene;
    std::optional<application::extensions::Scope> scope;
    auto retry = std::chrono::steady_clock::time_point{};
    while (!stop.stop_requested())
    {
        ControlInput in;
        TargetFilter filter;
        ScorePolicy policy;
        bool coverCheck, otherPlayerCheck;
        std::uint64_t generation;
        {
            std::unique_lock lock(mutex_);
            if (!wake_.wait(lock, stop, [&] { return pending_.has_value(); }))
                return;
            in = std::move(pending_->input);
            filter = pending_->filter;
            policy = pending_->policy;
            coverCheck = pending_->coverCheck;
            otherPlayerCheck = pending_->otherPlayerCheck;
            pending_.reset();
            generation = generation_;
        }
        auto out = std::make_shared<Result>();
        out->input = in;
        out->filter = filter;
        out->policy = policy;
        out->coverCheck = coverCheck;
        out->otherPlayerCheck = otherPlayerCheck;
        out->analysis.block = "scene-unavailable";
        try
        {
            const auto &f = *in.frame;
            const auto current = extensions::scope(f);
            if (scope != current)
            {
                scope = current;
                scene.reset();
                retry = {};
            }
            if (f.map && f.map->available())
            {
                const bool visibilityReady = visibility_ && visibility_->available();
                if (coverCheck && visibilityReady && !scene && std::chrono::steady_clock::now() >= retry)
                {
                    retry = std::chrono::steady_clock::now() + std::chrono::seconds(5);
                    scene = visibility_->load(f.map->name, f.map->resourcePath);
                }
                const auto ballistic =
                    damage.available() ? damage.acquire(current, *f.map, f.build, in.observedAtMs) : nullptr;
                if ((!coverCheck || (visibilityReady && scene)) && ballistic && !stop.stop_requested())
                {
                    out->visibility = scene;
                    out->ballistics = ballistic;
                    out->analysis = analyze(f, scene.get(), *ballistic, filter, stop, policy, coverCheck);
                }
            }
        }
        catch (...)
        {
            out->analysis = {};
            out->analysis.block = "scene-unavailable";
        }
        std::lock_guard lock(mutex_);
        if (!stop.stop_requested() && generation_ == generation)
            result_ = std::move(out);
    }
}
} // namespace mousebox::games::cs2::rage
