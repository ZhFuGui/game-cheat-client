#include "configuration.hpp"
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
    Json defaults() const override
    {
        return {{"schemaVersion", 1},
                {"speedSchema", 2},
                {"profiles", Json::array()},
                {"values",
                 {{"host", "127.0.0.1"},
                  {"port", 13302},
                  {"hz", 60.0},
                  {"valorantSkeletonHz", 60.0},
                  {"valorantViewHz", 60.0},
                  {"dpi", 1600},
                  {"renderAdapter", "auto"}}},
                {"espValues",
                 {{"marker", "point"},
                  {"aspect", "viewport"},
                  {"fovAxis", "horizontal"},
                  {"estimatedHeightMeters", 1.0},
                  {"maximumDistanceMeters", 0.0},
                  {"names", false},
                  {"distance", false},
                  {"bones", false},
                  {"tracers", false},
                  {"crosshair", false},
                  {"aimRange", false},
                  {"targetHighlight", false},
                  {"teammates", false},
                  {"opponents", false},
                  {"bots", false},
                  {"self", false}}}};
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
        if (document.contains("espValues"))
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
        if (key == "host" || key == "renderAdapter")
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
        const auto values = defaults().at("espValues");
        require(values.contains(key) && values.at(std::string(key)) == value, "VALORANT当前没有已开放的ESP显示参数");
    }
};
} // namespace
const application::ports::GameConfiguration &configuration()
{
    static const Configuration instance;
    return instance;
}
} // namespace mousebox::games::valorant
