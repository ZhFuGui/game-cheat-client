#pragma once
#include "frame_context.hpp"
namespace mousebox::games::cs2::pipeline
{
struct Reference
{
    std::optional<Angles> direction, recoil;
    bool inputControllable = false;
    std::string kind = "native-basis", block = "ready";
    double recoilHorizonMs = 0;
};
Reference shotReference(const Snapshot &, const Body &, const AimSettings &, double horizonMs, const PipelinePolicy &);
struct ResponseInput
{
    Angles angles;
    const char *kind;
};
ResponseInput responseInput(const Snapshot &, const PipelinePolicy &);
} // namespace mousebox::games::cs2::pipeline
