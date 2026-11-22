#pragma once
#include "application/json.hpp"
#include <cstdint>

namespace mousebox::games::cs2
{
struct Snapshot;
application::Json readRadarFrame(const Snapshot *, std::uint64_t revision, std::uint64_t connection,
                                 const application::Json &args);
}
