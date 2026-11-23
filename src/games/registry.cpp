#include "registry.hpp"
#include "valorant/definition.hpp"
#include <algorithm>
#include <array>

namespace mousebox::games
{
namespace
{
constexpr std::array<std::string_view, 4> PubgCapabilities{"esp", "aim", "input-calibration", "algorithm-details"};
constexpr std::array<std::string_view, 8> Cs2Capabilities{
    "esp", "radar", "aim", "recoil", "triggerbot", "rage", "input-calibration", "algorithm-details"};
const std::array<GameDefinition, 4> Games{
    GameDefinition{"pubg", "PUBG", "available", "", PubgCapabilities, ""},
    GameDefinition{"cs2", "Counter-Strike 2", "available", "", Cs2Capabilities, "cs2.exe"}, valorant::Definition,
    GameDefinition{"gta5", "Grand Theft Auto V", "unavailable", "game_module_not_connected", {}}};
} // namespace
std::span<const GameDefinition> gameDefinitions() noexcept
{
    return Games;
}
const GameDefinition *findGame(std::string_view id) noexcept
{
    const auto found = std::ranges::find_if(Games, [id](const auto &game) { return game.id == id; });
    return found == Games.end() ? nullptr : &*found;
}
} // namespace mousebox::games
