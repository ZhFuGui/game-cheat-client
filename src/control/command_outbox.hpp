#pragma once
#include "protocol/control_codec.hpp"
#include <deque>
#include <optional>

namespace mousebox
{
// Caller provides synchronization. STOP overtakes queued movement, never a frame already writing.
class CommandOutbox
{
    std::deque<ControlCommand> controls_;
    std::optional<ControlCommand> motion_;
    std::uint32_t activeSession_ = 0;
    std::uint32_t lastSequence_ = 0;

  public:
    void begin(const ControlCommand &command);
    bool refresh(const ControlCommand &command);
    bool tryOneShot(const ControlCommand &start, const ControlCommand &motion);
    bool tryButtonPulse(const ControlCommand &);
    void stop(std::uint32_t session);
    void clear();
    bool pop(ControlCommand &command);
    // Batch only an adjacent START/MOTION pair; STOP and other control ordering stays intact.
    std::size_t popBatch(std::array<ControlCommand, 2> &commands);
    bool empty() const
    {
        return controls_.empty() && !motion_;
    }
};
} // namespace mousebox
