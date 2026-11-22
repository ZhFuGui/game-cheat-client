#pragma once

#include <condition_variable>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace mousebox::application
{

// A single bounded FIFO for file work. Tasks own their payloads; their owner
// remains responsible for publishing completion under its own state lock.
class BackgroundQueue
{
  public:
    using FailureHandler = std::function<void(const std::string &)>;

    BackgroundQueue() = default;
    ~BackgroundQueue();
    BackgroundQueue(const BackgroundQueue &) = delete;
    BackgroundQueue &operator=(const BackgroundQueue &) = delete;

    void start(FailureHandler);
    void enqueue(std::function<void()>);
    bool waitIdle(std::chrono::milliseconds timeout);
    // Drain accepted work before joining. No state lock used by a task may be
    // held while calling this function.
    void drainAndStop();

  private:
    static constexpr std::size_t Capacity = 4;
    std::mutex mutex_;
    std::condition_variable_any wake_;
    std::condition_variable idle_;
    std::deque<std::function<void()>> jobs_;
    std::jthread worker_;
    bool stopping_ = false;
    bool active_ = false;
};
} // namespace mousebox::application
