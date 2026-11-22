#pragma once
#include "application/json.hpp"
#include <functional>
#include <string>
#include <vector>
namespace mousebox::application::ports
{
// Immutable accepted source references only. Read runs on the diagnostic worker,
// never on the source, control, message-window or render thread.
struct SourcePacket
{
    std::string id, title, identity, revision, format;
    std::function<Json()> read;
};
using SourcePackets = std::vector<SourcePacket>;
} // namespace mousebox::application::ports
