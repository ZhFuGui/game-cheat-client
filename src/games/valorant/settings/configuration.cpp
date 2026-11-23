#include "configuration.hpp"
#include "games/valorant/control/settings.hpp"
#include <cmath>
#include <stdexcept>

namespace mousebox::games::valorant
{
using application::Json;
namespace
{
void require(bool value, const char *message)
{
    if (!value)
        throw std::invalid_argument(message);
}
class Configuration final : public application::ports::GameConfiguration
{
  public:
    std::string_view gameId() const noexcept override
    {
        return "valorant";
    }
    bool permitsAbsentValue(std::string_view key) const noexcept override
    {
        return aim::isSetting(key) || key == "valorantShowSelf" || key == "valorantShowAllies" ||
               key == "valorantShowEnemies" || key == "valorantShowUnknown" || key == "valorantShowCamera" ||
               key == "valorantMetersPerUnit" || key == "valorantWindowExecutable";
    }
    Json defaults() const override
    {
        auto result = Json{{"schemaVersion", 1},
                           {"speedSchema", 2},
                           {"visualsSchema", 1},
                           {"profiles", Json::array()},
                           {"values",
                            {{"host", "127.0.0.1"},
                             {"port", 13302},
                             {"hz", 60.0},
                             {"valorantSkeletonHz", 60.0},
                             {"valorantViewHz", 60.0},
                             {"valorantShowSelf", true},
                             {"valorantShowAllies", true},
                             {"valorantShowEnemies", true},
                             {"valorantShowUnknown", true},
                             {"valorantShowCamera", true},
                             {"valorantMetersPerUnit", 0.0},
                             {"valorantWindowExecutable", "VALORANT-Win64-Shipping.exe"},
                             {"dpi", 1600},
                             {"renderAdapter", "auto"}}},
                           {"espValues",
                            {{"marker", "point"},
                             {"aspect", "viewport"},
                             {"fovAxis", "horizontal"},
                             {"estimatedHeightMeters", 1.0},
                             {"maximumDistanceMeters", 0.0},
                             {"names", true},
                             {"distance", true},
                             {"bones", true},
                             {"tracers", false},
                             {"crosshair", true},
                             {"aimRange", false},
                             {"targetHighlight", false},
                             {"teammates", false},
                             {"opponents", false},
                             {"bots", false},
                             {"self", false}}}};
        aim::addDefaults(result["values"]);
        return result;
    }
    Json normalize(const Json &document) const override
    {
        require(document.is_object(), "VALORANT配置必须为对象");
        require(!document.contains("gameId") || document.at("gameId") == "valorant", "VALORANT配置来源不匹配");
        auto result = defaults();
        if (document.contains("values"))
        {
            require(document.at("values").is_object(), "VALORANT参数格式无效");
            for (auto i = document.at("values").begin(); i != document.at("values").end(); ++i)
            {
                validateValue(i.key(), i.value());
                result["values"][i.key()] = i.value();
            }
        }
        // 2.8.54 had no editable ESP options. Replace its fixed disabled display once.
        if (document.contains("espValues") && document.value("visualsSchema", 0) == 1)
        {
            require(document.at("espValues").is_object(), "VALORANT显示参数格式无效");
            for (auto i = document.at("espValues").begin(); i != document.at("espValues").end(); ++i)
            {
                validateEspValue(i.key(), i.value());
                result["espValues"][i.key()] = i.value();
            }
        }
        require(!document.contains("profiles") || document.at("profiles") == Json::array(), "VALORANT尚无输入校准配置");
        return result;
    }
    void validateValue(std::string_view key, const Json &value) const override
    {
        if (aim::isSetting(key))
            aim::validateSetting(key, value);
        else if (key == "valorantWindowExecutable")
        {
            require(value.is_string(), "本机游戏进程名称必须为文本");
            const auto name = value.get<std::string>();
            require(name.size() > 4 && name.size() <= 240 && name.find_first_of("\\/:*?\"<>|\r\n\t") == name.npos &&
                        name.find('\0') == name.npos && name.substr(name.size() - 4) == ".exe",
                    "请填写进程文件名，例如 VALORANT-Win64-Shipping.exe");
        }
        else if (key == "valorantShowSelf" || key == "valorantShowAllies" || key == "valorantShowEnemies" ||
                 key == "valorantShowUnknown" || key == "valorantShowCamera")
            require(value.is_boolean(), "显示筛选必须为开关");
        else if (key == "valorantMetersPerUnit")
            require(value.is_number() && std::isfinite(value.get<double>()) && value.get<double>() >= 0 &&
                        value.get<double>() <= 1000,
                    "世界单位比例必须在0到1000之间");
        else if (key == "host" || key == "renderAdapter")
        {
            require(value.is_string(), "VALORANT地址或显卡标识必须为文本");
            const auto &text = value.get_ref<const std::string &>();
            require(!text.empty() && text.size() <= 255 && text.find('\0') == std::string::npos &&
                        text.find_first_not_of(" \t\r\n") != std::string::npos,
                    "VALORANT地址或显卡标识无效");
        }
        else if (key == "port" || key == "dpi" || key == "hz" || key == "valorantSkeletonHz" || key == "valorantViewHz")
        {
            require(value.is_number(), "VALORANT参数必须为数值");
            const auto number = value.get<double>();
            const auto low = key == "dpi" ? 100 : 1;
            const auto high = key == "port" ? 65535 : key == "dpi" ? 32000 : 300;
            require(std::isfinite(number) && number >= low && number <= high &&
                        ((key != "port" && key != "dpi") || number == std::floor(number)),
                    "VALORANT参数超出范围");
        }
        else
            throw std::invalid_argument("未知VALORANT设置，不接受其他游戏参数：" + std::string(key));
    }
    void validateEspValue(std::string_view key, const Json &value) const override
    {
        if (key == "names" || key == "distance" || key == "bones" || key == "tracers" || key == "crosshair")
        {
            require(value.is_boolean(), "ESP显示开关必须为布尔值");
            return;
        }
        if (key == "marker")
        {
            require(value == "point" || value == "corners", "仅支持定位点和骨点范围角标");
            return;
        }
        if (key == "aspect")
        {
            require(value == "viewport" || value == "16:9" || value == "16:10" || value == "21:9" || value == "4:3",
                    "视口比例无效");
            return;
        }
        if (key == "fovAxis")
        {
            require(value == "horizontal" || value == "vertical", "FOV轴向无效");
            return;
        }
        if (key == "maximumDistanceMeters")
        {
            require(value.is_number() && std::isfinite(value.get<double>()) && value.get<double>() >= 0 &&
                        value.get<double>() <= 1e9,
                    "距离上限无效");
            return;
        }
        const auto values = defaults().at("espValues");
        require(values.contains(key) && values.at(std::string(key)) == value, "VALORANT未提供此显示功能");
    }
};
} // namespace
const application::ports::GameConfiguration &configuration()
{
    static const Configuration instance;
    return instance;
}
} // namespace mousebox::games::valorant
