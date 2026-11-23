#pragma once
#include "application/json.hpp"
#include "games/valorant/control/output_program.hpp"

namespace mousebox::games::valorant
{
void describeAim(application::Json &state, const aim::Status &);
}
