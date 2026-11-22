#include "reader_metrics.hpp"
#include "games/pubg/model/source_sample.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace mousebox::application
{
namespace
{
template <class A, class B> bool sameSource(const A &a, const B &b)
{
    return a.reader == b.reader && a.session == b.session && a.epoch == b.epoch && a.clockDomain == b.clockDomain &&
           a.mappingRevision == b.mappingRevision;
}
template <class H> bool currentClock(const H &h, double now)
{
    if (!h.sourceClock)
        return false;
    const auto &c = *h.sourceClock;
    return c.sourceAnchorNs > 0 && std::isfinite(c.localAnchorMs) && std::isfinite(c.uncertaintyMs) &&
           c.uncertaintyMs >= 0 && now >= c.calibratedAtMs && now - c.calibratedAtMs <= 3000;
}
} // namespace
ReaderMetrics::Channel *ReaderMetrics::find(unsigned category)
{
    const auto it = std::ranges::find(channels_, category, &Channel::category);
    return it == channels_.end() ? nullptr : &*it;
}
ReaderMetrics::Bucket &ReaderMetrics::bucket(Window &window, std::uint64_t now)
{
    auto &b = window[now % window.size()];
    if (b.time != now)
        b = Bucket{now};
    return b;
}
void ReaderMetrics::subscribe(std::vector<std::pair<unsigned, std::string>> channels)
{
    const std::lock_guard lock(mutex_);
    channels_.clear();
    for (auto &[category, name] : channels)
    {
        if (channels_.size() == 16 || find(category))
            continue;
        Channel c;
        c.category = category;
        c.name = std::move(name);
        channels_.push_back(std::move(c));
    }
    // Keep recent traffic across reconnect / disconnect: those bytes really
    // crossed Reader TCP sockets within this same trailing second.
}
std::uint64_t ReaderMetrics::begin(unsigned category)
{
    const std::lock_guard lock(mutex_);
    auto *c = find(category);
    if (!c)
        return 0;
    c->connection = ++nextConnection_;
    c->current.reset();
    c->scope.reset();
    c->connected = false;
    c->completedCapture = 0;
    c->receivedCapture = 0;
    c->received = {};
    c->window = {};
    c->chain.reset();
    return c->connection;
}
void ReaderMetrics::observe(unsigned category, const control::Header *header, std::uint64_t now,
                            const domain::ReaderChainSample *chain)
{
    const std::lock_guard lock(mutex_);
    auto *c = find(category);
    if (!c)
        return;
    if (header && header->consumerConnection == c->connection)
    {
        if (chain)
            c->chain.observe(*chain);
        if (c->scope && !sameSource(*c->scope, *header))
        {
            c->completedCapture = 0;
            c->window = {};
            c->receivedCapture = 0;
        }
        if (header->capture > c->receivedCapture)
        {
            c->receivedCapture = header->capture;
            c->received.add(now);
        }
        c->scope = *header;
        c->connected = header->current;
        c->current = *header;
    }
    else
    {
        c->connected = false;
        c->current.reset();
        c->window = {};
        c->chain.reset();
    }
}
void ReaderMetrics::traffic(std::uint64_t now, std::uint64_t upload, std::uint64_t download)
{
    const std::lock_guard lock(mutex_);
    auto &b = bucket(traffic_, now);
    b.upload += upload;
    b.download += download;
}
void ReaderMetrics::completed(std::span<const control::Header> headers, double now)
{
    std::vector<domain::SourceSample> samples;
    samples.reserve(headers.size());
    for (const auto &header : headers)
        samples.push_back(games::pubg::sourceSample(header));
    completed(samples, now);
}
void ReaderMetrics::completed(std::span<const domain::SourceSample> headers, double now)
{
    if (!std::isfinite(now) || now < 0 || now >= static_cast<double>(UINT64_MAX))
        return;
    const std::lock_guard lock(mutex_);
    for (const auto &h : headers)
    {
        auto *c = find(h.channel);
        if (!c || !c->current || !c->connected || !h.current || !h.capture || h.consumerConnection != c->connection ||
            !sameSource(h, *c->current) || h.capture <= c->completedCapture)
            continue;
        // First completed draw only. Resize/theme redraws and repeated state
        // notifications must not change the average of the same source batch.
        c->completedCapture = h.capture;
        if (h.sourceStartNs <= 0 || h.sourceFinishNs < h.sourceStartNs || !currentClock(h, now))
            continue;
        const auto &clock = *h.sourceClock;
        const long double start =
            clock.localAnchorMs + (static_cast<long double>(h.sourceStartNs) - clock.sourceAnchorNs) / 1e6L;
        const auto delay = static_cast<long double>(now) - start;
        // Never substitute receive time, RTT/2, sample midpoint, or a clamped
        // zero when source-clock alignment cannot support this measurement.
        if (!std::isfinite(delay) || delay < 0 || delay > std::numeric_limits<double>::max())
            continue;
        auto &b = bucket(c->window, static_cast<std::uint64_t>(now));
        ++b.count;
        b.delay += static_cast<double>(delay);
        b.uncertainty = std::max(b.uncertainty, clock.uncertaintyMs);
    }
}
ReaderMetricsSnapshot ReaderMetrics::snapshot(std::uint64_t now) const
{
    const std::lock_guard lock(mutex_);
    const auto inWindow = [now](const Bucket &b) { return b.time <= now && now - b.time < 1000; };
    ReaderMetricsSnapshot result;
    for (const auto &b : traffic_)
        if (inWindow(b))
        {
            result.uploadBytesPerSecond += b.upload;
            result.downloadBytesPerSecond += b.download;
        }
    for (const auto &c : channels_)
    {
        ReaderChannelMetrics value;
        value.category = c.category;
        value.connection = c.connection;
        value.name = c.name;
        value.receivedFramesPerSecond = c.received.count(now);
        value.chain = c.chain.snapshot(now);
        value.status = !c.connected || !c.current                            ? "waiting-data"
                       : !currentClock(*c.current, static_cast<double>(now)) ? "waiting-clock"
                                                                             : "waiting-draw";
        double sum = 0, uncertainty = 0;
        if (value.status == "waiting-draw")
            for (const auto &b : c.window)
                if (inWindow(b))
                {
                    value.samples += b.count;
                    sum += b.delay;
                    uncertainty = std::max(uncertainty, b.uncertainty);
                }
        if (value.samples && std::isfinite(sum))
        {
            value.averageMs = sum / static_cast<double>(value.samples);
            value.uncertaintyMs = uncertainty;
            value.status = "ready";
        }
        result.channels.push_back(std::move(value));
    }
    return result;
}
} // namespace mousebox::application
