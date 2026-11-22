#pragma once
#include "application/json.hpp"
#include "domain/source_sample.hpp"
#include "domain/source_receipt.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <stop_token>
#include <string>
#include <vector>

namespace mousebox::application::extensions
{
struct Definition
{
    std::string id, game, title, enabledSetting;
    unsigned version = 1;
};
struct Scope
{
    std::string source, clock, map, mapResource;
    std::uint64_t epoch = 0, mapGeneration = 0;
    std::uint32_t process = 0;
    bool operator==(const Scope &) const = default;
};
struct State
{
    std::string id, phase = "disabled", message;
    bool enabled = false;
    std::uint64_t capture = 0;
    unsigned queries = 0, visible = 0, occluded = 0, unknown = 0;
    std::string code;
};
struct Attachment
{
    virtual ~Attachment() = default;
};
template <class Frame> struct Input
{
    std::shared_ptr<const Frame> frame;
    Scope scope;
    std::uint64_t capture = 0, revision = 0, observedAtMs = 0;
    std::optional<domain::SourceSample> sample;
    std::optional<domain::SourceReceipt> receipt;
};
struct Contribution
{
    State state;
    std::shared_ptr<const Attachment> attachment;
};
template <class Frame> class Extension
{
  public:
    virtual ~Extension() = default;
    virtual const Definition &definition() const noexcept = 0;
    // Opt-in lane: expensive optional work must not serialize unrelated analysis.
    virtual bool independentWorker() const noexcept
    {
        return false;
    }
    virtual unsigned maximumResultAgeMs() const noexcept
    {
        return 250;
    }
    // Pure main-thread extraction; only this extension's configuration is copied
    // into its immutable work item. Application validation owns persisted keys.
    virtual Json settings(const Json &) const
    {
        return Json::object();
    }
    // Serial worker call, idempotent and separate from slow scene loading.
    virtual void configure(const Json &) {}
    // Serial worker ownership; no source, device or renderer calls are exposed.
    virtual Contribution evaluate(const Input<Frame> &, std::stop_token) = 0;
    virtual void reset() noexcept = 0;
};
template <class Frame> struct Result
{
    Input<Frame> input;
    std::vector<Contribution> contributions;
    template <class T> std::shared_ptr<const T> find(std::string_view id) const
    {
        for (const auto &c : contributions)
            if (c.state.id == id)
                return std::dynamic_pointer_cast<const T>(c.attachment);
        return {};
    }
};
inline Json describe(const std::vector<State> &states)
{
    auto out = Json::array();
    for (const auto &s : states)
        out.push_back({{"id", s.id},
                       {"phase", s.phase},
                       {"code", s.code},
                       {"message", s.message.substr(0, 1024)},
                       {"enabled", s.enabled},
                       {"capture", std::to_string(s.capture)},
                       {"queries", s.queries},
                       {"visible", s.visible},
                       {"occluded", s.occluded},
                       {"unknown", s.unknown}});
    return out;
}
} // namespace mousebox::application::extensions
