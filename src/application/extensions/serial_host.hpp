#pragma once
#include "contracts.hpp"
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>
#include <utility>

namespace mousebox::application::extensions
{
// One latest-input slot and one immutable completed result. The Reader, output
// owner and render thread never wait for an extension's evaluation or scene I/O.
template <class Frame> class SerialHost
{
    struct Work
    {
        Input<Frame> input;
        std::uint64_t generation;
        std::vector<bool> enabled;
        std::vector<Json> configurations;
        std::stop_token cancel;
    };
    std::vector<std::unique_ptr<Extension<Frame>>> extensions_;
    mutable std::mutex mutex_;
    std::condition_variable wake_;
    std::optional<Work> pending_;
    std::shared_ptr<const Result<Frame>> completed_;
    std::optional<Scope> scope_;
    std::vector<bool> enabled_;
    std::vector<Json> configurations_;
    std::uint64_t generation_ = 1;
    std::stop_source cancellation_;
    bool resetRequested_ = false, closed_ = false;
    std::function<void()> notify_;
    std::jthread worker_;
    void invalidateLocked()
    {
        cancellation_.request_stop();
        cancellation_ = std::stop_source{};
        ++generation_;
        pending_.reset();
        completed_.reset();
        resetRequested_ = true;
    }
    void run(std::stop_token stop)
    {
        while (!stop.stop_requested())
        {
            std::optional<Work> work;
            bool reset = false;
            {
                std::unique_lock lock(mutex_);
                wake_.wait(lock, [&] { return closed_ || pending_ || resetRequested_; });
                if (closed_)
                    break;
                work = std::move(pending_);
                pending_.reset();
                reset = std::exchange(resetRequested_, false);
            }
            if (reset)
                for (auto &e : extensions_)
                    e->reset();
            if (!work || work->cancel.stop_requested())
                continue;
            auto result = std::make_shared<Result<Frame>>();
            result->input = work->input;
            for (std::size_t i = 0; i < extensions_.size(); ++i)
            {
                Contribution c;
                if (work->enabled[i])
                {
                    try
                    {
                        extensions_[i]->configure(work->configurations[i]);
                        c = extensions_[i]->evaluate(work->input, work->cancel);
                    }
                    catch (const std::exception &error)
                    {
                        c.state.phase = "error";
                        c.state.message = error.what();
                    }
                    catch (...)
                    {
                        c.state.phase = "error";
                        c.state.message = "Extension evaluation failed";
                    }
                }
                c.state.id = extensions_[i]->definition().id;
                c.state.enabled = work->enabled[i];
                c.state.capture = work->input.capture;
                result->contributions.push_back(std::move(c));
                if (work->cancel.stop_requested())
                    break;
            }
            std::function<void()> notify;
            {
                std::lock_guard lock(mutex_);
                if (!closed_ && work->generation == generation_ && !work->cancel.stop_requested())
                {
                    completed_ = std::move(result);
                    notify = notify_;
                }
            }
            if (notify)
                notify();
        }
        for (auto &e : extensions_)
            e->reset();
    }

  public:
    explicit SerialHost(std::string_view game, std::vector<std::unique_ptr<Extension<Frame>>> extensions = {})
        : extensions_(std::move(extensions)), enabled_(extensions_.size(), false),
          configurations_(extensions_.size(), Json::object())
    {
        std::set<std::string> ids, keys;
        if (extensions_.size() > 32)
            throw std::invalid_argument("Too many extensions for one game");
        for (const auto &e : extensions_)
        {
            if (!e)
                throw std::invalid_argument("Empty extension registration");
            const auto &d = e->definition();
            if (d.game != game || !d.id.starts_with(std::string(game) + ".") ||
                !d.enabledSetting.starts_with("extension." + d.id + ".") || d.version != 1 ||
                !ids.insert(d.id).second || !keys.insert(d.enabledSetting).second)
                throw std::invalid_argument("Invalid or duplicate game extension registration");
        }
        worker_ = std::jthread([this](std::stop_token stop) { run(stop); });
    }
    ~SerialHost()
    {
        shutdown();
    }
    void shutdown()
    {
        {
            std::lock_guard lock(mutex_);
            if (closed_)
                return;
            closed_ = true;
            cancellation_.request_stop();
            notify_ = {};
        }
        worker_.request_stop();
        wake_.notify_one();
        if (worker_.joinable())
            worker_.join();
    }
    void setNotifier(std::function<void()> fn)
    {
        std::lock_guard lock(mutex_);
        notify_ = std::move(fn);
    }
    void configure(const Json &values)
    {
        std::vector<bool> next;
        std::vector<Json> configurations;
        for (const auto &e : extensions_)
        {
            next.push_back(values.value(e->definition().enabledSetting, false));
            configurations.push_back(e->settings(values));
        }
        std::lock_guard lock(mutex_);
        if (next != enabled_ || configurations != configurations_)
        {
            enabled_ = std::move(next);
            configurations_ = std::move(configurations);
            invalidateLocked();
            wake_.notify_one();
        }
    }
    void reset()
    {
        std::lock_guard lock(mutex_);
        scope_.reset();
        invalidateLocked();
        wake_.notify_one();
    }
    void submit(Input<Frame> input)
    {
        if (!input.frame)
        {
            reset();
            return;
        }
        std::lock_guard lock(mutex_);
        if (closed_ || std::none_of(enabled_.begin(), enabled_.end(), [](bool enabled) { return enabled; }))
            return;
        if (scope_ != input.scope)
        {
            invalidateLocked();
            scope_ = input.scope;
        }
        pending_ = Work{std::move(input), generation_, enabled_, configurations_, cancellation_.get_token()};
        wake_.notify_one();
    }
    std::shared_ptr<const Result<Frame>> latest(const Scope &scope, std::uint64_t now, unsigned maximumAgeMs) const
    {
        std::lock_guard lock(mutex_);
        if (!completed_ || completed_->input.scope != scope || now < completed_->input.observedAtMs ||
            now - completed_->input.observedAtMs > maximumAgeMs)
            return {};
        return completed_;
    }
    std::vector<State> states(std::uint64_t now = 0, unsigned maximumAgeMs = 250) const
    {
        std::lock_guard lock(mutex_);
        std::vector<State> out;
        if (completed_)
            for (const auto &c : completed_->contributions)
            {
                auto state = c.state;
                if (state.enabled && now &&
                    (now < completed_->input.observedAtMs || now - completed_->input.observedAtMs > maximumAgeMs))
                {
                    state.phase = "waiting";
                    state.code = "stale";
                    state.message = "等待新鲜的扩展分析结果";
                }
                out.push_back(std::move(state));
            }
        else
            for (std::size_t i = 0; i < extensions_.size(); ++i)
            {
                State state;
                state.id = extensions_[i]->definition().id;
                state.enabled = enabled_[i];
                state.phase = state.enabled ? "waiting" : "disabled";
                state.code = state.enabled ? "waiting-data" : "disabled";
                out.push_back(std::move(state));
            }
        return out;
    }
};
} // namespace mousebox::application::extensions
