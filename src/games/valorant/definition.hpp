#pragma once
#include "domain/game.hpp"
#include <array>

namespace mousebox::games::valorant
{
// Manual camera-feedback aiming owns explicit coefficients and user-confirmed view scope.
// It does not claim that this Reader revision provides native input or life state.
inline constexpr std::array<std::string_view, 3> Capabilities{"radar", "esp", "aim"};
inline constexpr GameDefinition Definition{"valorant", "VALORANT", "available", "", Capabilities, {}};
} // namespace mousebox::games::valorant
