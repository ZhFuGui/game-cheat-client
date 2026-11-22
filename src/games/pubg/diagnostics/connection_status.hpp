#pragma once
#include "games/pubg/runtime/runtime_models.hpp"
#include "application/json.hpp"
namespace mousebox::application
{
Json describeReaderConnection(const Snapshot &, std::uint64_t now);
}
