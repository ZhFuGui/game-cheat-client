#pragma once
#include "model/primitives.hpp"
namespace monitor {
struct Session {
    u32 pid = 0;
    u64 process_sequence = 0, cr3 = 0, peb = 0, base = 0, generation = 0;
    u32 image_size = 0, timestamp = 0;
};
bool SameSession(const Session&, const Session&) noexcept;
} // namespace monitor
