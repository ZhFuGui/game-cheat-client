#pragma once
#include "games/cs2/model/snapshot.hpp"
#include "games/cs2/model/target_filter.hpp"
#include "games/cs2/model/target_key.hpp"
#include "application/ports/scene_visibility.hpp"
#include "domain/source_clock.hpp"
#include <map>
#include <span>
#include <stop_token>
namespace mousebox::games::cs2::trigger
{
struct Exposure
{
    TargetKey id;
    application::ports::Visibility visibility = application::ports::Visibility::Unknown;
    std::optional<std::int64_t> visibleSinceNs;
    std::optional<unsigned> visibleBone;
};
// An existential test across all supplied body bones, independent of the aim ray.
std::vector<Exposure> scanTargets(const Snapshot &, const application::ports::VisibilityScene &, TargetFilter,
                                  std::stop_token);
bool remainsVisible(const Snapshot &, const application::ports::VisibilityScene &, TargetKey, unsigned bone);
// Source-domain starts survive clock-estimate refreshes; only observed continuous
// visibility may carry a start into the next observation. Missing observations reset.
class ReactionTimers
{
    std::map<TargetKey, std::int64_t> starts_;
    std::uint64_t order_ = 0;
    std::int64_t sourceNs_ = 0;

  public:
    void reset();
    void observe(std::vector<Exposure> &, std::int64_t sourceStartNs, std::uint64_t order);
};
// Caller must first qualify the current source clock and sample with the normal
// age/uncertainty policy. Includes read, transport, scheduling and analysis time.
double remainingReactionMs(std::int64_t visibleSinceNs, const control::SourceClockEstimate &, std::uint64_t nowMs,
                           unsigned delayMs) noexcept;
} // namespace mousebox::games::cs2::trigger
