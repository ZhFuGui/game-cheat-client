#include "settings.hpp"
#include "application/settings/mouse_trajectory.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace mousebox::games::valorant::aim
{
using application::Json;
namespace
{
struct Number
{
    const char *key;
    double value, low, high;
    bool integer;
};
constexpr Number Numbers[]{{"valorantAimPart", 0, 0, 3, true},
                           {"valorantTargetPriority", 1, 0, 1, true},
                           {"valorantAimCone", 15, .1, 89, false},
                           {"valorantAimDistanceUnits", 0, 0, 1e9, false},
                           {"valorantAimMaxAgeMs", 150, 10, 1000, true},
                           {"valorantResponseMs", 20, 0, 250, true},
                           {"valorantYawPerCount", 0, -10, 10, false},
                           {"valorantPitchPerCount", 0, -10, 10, false},
                           {"speed", 2, 0, 4, true},
                           {"trigger", 8, 8, 16, true}};
constexpr const char *Flags[]{"aimEnabled", "valorantAimEnemies", "valorantAimTeammates", "valorantCheckForeground"};
const Number *number(std::string_view key)
{
    for (const auto &entry : Numbers)
        if (key == entry.key)
            return &entry;
    return nullptr;
}
} // namespace
void addDefaults(Json &values)
{
    for (const auto &entry : Numbers)
        values[entry.key] = entry.integer ? Json(static_cast<unsigned>(entry.value)) : Json(entry.value);
    values["aimEnabled"] = false;
    values["valorantAimEnemies"] = true;
    values["valorantAimTeammates"] = false;
    values["valorantCheckForeground"] = true;
    values["mouseMovementAlgorithm"] = "smooth";
    application::addTrajectoryDefaults(values);
}
bool isSetting(std::string_view key) noexcept
{
    return number(key) || std::find(std::begin(Flags), std::end(Flags), key) != std::end(Flags) ||
           key == "mouseMovementAlgorithm" || application::isTrajectorySetting(key);
}
void validateSetting(std::string_view key, const Json &value)
{
    if (const auto spec = number(key))
    {
        if (value.is_number())
        {
            const auto n = value.get<double>();
            if (std::isfinite(n) && n >= spec->low && n <= spec->high && (!spec->integer || n == std::floor(n)) &&
                ((key != "valorantYawPerCount" && key != "valorantPitchPerCount") || n == 0 || std::abs(n) >= 1e-7) &&
                (key != "trigger" || n == 8 || n == 16))
                return;
        }
        throw std::invalid_argument("VALORANT瞄准参数超出范围");
    }
    if (std::find(std::begin(Flags), std::end(Flags), key) != std::end(Flags))
    {
        if (!value.is_boolean())
            throw std::invalid_argument("瞄准开关必须为布尔值");
        return;
    }
    if (key == "mouseMovementAlgorithm")
    {
        if (value != "smooth" && value != "direct")
            throw std::invalid_argument("VALORANT仅支持平滑或直接速度控制");
        return;
    }
    if (application::isTrajectorySetting(key))
    {
        application::validateTrajectorySetting(key, value);
        return;
    }
    throw std::invalid_argument("未知VALORANT瞄准设置");
}
Settings readSettings(const Json &v)
{
    Settings out;
    out.enabled = v.at("aimEnabled");
    out.enemies = v.at("valorantAimEnemies");
    out.teammates = v.at("valorantAimTeammates");
    out.checkForeground = v.at("valorantCheckForeground");
    out.part = v.at("valorantAimPart");
    out.priority = v.at("valorantTargetPriority");
    out.maximumAgeMs = v.at("valorantAimMaxAgeMs");
    out.responseMs = v.at("valorantResponseMs");
    out.coneDegrees = v.at("valorantAimCone");
    out.maximumDistance = v.at("valorantAimDistanceUnits");
    out.yawPerCount = v.at("valorantYawPerCount");
    out.pitchPerCount = v.at("valorantPitchPerCount");
    out.speed = static_cast<control::SpeedPreset>(v.at("speed").get<unsigned>());
    out.algorithm = control::movement::parseAlgorithm(v.at("mouseMovementAlgorithm").get<std::string>());
    out.trajectory = application::readTrajectorySettings(v);
    return out;
}
} // namespace mousebox::games::valorant::aim
