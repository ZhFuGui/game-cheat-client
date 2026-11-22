#pragma once
#include "domain/game.hpp"

namespace mousebox::games
{
std::span<const GameDefinition> gameDefinitions() noexcept;
const GameDefinition *findGame(std::string_view id) noexcept;
} // namespace mousebox::games
