#pragma once
#include "inputs.hpp"
#include "application/extensions/contracts.hpp"
#include "application/ports/shot_prediction.hpp"
namespace mousebox::games::cs2::extensions
{
struct ActorDamage
{
    std::uint32_t controller = 0, pawn = 0;
    application::ports::ShotPrediction prediction;
};
struct RemainingDamageResult final : application::extensions::Attachment
{
    application::extensions::Scope scope;
    std::uint64_t capture = 0;
    ShotContext shooter;
    std::vector<ActorDamage> actors;
};
bool matchesCurrent(const RemainingDamageResult &, const Snapshot &);
} // namespace mousebox::games::cs2::extensions
