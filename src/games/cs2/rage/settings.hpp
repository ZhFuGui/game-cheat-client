#pragma once
#include "scoring.hpp"
#include "application/json.hpp"
#include <string_view>
namespace mousebox::games::cs2::rage
{
void addScoringDefaults(application::Json &);
bool permitsScoringSetting(std::string_view);
bool validateScoringSetting(std::string_view, const application::Json &);
ScorePolicy scorePolicy(const application::Json &);
} // namespace mousebox::games::cs2::rage
