#pragma once
#include "field_values.hpp"
#include <memory>

namespace mousebox::games::cs2
{
struct Snapshot;
}
namespace mousebox::sources::cs2_reader::detail
{
std::shared_ptr<const games::cs2::Snapshot> decodeSnapshot(const Fields &, std::string_view wire);
} // namespace mousebox::sources::cs2_reader::detail
