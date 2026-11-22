#include "radar_frame.hpp"
#include "games/cs2/diagnostics/map_state.hpp"
#include "games/cs2/model/snapshot.hpp"
#include <stdexcept>
#include <unordered_map>

namespace mousebox::games::cs2
{
namespace
{
using application::Json;

template <class T> Json nullable(const std::optional<T> &value)
{
    return value ? Json(*value) : Json{};
}

std::optional<bool> playerAlive(const Player &player, const Entity *body)
{
    if (player.alive == false || (body && ((body->health && *body->health <= 0) ||
                                           (body->lifeState && *body->lifeState != 0))))
        return false;
    if (player.alive == true || (body && body->health && *body->health > 0))
        return true;
    return {};
}
} // namespace

application::Json readRadarFrame(const Snapshot *snapshot, std::uint64_t captureRevision, std::uint64_t connection,
                                 const application::Json &args)
{
    if (!args.is_object() || (!args.empty() && (args.size() != 1 || !args.contains("after"))))
        throw std::invalid_argument("Radar 只接受取样标识");
    const auto after = args.value("after", std::string{});
    if (args.contains("after") && (after.empty() || after.size() > 20 ||
                                   (after.size() > 1 && after.front() == '0') ||
                                   after.find_first_not_of("0123456789") != std::string::npos))
        throw std::invalid_argument("Radar 取样标识无效");
    if (!snapshot)
        return {{"frame", nullptr}, {"unchanged", false}};
    const auto revision = std::to_string(captureRevision);
    if (after == revision)
        return {{"frame", nullptr}, {"unchanged", true}};

    const auto &frame = *snapshot;
    std::unordered_map<std::uint32_t, const Weapon *> weapons;
    for (const auto &weapon : frame.weapons)
        weapons.emplace(weapon.entity.id, &weapon);
    auto players = Json::array();
    for (const auto &player : frame.players)
    {
        const auto *pawn = player.body ? &*player.body : nullptr;
        const auto *body = pawn ? &pawn->entity : nullptr;
        const auto team = player.team ? player.team : body ? body->team : std::nullopt;
        if (team != 2 && team != 3)
            continue;
        const bool local = frame.local.controller == player.controller;
        const auto alive = playerAlive(player, body);
        const bool placed = body && body->position && body->dormant != true && alive == true;
        const auto *position = placed ? &*body->position : nullptr;
        Json yaw;
        if (placed)
        {
            const bool localPawn = local && frame.local.pawn == body->id;
            if (localPawn && frame.local.angles)
                yaw = frame.local.angles->y;
            else if (pawn->networkEyeAngles)
                yaw = pawn->networkEyeAngles->y;
        }
        Json activeWeapon;
        if (pawn && pawn->activeWeapon)
        {
            const auto active = weapons.find(*pawn->activeWeapon);
            if (active != weapons.end())
            {
                const auto &weapon = *active->second;
                activeWeapon = {{"name", weapon.entity.name}, {"clip", nullable(weapon.clip)},
                                {"reserve", nullable(weapon.reserve)}};
            }
        }
        players.push_back({{"controller", player.controller},
                           {"name", player.name.value_or(body ? body->name : std::string{})},
                           {"team", *team},
                           {"local", local && (!frame.local.pawn || (body && frame.local.pawn == body->id))},
                           {"alive", nullable(alive)},
                           {"position", position ? Json::array({position->x, position->y, position->z}) : Json{}},
                           {"yaw", std::move(yaw)},
                           {"health", body ? nullable(body->health) : Json{}},
                           {"maxHealth", body ? nullable(body->maxHealth) : Json{}},
                           {"armor", pawn ? nullable(pawn->armor) : Json{}},
                           {"weapon", std::move(activeWeapon)}});
    }
    const auto scope = std::to_string(connection) + ":" + std::to_string(frame.epoch) + ":" +
                       std::to_string(frame.processId);
    return {{"unchanged", false},
            {"frame", {{"revision", revision}, {"scope", scope}, {"map", describeMap(frame.map)},
                       {"players", std::move(players)}}}};
}
} // namespace mousebox::games::cs2
