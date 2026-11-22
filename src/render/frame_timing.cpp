#include "frame_timing.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace mousebox::render
{
namespace
{
bool sameScope(const domain::SourceReceipt &a, const domain::SourceReceipt &b)
{
    return a.channel == b.channel && a.reader == b.reader && a.session == b.session && a.epoch == b.epoch &&
           a.connection == b.connection;
}
bool valid(const domain::SourceReceipt &s)
{
    return s.channel && !s.reader.empty() && s.epoch && s.capture && s.connection && std::isfinite(s.receivedAtMs) &&
           s.receivedAtMs >= 0;
}
} // namespace
void FrameTiming::select(const std::optional<domain::SourceReceipt> &next)
{
    if (next && valid(*next) && scope_ && sameScope(*next, *scope_))
        return;
    scope_ = next && valid(*next) ? next : std::nullopt;
    buckets_.clear();
    order_.clear();
    seen_.clear();
    retired_ = 0;
}
void FrameTiming::completed(const FrameDrawStart &frame, double end)
{
    const auto &s = frame.receipt;
    if (!scope_ || !valid(s) || !sameScope(s, *scope_) || s.capture <= retired_ || !seen_.insert(s.capture).second)
        return;
    order_.push_back(s.capture);
    if (order_.size() > 4096)
    {
        retired_ = std::max(retired_, order_.front());
        seen_.erase(order_.front());
        order_.pop_front();
    }
    if (!std::isfinite(frame.startedAtMs) || !std::isfinite(end) || frame.startedAtMs < s.receivedAtMs ||
        end < frame.startedAtMs || end >= static_cast<double>(UINT64_MAX))
        return;
    const auto now = static_cast<std::uint64_t>(end);
    // Completion notifications are ordered on the renderer's worker.
    if (!buckets_.empty() && now < buckets_.back().at)
        return;
    while (!buckets_.empty() && now - buckets_.front().at >= 2000)
        buckets_.pop_front();
    if (buckets_.empty() || buckets_.back().at != now)
        buckets_.push_back({now});
    auto &b = buckets_.back();
    ++b.count;
    b.processing += frame.startedAtMs - s.receivedAtMs;
    b.drawing += end - frame.startedAtMs;
}
FrameTimingSnapshot FrameTiming::snapshot(double now) const
{
    FrameTimingSnapshot s;
    if (!std::isfinite(now) || now < 0)
        return s;
    double processing = 0, drawing = 0;
    for (const auto &b : buckets_)
        if (b.at <= now && now - b.at < 2000)
        {
            s.samples += b.count;
            processing += b.processing;
            drawing += b.drawing;
        }
    if (s.samples && std::isfinite(processing) && std::isfinite(drawing))
    {
        s.processingMs = processing / static_cast<double>(s.samples);
        s.drawingMs = drawing / static_cast<double>(s.samples);
    }
    return s;
}
} // namespace mousebox::render
