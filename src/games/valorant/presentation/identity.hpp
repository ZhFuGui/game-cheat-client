#pragma once
#include "adapters/sources/valorant_reader/model.hpp"

namespace mousebox::games::valorant
{
inline unsigned relationship(std::uint64_t id, const sources::valorant_reader::PositionContext &context,
                             const sources::valorant_reader::Position *position)
{
    if (context.localState == 3 && context.localEntity == id)
        return 1;
    if (!position || !position->bombTeam || !context.referenceBombTeam || *position->bombTeam == 0xfe ||
        *context.referenceBombTeam == 0xfe)
        return 0;
    return position->relationship == 2 || position->relationship == 3 ? position->relationship : 0;
}
} // namespace mousebox::games::valorant
