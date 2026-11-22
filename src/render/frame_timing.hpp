#pragma once
#include "domain/source_receipt.hpp"
#include <deque>
#include <optional>
#include <unordered_set>
namespace mousebox::render
{
struct FrameDrawStart
{
    domain::SourceReceipt receipt;
    double startedAtMs = 0;
};
struct FrameTimingSnapshot
{
    std::optional<double> processingMs, drawingMs;
    std::uint64_t samples = 0;
};
// Pure bounded telemetry. Both means use the same first-completed source-frame
// cohort; redraws do not turn old data age into new processing samples.
class FrameTiming
{
    struct Bucket
    {
        std::uint64_t at = 0, count = 0;
        double processing = 0, drawing = 0;
    };
    std::optional<domain::SourceReceipt> scope_;
    std::deque<Bucket> buckets_;
    std::deque<std::uint64_t> order_;
    std::unordered_set<std::uint64_t> seen_;
    std::uint64_t retired_ = 0;

  public:
    void select(const std::optional<domain::SourceReceipt> &);
    void completed(const FrameDrawStart &, double completedAtMs);
    FrameTimingSnapshot snapshot(double nowMs) const;
};
} // namespace mousebox::render
