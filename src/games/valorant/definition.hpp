#pragma once
#include "domain/game.hpp"
#include <array>

namespace mousebox::games::valorant
{
// Read-only Reader integration. Visual/control capabilities are added only
// after their independently verified application adapters are available.
inline constexpr std::array<std::string_view, 2> Capabilities{"radar", "esp"};
inline constexpr GameDefinition Definition{"valorant", "VALORANT", "available", "", Capabilities, {}};
} // namespace mousebox::games::valorant
