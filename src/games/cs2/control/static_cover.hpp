#pragma once
#include "aim_types.hpp"
#include "application/ports/scene_visibility.hpp"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
namespace mousebox::games::cs2
{
// Map resources are loaded off the control thread. Visibility itself is checked
// against the current selected point, never against a previous capture's result.
class StaticCover
{
    struct Scope
    {
        std::string reader, clock, map, resource;
        std::uint64_t epoch = 0, mapGeneration = 0, session = 0, connection = 0;
        std::uint32_t process = 0, controller = 0, pawn = 0;
        bool operator==(const Scope &) const = default;
    };
    struct Job
    {
        Scope scope;
        std::uint64_t generation = 0;
    };
    struct Result
    {
        std::shared_ptr<const application::ports::VisibilityScene> scene;
        std::chrono::steady_clock::time_point retryAfter;
    };
    std::shared_ptr<application::ports::VisibilityProvider> provider_;
    std::mutex mutex_;
    std::condition_variable_any wake_;
    std::uint64_t generation_ = 0;
    std::optional<Scope> requested_;
    std::optional<Job> pending_;
    std::optional<Result> result_;
    std::jthread worker_;
    void run(std::stop_token);

  public:
    explicit StaticCover(std::shared_ptr<application::ports::VisibilityProvider> provider = {})
        : provider_(std::move(provider))
    {
    }
    struct Access
    {
        std::shared_ptr<const application::ports::VisibilityScene> scene;
        const char *block = "cover-pending";
        std::uint64_t generation = 0;
        std::string check(Vec3 origin, Vec3 point) const;
    };

    ~StaticCover();
    void reset();
    Access acquire(const ControlInput &);
    std::string check(const ControlInput &in, Vec3 origin, Vec3 point)
    {
        return acquire(in).check(origin, point);
    }
};
} // namespace mousebox::games::cs2
