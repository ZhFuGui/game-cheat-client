#pragma once
#include <optional>
namespace mousebox::render
{
// Shared local clock for renderer entry and completion observation. Failure only
// withdraws telemetry; it must not stop drawing or change control eligibility.
std::optional<double> frameClockMilliseconds() noexcept;
} // namespace mousebox::render
