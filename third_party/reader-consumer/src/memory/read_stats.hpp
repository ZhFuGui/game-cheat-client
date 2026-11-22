#pragma once
#include "model/primitives.hpp"
namespace monitor {
// Per-stage counts describe backend attempts, not independent failed frames.
struct ReadStageStats {
    std::size_t attempts = 0, failed_attempts = 0, exhausted_requests = 0;
    u64 first_failed_address = 0;
    std::size_t first_requested_bytes = 0;
};
} // namespace monitor
