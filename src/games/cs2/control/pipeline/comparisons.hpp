#pragma once
#include "reference.hpp"
#include "geometry.hpp"
namespace mousebox::games::cs2::pipeline
{
void applyComparison(AimSolution &, const FrameContext &, const AimSettings &, const PipelinePolicy &);
} // namespace mousebox::games::cs2::pipeline
