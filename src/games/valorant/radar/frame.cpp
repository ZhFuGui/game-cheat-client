#include "frame.hpp"
#include <stdexcept>

namespace mousebox::games::valorant
{
using application::Json;
Json readRadar(const sources::valorant_reader::Observation &observation, const Json &args)
{
    const auto revision = std::to_string(observation.revision);
    if (args.contains("after") && args.at("after") == revision)
        return {{"unchanged", true}, {"frame", nullptr}};
    const auto &positionChannel = observation.channels[0];
    if (!positionChannel.data)
        return {{"unchanged", false}, {"frame", nullptr}};
    const auto &data = *positionChannel.data;
    const auto &context = data.context;
    if (data.positions.size() > 4096)
        throw std::runtime_error("当前人物数超过雷达展示预算，完整接收结果仍保留");
    Json players = Json::array();
    for (const auto &[id, position] : data.positions)
    {
        const char *relation = "unknown";
        if (context.localState == 3 && context.localEntity == id)
            relation = "self";
        else if (position.bombTeam && context.referenceBombTeam && *position.bombTeam != 0xfe &&
                 *context.referenceBombTeam != 0xfe)
        {
            if (position.relationship == 2)
                relation = "ally";
            else if (position.relationship == 3)
                relation = "opponent";
        }
        players.push_back(
            {{"id", std::to_string(id)},
             {"position",
              position.world ? Json::array({position.world->x, position.world->y, position.world->z}) : Json{}},
             {"team", position.bombTeam ? Json{{"scheme", 1}, {"key", std::to_string(*position.bombTeam)}} : Json{}},
             {"relation", relation}});
    }
    Json camera = nullptr;
    const auto &view = observation.channels[2];
    if (view.data && view.instance == positionChannel.instance && view.data->capture.epoch == data.capture.epoch &&
        view.data->camera && view.data->camera->position)
    {
        const auto &value = *view.data->camera->position;
        camera = Json::array({value.x, value.y, value.z});
    }
    constexpr const char *states[]{"unknown", "absent", "unresolved", "resolved"};
    return {
        {"unchanged", false},
        {"frame",
         {{"revision", revision},
          {"scope", positionChannel.instance + ":" + std::to_string(data.capture.epoch)},
          {"map", context.map ? Json{{"packagePath", context.map->packagePath}, {"worldName", context.map->worldName}}
                              : Json{}},
          {"local",
           {{"state", states[context.localState]},
            {"entityId", context.localEntity ? Json(std::to_string(*context.localEntity)) : Json{}}}},
          {"players", std::move(players)},
          {"camera", std::move(camera)}}}};
}
} // namespace mousebox::games::valorant
