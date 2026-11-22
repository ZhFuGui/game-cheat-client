#include "application_adapter.hpp"
#include "application/settings/mouse_trajectory.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <set>
#include <limits>
namespace mousebox::games::pubg::settings
{
using application::Backend;
using application::Json;
Json defaultValues()
{
    Json values = {{"host", "127.0.0.1"},
                   {"aimEnabled", true},
                   {"port", 13102},
                   {"hz", 30},
                   {"dpi", 1600},
                   {"general", 50},
                   {"aim", 50},
                   {"ads", 60},
                   {"vertical", 1.0},
                   {"inverted", false},
                   {"skew", 800},
                   {"livingLocalSkew", control::DefaultLivingLocalSkewMs},
                   {"mapping", 0},
                   {"mode", 0},
                   {"measurement", 0},
                   {"speed", 2},
                   {"part", 0},
                   {"targetPriority", 0},
                   {"distance", 200.0},
                   {"zeroing", 100},
                   {"gravity", control::DefaultGravityMetersPerSecondSquared},
                   {"trigger", 8},
                   {"renderAdapter", "auto"},
                   {"aimVersion", "v1"},
                   {"mouseMovementAlgorithm", "smooth"}};
    for (const auto &entry : control::WeaponAimCategories)
        values[std::string(entry.setting)] = -1;
    application::addTrajectoryDefaults(values);
    return values;
}
Json defaultEspValues()
{
    return {{"marker", "corners"},
            {"names", true},
            {"distance", true},
            {"bones", false},
            {"tracers", false},
            {"crosshair", true},
            {"aimRange", true},
            {"targetHighlight", true},
            {"teammates", true},
            {"opponents", true},
            {"bots", true},
            {"self", false},
            {"aspect", "16:9"},
            {"fovAxis", "horizontal"},
            {"estimatedHeightMeters", 1.8},
            {"maximumDistanceMeters", 0.0}};
}
namespace
{
void require(bool value, const char *text)
{
    if (!value)
        throw std::invalid_argument(text);
}
void bounded(const Json &value, double lo, double hi, bool integer)
{
    require(value.is_number(), "参数必须为数字");
    const auto n = value.get<double>();
    require(std::isfinite(n) && n >= lo && n <= hi && (!integer || n == std::floor(n)),
            "参数超出允许范围或整数参数含小数");
}
} // namespace
void validateSetting(std::string_view key, const Json &value)
{
    const std::string name(key);
    const auto defaults = defaultValues();
    require(defaults.contains(name), "未知设置");
    if (application::isTrajectorySetting(key))
    {
        application::validateTrajectorySetting(key, value);
        return;
    }
    if (name == "aimVersion")
    {
        require(value == "v1", "未知PUBG瞄准算法版本");
        return;
    }
    if (name == "mouseMovementAlgorithm")
    {
        require(value == "smooth", "未知PUBG鼠标移动算法");
        return;
    }
    if (name == "host")
    {
        require(value.is_string() && !value.get_ref<const std::string &>().empty() &&
                    value.get_ref<const std::string &>().size() <= 253,
                "Reader地址不能为空或过长");
        return;
    }
    if (name == "renderAdapter")
    {
        require(value.is_string() && !value.get_ref<const std::string &>().empty() &&
                    value.get_ref<const std::string &>().size() <= 512,
                "无效显卡");
        return;
    }
    if (name == "inverted" || name == "aimEnabled")
    {
        require(value.is_boolean(), "反向设置必须为布尔值");
        return;
    }
    if (control::weaponAimSetting(key))
        bounded(value, -1, 4, true);
    else if (name == "targetPriority")
        bounded(value, 0, 1, true);
    else if (name == "port")
        bounded(value, 1, 65535, true);
    else if (name == "hz")
        bounded(value, 1, 240, true);
    else if (name == "dpi")
        bounded(value, 100, 32000, true);
    else if (name == "general" || name == "aim" || name == "ads")
        bounded(value, 0, 100, true);
    else if (name == "vertical")
        bounded(value, 0.1, 5, false);
    else if (name == "skew")
        bounded(value, 0, 5000, true);
    else if (name == "livingLocalSkew")
        bounded(value, 0, control::MaximumLivingLocalSkewMs, true);
    else if (name == "mapping")
        bounded(value, 0, 1, true);
    else if (name == "mode")
        bounded(value, -1, 3, true);
    else if (name == "measurement")
        bounded(value, 0, 3, true);
    else if (name == "speed" || name == "part")
        bounded(value, 0, 4, true);
    else if (name == "distance")
        bounded(value, 1, 5000, false);
    else if (name == "gravity")
        bounded(value, 0, 100, false);
    else if (name == "zeroing")
    {
        bounded(value, 100, 500, true);
        require(control::validZeroingDistance(value.get<unsigned>()), "无效归零档位");
    }
    else if (name == "trigger")
    {
        bounded(value, 8, 16, true);
        require(value == 8 || value == 16, "无效侧键");
    }
}
void validateEspSetting(std::string_view key, const Json &value)
{
    const std::string name(key);
    const auto defaults = defaultEspValues();
    require(defaults.contains(name), "未知ESP设置");
    if (name == "marker")
        require(value == "corners" || value == "point" || value == "estimated", "无效标记");
    else if (name == "aspect")
        require(value == "16:9" || value == "16:10" || value == "21:9" || value == "4:3" || value == "viewport",
                "无效宽高比");
    else if (name == "fovAxis")
        require(value == "horizontal" || value == "vertical", "无效FOV轴");
    else if (name == "estimatedHeightMeters")
    {
        bounded(value, 0.001, 100, false);
    }
    else if (name == "maximumDistanceMeters")
    {
        bounded(value, 0, std::numeric_limits<double>::max(), false);
    }
    else
        require(value.is_boolean(), "ESP开关必须为布尔值");
}
Json normalizeSettings(const Json &source)
{
    Json result = {{"schemaVersion", 1},
                   {"values", defaultValues()},
                   {"espValues", defaultEspValues()},
                   {"profiles", Json::array()},
                   {"speedSchema", 2}};
    const auto values = source.value("values", Json::object());
    for (auto i = values.begin(); i != values.end(); ++i)
    {
        try
        {
            if (result["values"].contains(i.key()))
            {
                validateSetting(i.key(), i.value());
                result["values"][i.key()] = i.value();
            }
        }
        catch (const std::exception &)
        {
        }
    }
    if (values.contains("speed") && values["speed"].is_number_integer())
        result["values"]["speed"] = static_cast<int>(
            control::storedSpeedPreset(values["speed"].get<int>(), source.value("speedSchema", 2), true));
    if (result["values"]["mapping"] == 0 && result["values"]["mode"] == -1)
        result["values"]["mode"] = 0;
    const auto esp = source.value("espValues", Json::object());
    for (auto i = esp.begin(); i != esp.end(); ++i)
        try
        {
            validateEspSetting(i.key(), i.value());
            result["espValues"][i.key()] = i.value();
        }
        catch (const std::exception &)
        {
        }
    if (source.contains("profiles") && source["profiles"].is_array())
        result["profiles"] = source["profiles"];
    return result;
}
std::string calibrationSignature(const Json &v)
{
    std::ostringstream out;
    out << "dpi" << v.at("dpi").get<unsigned>() << "-general" << v.at("general").get<int>() << "-aim"
        << v.at("aim").get<int>() << "-ads" << v.at("ads").get<int>() << "-vertical" << std::fixed
        << std::setprecision(2) << v.at("vertical").get<double>();
    if (v.at("inverted").get<bool>())
        out << "-inverted";
    return out.str();
}
std::vector<control::CalibrationRecord> calibrationRecords(const Json &list, const std::string &signature)
{
    std::vector<control::CalibrationRecord> records;
    if (!list.is_array())
        return records;
    for (const auto &item : list)
    {
        if (records.size() >= 64)
            break;
        try
        {
            const auto mode = item.at("mode").get<int>();
            control::Calibration c{true, item.at("profile").get<std::string>(), item.at("yaw").get<double>(),
                                   item.at("pitch").get<double>(), item.at("fov").get<double>()};
            if (mode >= 0 && mode <= 3 && c.profile == signature && control::validCalibration(c))
                records.push_back({static_cast<control::ViewMode>(mode), c});
        }
        catch (const std::exception &)
        {
        }
    }
    return records;
}
Json calibrationRecordsJson(const std::vector<control::CalibrationRecord> &records)
{
    Json list = Json::array();
    for (const auto &r : records)
        list.push_back({{"mode", static_cast<int>(r.mode)},
                        {"profile", r.value.profile},
                        {"yaw", r.value.yawDegreesPerCount},
                        {"pitch", r.value.pitchDegreesPerCount},
                        {"fov", r.value.fov}});
    return list;
}
void applySettings(Backend &b, const Json &v)
{
    b.setAimEnabled(v.value("aimEnabled", true));
    b.setGameInput({v.at("general").get<double>(), v.at("aim").get<double>(), v.at("ads").get<double>(),
                    v.at("vertical").get<double>(), v.at("inverted").get<bool>()});
    b.setInputMapping(static_cast<control::InputMapping>(v.at("mapping").get<int>()));
    const auto mode = v.at("mode").get<int>();
    b.setOutputMode(mode < 0 ? std::nullopt : std::optional(static_cast<control::ViewMode>(mode)));
    b.setSpeed(static_cast<control::SpeedPreset>(v.at("speed").get<int>()));
    b.setTrajectory(application::readTrajectorySettings(v));
    b.setAimPart(static_cast<control::AimPart>(v.at("part").get<int>()));
    control::WeaponAimPolicy weaponAim;
    for (const auto &entry : control::WeaponAimCategories)
    {
        const auto part = v.at(std::string(entry.setting)).get<int>();
        if (part >= 0)
            weaponAim.parts[static_cast<unsigned>(entry.category)] = static_cast<control::AimPart>(part);
    }
    b.setWeaponAimPolicy(weaponAim);
    b.setAimDistance(v.at("distance").get<double>());
    b.setTargetPriority(static_cast<control::TargetPriority>(v.at("targetPriority").get<int>()));
    b.setZeroingDistance(v.at("zeroing").get<unsigned>());
    b.setGravity(v.at("gravity").get<double>());
    b.setTriggerButton(v.at("trigger").get<std::uint16_t>());
    b.setSourceSkew(v.at("skew").get<double>());
    b.setLivingLocalSkew(v.at("livingLocalSkew").get<double>());
}
namespace
{
class PubgConfiguration final : public application::ports::GameConfiguration
{
  public:
    bool permitsAbsentValue(std::string_view key) const noexcept override
    {
        return key == "aimEnabled" || key == "livingLocalSkew" || key == "aimVersion" ||
               key == "mouseMovementAlgorithm" || application::isTrajectorySetting(key) ||
               control::weaponAimSetting(key);
    }
    std::string_view gameId() const noexcept override
    {
        return "pubg";
    }
    Json defaults() const override
    {
        return normalizeSettings(Json::object());
    }
    Json normalize(const Json &source) const override
    {
        return normalizeSettings(source);
    }
    void validateValue(std::string_view key, const Json &value) const override
    {
        validateSetting(key, value);
    }
    void validateEspValue(std::string_view key, const Json &value) const override
    {
        validateEspSetting(key, value);
    }
};
} // namespace
const application::ports::GameConfiguration &configuration()
{
    static const PubgConfiguration pubg;
    return pubg;
}
} // namespace mousebox::games::pubg::settings
