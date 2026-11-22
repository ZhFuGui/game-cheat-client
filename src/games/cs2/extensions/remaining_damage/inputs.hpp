#pragma once
#include "games/cs2/model/snapshot.hpp"
namespace mousebox::games::cs2::extensions
{
struct ShotContext
{
    std::uint32_t controller = 0, pawn = 0, weapon = 0;
    unsigned definition = 0, team = 0;
    Vec3 origin;
};
std::optional<ShotContext> shotContext(const Snapshot &);
std::optional<Vec3> headEndpoint(const Snapshot &, const Body &);
bool livingEnemy(const Player &, const ShotContext &);
} // namespace mousebox::games::cs2::extensions
