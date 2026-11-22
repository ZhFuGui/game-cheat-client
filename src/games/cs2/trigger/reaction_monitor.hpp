#pragma once
#include "games/cs2/control/static_cover.hpp"
#include "reaction.hpp"
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>

namespace mousebox::games::cs2::trigger
{
// Keep one running bone scan and one replaceable observation. The static scene
// comes from the shared loader and is independent of a scan's completion.
class ReactionMonitor
{
    struct Job
    {
        std::shared_ptr<const Snapshot> frame;
        std::shared_ptr<const application::ports::VisibilityScene> scene;
        TargetFilter filter;
        std::int64_t startedNs = 0;
        std::uint64_t generation = 0, order = 0;
    };
    struct Result
    {
        std::vector<Exposure> exposures;
        std::int64_t startedNs = 0;
        std::uint64_t order = 0;
        bool failed = false;
    };

    mutable std::mutex mutex_;
    std::condition_variable_any wake_;
    std::uint64_t generation_ = 1, sceneGeneration_ = 0, order_ = 0;
    std::int64_t startedNs_ = 0;
    std::shared_ptr<const Snapshot> frame_;
    std::optional<Job> pending_;
    std::optional<Result> result_;
    std::jthread worker_;
    void clear();
    void run(std::stop_token);

  public:
    struct Evidence
    {
        bool current = false;
        bool failed = false;
        std::optional<Exposure> exposure;
    };
    ~ReactionMonitor();
    void reset();
    void observe(const ControlInput &, const StaticCover::Access &, TargetFilter);
    std::optional<Evidence> evidence(const Snapshot &, TargetKey) const;
};
} // namespace mousebox::games::cs2::trigger
