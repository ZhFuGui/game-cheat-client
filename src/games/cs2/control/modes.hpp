#pragma once
#include "policy.hpp"
#include <span>
#include <string_view>
namespace mousebox::games::cs2
{
struct ModeDefinition
{
    std::string_view id, factor;
    ModeSelection selection;
};
std::span<const ModeDefinition> algorithmModes();
const ModeDefinition &algorithmMode(ModeSelection);
ModeSelection parseAlgorithmMode(std::string_view);
PipelinePolicy pipelinePolicy(ModeSelection);
} // namespace mousebox::games::cs2
