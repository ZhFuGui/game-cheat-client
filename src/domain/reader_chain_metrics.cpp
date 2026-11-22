#include "reader_chain_metrics.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace mousebox::domain
{
void ReaderChainMetrics::reset()
{
    buckets_.clear();
    previous_.reset();
    reasons_.fill("waiting-data");
}
void ReaderChainMetrics::observe(const ReaderChainSample &s)
{
    if (!s.capture || !s.epoch || s.reader.empty() || s.clockDomain.empty() || !std::isfinite(s.receivedAtMs) ||
        s.receivedAtMs < 0 || s.receivedAtMs >= static_cast<double>(UINT64_MAX))
        return;
    if (previous_ &&
        (s.reader != previous_->reader || s.clockDomain != previous_->clockDomain || s.epoch != previous_->epoch))
        reset();
    if (previous_ && (s.capture <= previous_->capture || s.receivedAtMs < previous_->receivedAtMs))
        return;
    previous_ = s;
    const auto now = static_cast<std::uint64_t>(s.receivedAtMs);
    while (!buckets_.empty() && now >= buckets_.front().at && now - buckets_.front().at >= 2000)
        buckets_.pop_front();
    if (buckets_.empty() || buckets_.back().at != now)
        buckets_.push_back({now, {}});
    // At most one bucket per millisecond in a two-second interval.
    while (buckets_.size() > 2000)
        buckets_.pop_front();
    auto &values = buckets_.back().values;
    const auto add = [&](unsigned stage, long double value, double uncertainty = 0) {
        if (!std::isfinite(value) || value < 0 || value > std::numeric_limits<double>::max())
        {
            reasons_[stage] = "invalid-timing";
            return;
        }
        ++values[stage].count;
        values[stage].sum += static_cast<double>(value);
        values[stage].uncertainty = std::max(values[stage].uncertainty, uncertainty);
        reasons_[stage] = "no-samples";
    };
    const auto interval = [&](unsigned stage, const auto &first, const auto &last) {
        if (!first || !last)
            reasons_[stage] = "missing-timing";
        else if (*first < 0 || *last < *first)
            reasons_[stage] = "invalid-timing";
        else
            add(stage, (static_cast<long double>(*last) - *first) / 1e6L);
    };
    interval(0, s.startedNs, s.readFinishedNs);
    interval(1, s.readFinishedNs, s.sendStartedNs);
    if (!s.sendStartedNs)
        reasons_[2] = "missing-timing";
    else if (!s.clock || s.clock->sourceAnchorNs <= 0 || !std::isfinite(s.clock->localAnchorMs) ||
             !std::isfinite(s.clock->uncertaintyMs) || s.clock->uncertaintyMs < 0 ||
             s.receivedAtMs < s.clock->calibratedAtMs || s.receivedAtMs - s.clock->calibratedAtMs > 3000)
        reasons_[2] = "waiting-clock";
    else if (*s.sendStartedNs < 0)
        reasons_[2] = "invalid-timing";
    else
    {
        const auto &c = *s.clock;
        const auto sent = c.localAnchorMs + (static_cast<long double>(*s.sendStartedNs) - c.sourceAnchorNs) / 1e6L;
        add(2, s.receivedAtMs - sent, c.uncertaintyMs);
    }
}
ReaderChainSnapshot ReaderChainMetrics::snapshot(std::uint64_t now) const
{
    ReaderChainSnapshot result;
    const std::array<ReaderStageAverage *, 3> stages{&result.read, &result.prepare, &result.transport};
    std::array<double, 3> sums{}, errors{};
    for (const auto &b : buckets_)
        if (b.at <= now && now - b.at < 2000)
            for (unsigned i = 0; i < stages.size(); ++i)
            {
                stages[i]->samples += b.values[i].count;
                sums[i] += b.values[i].sum;
                errors[i] = std::max(errors[i], b.values[i].uncertainty);
            }
    for (unsigned i = 0; i < stages.size(); ++i)
    {
        auto &stage = *stages[i];
        stage.status = reasons_[i];
        if (stage.samples && std::isfinite(sums[i]))
        {
            stage.averageMs = sums[i] / static_cast<double>(stage.samples);
            stage.uncertaintyMs = errors[i];
            stage.status = "ready";
        }
    }
    return result;
}
} // namespace mousebox::domain
