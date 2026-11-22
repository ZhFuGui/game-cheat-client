#include "map_state.hpp"

namespace mousebox::games::cs2
{
application::Json describeMap(const std::optional<MapIdentity> &map)
{
    using application::Json;
    if (!map)
        return {};
    return {{"status", static_cast<unsigned>(map->status)},
            {"generation", std::to_string(map->generation)},
            {"available", map->available()},
            {"name", map->available() ? Json(map->name) : Json{}},
            {"resourcePath", map->available() ? Json(map->resourcePath) : Json{}}};
}
} // namespace mousebox::games::cs2
