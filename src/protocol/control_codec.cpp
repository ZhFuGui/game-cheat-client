#include "protocol/control_codec.hpp"

namespace mousebox {
ControlFrame encodeCommand(const ControlCommand& command) {
    ControlFrame frame{};
    mousebox_wire::encode(command, frame.data());
    return frame;
}
bool decodeCommand(const ControlFrame& frame, ControlCommand& command) {
    return mousebox_wire::decode(frame.data(), command);
}
}
