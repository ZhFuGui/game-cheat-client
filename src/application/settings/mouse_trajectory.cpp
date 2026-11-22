#include "mouse_trajectory.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
namespace mousebox::application
{
namespace
{
constexpr std::array<std::string_view, 4> Handles{"mouseBezierControl1X", "mouseBezierControl1Y",
                                                  "mouseBezierControl2X", "mouseBezierControl2Y"};
}
bool isTrajectorySetting(std::string_view key) noexcept
{
    return key == "mouseTrajectory" || std::find(Handles.begin(), Handles.end(), key) != Handles.end();
}
void addTrajectoryDefaults(Json &v)
{
    v["mouseTrajectory"] = "linear";
    const double defaults[]{25, 20, 75, 20};
    for (unsigned i = 0; i < Handles.size(); ++i)
        v[std::string(Handles[i])] = defaults[i];
}
void validateTrajectorySetting(std::string_view key, const Json &v)
{
    if (key == "mouseTrajectory")
    {
        if (v != "linear" && v != "bezier")
            throw std::invalid_argument("未知移动轨迹");
        return;
    }
    const auto found = std::find(Handles.begin(), Handles.end(), key);
    if (found == Handles.end())
        throw std::invalid_argument("未知轨迹设置");
    const auto index = static_cast<unsigned>(found - Handles.begin());
    const double lower = index % 2 ? -100 : 0;
    if (!v.is_number() || !std::isfinite(v.get<double>()) || v.get<double>() < lower || v.get<double>() > 100)
        throw std::invalid_argument("控制点纵向须为0至100%，横向须为-100至100%");
}
control::movement::TrajectorySettings readTrajectorySettings(const Json &v)
{
    Json values;
    addTrajectoryDefaults(values);
    for (auto it = values.begin(); it != values.end(); ++it)
        if (v.contains(it.key()))
        {
            validateTrajectorySetting(it.key(), v.at(it.key()));
            it.value() = v.at(it.key());
        }
    using namespace control::movement;
    return {
        values.at("mouseTrajectory") == "bezier" ? TrajectoryKind::Bezier : TrajectoryKind::Linear,
        values.at("mouseBezierControl1X").get<double>() / 100, values.at("mouseBezierControl1Y").get<double>() / 100,
        values.at("mouseBezierControl2X").get<double>() / 100, values.at("mouseBezierControl2Y").get<double>() / 100};
}
} // namespace mousebox::application
