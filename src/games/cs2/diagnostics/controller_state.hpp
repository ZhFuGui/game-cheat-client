#pragma once
#include "games/cs2/control/output_program.hpp"
namespace mousebox::games::cs2
{
void describeController(application::Json &, const ControllerState &, bool sessionEligible, bool diagnostics,
                        bool selected = true);
}
