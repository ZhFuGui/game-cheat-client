#pragma once
#include "analysis.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>

namespace mousebox::games::cs2::rage
{
struct Result
{
    ControlInput input;
    Analysis analysis;
    std::shared_ptr<const application::ports::VisibilityScene> visibility;
    std::shared_ptr<const application::ports::BallisticsScene> ballistics;
    TargetFilter filter;
    ScorePolicy policy;
    bool coverCheck = true;
    bool otherPlayerCheck = true;
};
class Worker
{
    std::shared_ptr<application::ports::VisibilityProvider> visibility_;
    std::shared_ptr<application::ports::BallisticsProvider> ballistics_;
    mutable std::mutex mutex_;
    std::condition_variable_any wake_;
    struct Job
    {
        ControlInput input;
        TargetFilter filter;
        ScorePolicy policy;
        bool coverCheck = true;
        bool otherPlayerCheck = true;
    };
    std::optional<Job> pending_;
    std::shared_ptr<const Snapshot> submitted_;
    TargetFilter submittedFilter_;
    ScorePolicy submittedPolicy_;
    bool submittedCoverCheck_ = true;
    bool submittedOtherPlayerCheck_ = true;
    std::shared_ptr<const Result> result_;
    std::uint64_t generation_ = 1;
    std::jthread worker_;
    void run(std::stop_token);

  public:
    Worker(std::shared_ptr<application::ports::VisibilityProvider>,
           std::shared_ptr<application::ports::BallisticsProvider>);
    ~Worker();
    void submit(const ControlInput &, TargetFilter = {}, const ScorePolicy & = {}, bool coverCheck = true,
                bool otherPlayerCheck = true);
    std::shared_ptr<const Result> result() const;
    void reset();
};
} // namespace mousebox::games::cs2::rage
