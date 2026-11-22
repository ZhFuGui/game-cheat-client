#include "background_queue.hpp"
#include <stdexcept>

namespace mousebox::application
{
BackgroundQueue::~BackgroundQueue()
{
    drainAndStop();
}

void BackgroundQueue::start(FailureHandler failed)
{
    if (worker_.joinable())
        throw std::logic_error("后台文件队列已启动");
    worker_ = std::jthread([this, failed = std::move(failed)](std::stop_token stop) {
        for (;;)
        {
            std::function<void()> job;
            {
                std::unique_lock lock(mutex_);
                wake_.wait(lock, stop, [&] { return !jobs_.empty(); });
                if (jobs_.empty())
                {
                    if (stop.stop_requested())
                        return;
                    continue;
                }
                job = std::move(jobs_.front());
                jobs_.pop_front();
                active_ = true;
            }
            try
            {
                job();
            }
            catch (const std::exception &error)
            {
                failed(error.what());
            }
            {
                const std::lock_guard lock(mutex_);
                active_ = false;
            }
            idle_.notify_all();
        }
    });
}

bool BackgroundQueue::waitIdle(std::chrono::milliseconds timeout)
{
    std::unique_lock lock(mutex_);
    return idle_.wait_for(lock, timeout, [this] { return jobs_.empty() && !active_; });
}

void BackgroundQueue::enqueue(std::function<void()> job)
{
    {
        const std::lock_guard lock(mutex_);
        if (stopping_)
            throw std::runtime_error("后台文件队列已关闭");
        if (jobs_.size() >= Capacity)
            throw std::runtime_error("文件任务队列已满");
        jobs_.push_back(std::move(job));
    }
    wake_.notify_one();
}

void BackgroundQueue::drainAndStop()
{
    {
        const std::lock_guard lock(mutex_);
        stopping_ = true;
    }
    worker_.request_stop();
    wake_.notify_all();
    if (worker_.joinable())
        worker_.join();
}
} // namespace mousebox::application
