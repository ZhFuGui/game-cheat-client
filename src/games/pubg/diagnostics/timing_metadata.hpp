#pragma once
#include "games/pubg/runtime/runtime_models.hpp"
#include "application/json.hpp"

namespace mousebox::application
{
// Time/status/counts only. No entity identity, world coordinates or angles.
Json describeTimingMetadata(const Snapshot &, const control::LiveDiagnostic &, std::uint64_t nowMs);
} // namespace mousebox::application
