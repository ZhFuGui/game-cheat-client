#pragma once
#include "games/cs2/extensions/head_damage/inputs.hpp"
#include "games/cs2/damage_search/search.hpp"
#include "application/extensions/contracts.hpp"
namespace mousebox::games::cs2::extensions
{
struct ActorBestDamage
{
    std::uint32_t controller = 0, pawn = 0;
    std::optional<application::ports::PlayerDamageTarget> target;
    std::optional<std::string> model, profile;
    damage_search::Result search;
};
struct BestDamageResult final : application::extensions::Attachment
{
    application::extensions::Scope scope;
    std::uint64_t capture = 0;
    ShotContext shooter;
    std::vector<ActorBestDamage> actors;
};
bool matchesCurrent(const BestDamageResult &, const Snapshot &);
} // namespace mousebox::games::cs2::extensions
