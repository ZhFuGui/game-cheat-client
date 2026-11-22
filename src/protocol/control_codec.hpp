#pragma once
#include <array>
#include <cstdint>
#include <control_wire.h>

namespace mousebox {
using ControlCommand = mousebox_wire::Command;
using ControlFrame = std::array<std::uint8_t, mousebox_wire::FrameSize>;
ControlFrame encodeCommand(const ControlCommand& command);
bool decodeCommand(const ControlFrame& frame, ControlCommand& command);
}
