#pragma once
#include "model.hpp"
#include "adapters/sources/valorant_reader/client.hpp"
#include <limits>
#include <mutex>

namespace mousebox::games::valorant::aim
{
std::shared_ptr<const Frame> controlFrame(const sources::valorant_reader::Observation &);
class FrameCache
{
    std::mutex mutex_;
    std::uint64_t revision_ = std::numeric_limits<std::uint64_t>::max();
    std::shared_ptr<const Frame> frame_;
    std::array<std::optional<control::SourceClockEstimate>, 3> clocks_;

  public:
    std::shared_ptr<const Frame> read(const sources::valorant_reader::Observation &source)
    {
        const std::lock_guard lock(mutex_);
        bool changed = revision_ != source.revision;
        for (unsigned i = 0; i < clocks_.size(); ++i)
            if (clocks_[i] != source.channels[i].sample.sourceClock)
            {
                clocks_[i] = source.channels[i].sample.sourceClock;
                changed = true;
            }
        if (changed)
        {
            frame_ = controlFrame(source);
            revision_ = source.revision;
        }
        return frame_;
    }
};
} // namespace mousebox::games::valorant::aim
