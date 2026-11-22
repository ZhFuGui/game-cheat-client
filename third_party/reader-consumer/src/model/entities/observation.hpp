#pragma once
#include "memory/issue.hpp"
namespace monitor {
// These values match the protocol enums; every sample is independently timed.
enum class OwnedReadStatus : unsigned { Available = 1, Partial = 2, Failed = 3, NotAttempted = 4, Unverified = 5 };
enum class OwnedCoverage : unsigned { Unknown = 0, Complete = 1, Partial = 2, Failed = 3, NotAttempted = 4 };

// No retained-value cache: both stamps describe this one acquisition interval.
// value_status is absent when this attempt produced no publishable value.
struct OwnedObservation {
    u64 capture_sequence = 0;
    Clock::time_point started{}, finished{};
    OwnedReadStatus latest_status = OwnedReadStatus::NotAttempted;
    std::optional<OwnedReadStatus> value_status;
    Issue issue;
};

} // namespace monitor
