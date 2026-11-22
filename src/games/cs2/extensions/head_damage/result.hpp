#pragma once
#include "inputs.hpp"
#include "application/extensions/contracts.hpp"
namespace mousebox::games::cs2::extensions
{
struct ActorHeadDamage
{
    std::uint32_t controller = 0, pawn = 0;
    std::optional<application::ports::PlayerDamageTarget> target;
    std::optional<std::string> model, profile;
    application::ports::HeadDamagePrediction prediction;
};
struct HeadDamageResult final : application::extensions::Attachment
{
    application::extensions::Scope scope;
    std::uint64_t capture = 0;
    ShotContext shooter;
    std::vector<ActorHeadDamage> actors;
};
bool matchesCurrent(const HeadDamageResult &, const Snapshot &);
} // namespace mousebox::games::cs2::extensions
