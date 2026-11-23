#include "settings.hpp"

namespace mousebox::games::valorant
{
VisualSettings visualSettings(const application::Json &values)
{
    return {values.at("valorantShowSelf").get<bool>(),
            values.at("valorantShowAllies").get<bool>(),
            values.at("valorantShowEnemies").get<bool>(),
            values.at("valorantShowUnknown").get<bool>(),
            values.at("valorantShowCamera").get<bool>(),
            values.at("valorantMetersPerUnit").get<double>(),
            values.at("valorantWindowExecutable").get<std::string>()};
}
bool included(unsigned relation, const VisualSettings &s)
{
    return relation == 1 ? s.self : relation == 2 ? s.allies : relation == 3 ? s.enemies : s.unknown;
}
} // namespace mousebox::games::valorant
