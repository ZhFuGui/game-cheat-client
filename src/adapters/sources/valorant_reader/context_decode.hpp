#pragma once
#include "model.hpp"
#include "wire.hpp"
namespace mousebox::sources::valorant_reader
{
std::optional<unsigned> bombTeam(const wire::Fields &, unsigned field);
PositionContext decodeContext(std::string_view);
} // namespace mousebox::sources::valorant_reader
