#pragma once
#include <target_capabilities.hpp>

namespace mousebox_target
{
typedef mousebox_generated::Limits Limits;
typedef mousebox_generated::Capabilities Capabilities;
static_assert(Limits::MaxStepCounts == 100 && Limits::MinIntervalMs == 2 && Limits::MaxBudget == 200,
              "The legacy HighRateMotion flag has fixed compatibility semantics");
} // namespace mousebox_target
