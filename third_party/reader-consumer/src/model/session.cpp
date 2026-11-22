#include "model/session.hpp"
namespace monitor {
bool SameSession(const Session& a, const Session& b) noexcept {
    // generation is the monitor's re-attach counter; it is deliberately NOT identity.
    return a.pid && a.base && a.pid == b.pid && a.process_sequence == b.process_sequence && a.cr3 == b.cr3 &&
           a.peb == b.peb && a.base == b.base && a.image_size == b.image_size && a.timestamp == b.timestamp;
}

} // namespace monitor
