#pragma once
#include "serial_host.hpp"
#include <map>

namespace mousebox::application::extensions
{
// Existing extensions retain their shared serial lane. A feature can opt into
// its own bounded mailbox/worker and publish without holding up that lane.
template <class Frame> class Host
{
    struct Independent
    {
        std::string id;
        unsigned maximumAge;
        std::unique_ptr<SerialHost<Frame>> worker;
    };
    std::unique_ptr<SerialHost<Frame>> shared_;
    std::vector<Independent> independent_;
    std::vector<std::string> order_;

  public:
    explicit Host(std::string_view game, std::vector<std::unique_ptr<Extension<Frame>>> extensions = {})
    {
        if (extensions.size() > 32)
            throw std::invalid_argument("Too many extensions for one game");
        std::set<std::string> ids, keys;
        std::vector<std::unique_ptr<Extension<Frame>>> shared;
        for (auto &extension : extensions)
        {
            if (!extension)
                throw std::invalid_argument("Empty extension registration");
            const auto &d = extension->definition();
            if (d.game != game || !ids.insert(d.id).second || !keys.insert(d.enabledSetting).second)
                throw std::invalid_argument("Invalid or duplicate game extension registration");
            order_.push_back(d.id);
            if (extension->independentWorker())
            {
                const auto id = d.id;
                const auto age = std::clamp(extension->maximumResultAgeMs(), 1u, 5000u);
                std::vector<std::unique_ptr<Extension<Frame>>> lane;
                lane.push_back(std::move(extension));
                independent_.push_back({id, age, std::make_unique<SerialHost<Frame>>(game, std::move(lane))});
            }
            else
                shared.push_back(std::move(extension));
        }
        shared_ = std::make_unique<SerialHost<Frame>>(game, std::move(shared));
    }
    void shutdown()
    {
        // Cancel every lane before joining any slow worker.
        reset();
        shared_->shutdown();
        for (auto &lane : independent_)
            lane.worker->shutdown();
    }
    void setNotifier(std::function<void()> fn)
    {
        shared_->setNotifier(fn);
        for (auto &lane : independent_)
            lane.worker->setNotifier(fn);
    }
    void configure(const Json &values)
    {
        shared_->configure(values);
        for (auto &lane : independent_)
            lane.worker->configure(values);
    }
    void reset()
    {
        if (shared_)
            shared_->reset();
        for (auto &lane : independent_)
            lane.worker->reset();
    }
    void submit(Input<Frame> input)
    {
        shared_->submit(input);
        for (auto &lane : independent_)
            lane.worker->submit(input);
    }
    std::shared_ptr<const Result<Frame>> latest(const Scope &scope, std::uint64_t now, unsigned age) const
    {
        return shared_->latest(scope, now, age);
    }
    std::shared_ptr<const Result<Frame>> latestFor(std::string_view id, const Scope &scope, std::uint64_t now) const
    {
        for (const auto &lane : independent_)
            if (lane.id == id)
                return lane.worker->latest(scope, now, lane.maximumAge);
        return shared_->latest(scope, now, 250);
    }
    std::vector<State> states(std::uint64_t now = 0, unsigned age = 250) const
    {
        std::map<std::string, State> byId;
        for (auto state : shared_->states(now, age))
            byId.emplace(state.id, std::move(state));
        for (const auto &lane : independent_)
            for (auto state : lane.worker->states(now, lane.maximumAge))
                byId.emplace(state.id, std::move(state));
        std::vector<State> out;
        for (const auto &id : order_)
            if (const auto found = byId.find(id); found != byId.end())
                out.push_back(found->second);
        return out;
    }
};
} // namespace mousebox::application::extensions
