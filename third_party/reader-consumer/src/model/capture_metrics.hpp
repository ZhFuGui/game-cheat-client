#pragma once
#include <cstdint>

namespace monitor::capture {
enum class RecoveryState : unsigned {
    Ready = 1,
    Recovering = 2,
    WaitingTarget = 3,
    Unsupported = 4,
    Blocked = 5,
    Stopping = 6
};
struct TaskCoverage {
    std::uint64_t planned = 0, succeeded = 0, read_failed = 0, dependency_blocked = 0, unexecuted = 0;
    std::uint64_t Finished() const noexcept { return succeeded + read_failed + dependency_blocked; }
    bool Complete() const noexcept { return unexecuted == 0 && planned == Finished(); }
    TaskCoverage& operator+=(const TaskCoverage& other) noexcept {
        planned += other.planned;
        succeeded += other.succeeded;
        read_failed += other.read_failed;
        dependency_blocked += other.dependency_blocked;
        unexecuted += other.unexecuted;
        return *this;
    }
};
struct FrameCounters {
    std::uint64_t capture_opportunities = 0, capture_started = 0, capture_completed = 0, capture_accepted = 0,
                  capture_rejected = 0;
    std::uint64_t missed_schedule = 0, missed_busy = 0, missed_unavailable = 0, capture_counters_qpc = 0;
    std::uint64_t sent_frames = 0, sent_datagrams = 0, sent_bytes = 0, send_failures = 0, send_counters_qpc = 0;
};
struct FrameMetrics {
    std::uint64_t epoch = 0, opportunity = 0, scheduled_qpc = 0, started_qpc = 0, closed_qpc = 0, qpc_frequency = 0;
    std::uint64_t unique_actors = 0, completed_actors = 0, revoked_members = 0;
    std::uint64_t target_interval_ns = 0, pacing_revision = 0;
    double target_fps = 30;
    unsigned control_port = 0;
    RecoveryState recovery = RecoveryState::Ready;
    TaskCoverage tasks;
    FrameCounters counters;
};
struct PipelineStats {
    std::uint64_t opportunities = 0, opened = 0, closed = 0, rejected = 0, missed_busy = 0, missed_schedule = 0,
                  missed_unavailable = 0;
    std::uint64_t mailbox_overwrites = 0, stale_frames = 0, encoding_failures = 0, send_failures = 0;
    std::uint64_t sent_frames = 0, sent_datagrams = 0, sent_bytes = 0, export_sequence = 0;
};
} // namespace monitor::capture
