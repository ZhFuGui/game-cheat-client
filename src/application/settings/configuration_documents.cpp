#include "configuration_documents.hpp"
#include "application_values.hpp"
#include "game_configurations.hpp"
#include <stdexcept>

namespace mousebox::application
{
namespace
{
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
} // namespace

Json splitApplicationDocument(const Json &normalized)
{
    Json values = Json::object();
    for (auto i = normalized.at("values").begin(); i != normalized.at("values").end(); ++i)
        if (isApplicationValue(i.key()))
            values[i.key()] = i.value();
    return {{"schemaVersion", 1}, {"values", std::move(values)}};
}
Json splitGameDocument(const Json &normalized, const std::string &gameId, const Json &binding)
{
    auto result = normalized;
    result["schemaVersion"] = 1;
    result["gameId"] = gameId;
    result["mouseMotion"] = binding;
    for (auto i = result["values"].begin(); i != result["values"].end();)
        if (isApplicationValue(i.key()))
            i = result["values"].erase(i);
        else
            ++i;
    return result;
}
void validateGameDocument(const Json &document)
{
    require(document.is_object() && document.value("schemaVersion", 0) == 1 && document.at("values").is_object() &&
                document.at("espValues").is_object() && document.at("profiles").is_array(),
            "工作区配置结构或版本无效");
    const auto &configuration = gameConfiguration(document.at("gameId").get<std::string>());
    const auto schemaDefaults = configuration.defaults();
    require(document.value("speedSchema", 0) == schemaDefaults.value("speedSchema", 0), "工作区速度配置版本尚不支持");
    const auto defaults = schemaDefaults.at("values");
    for (auto i = defaults.begin(); i != defaults.end(); ++i)
        if (!isApplicationValue(i.key()) && !configuration.permitsAbsentValue(i.key()))
            require(document.at("values").contains(i.key()), "工作区缺少必需参数，不会静默恢复默认值");
    const auto esp = schemaDefaults.at("espValues");
    for (auto i = esp.begin(); i != esp.end(); ++i)
        require(document.at("espValues").contains(i.key()), "工作区缺少ESP参数");
    for (auto i = document.at("values").begin(); i != document.at("values").end(); ++i)
    {
        require(!isApplicationValue(i.key()), "工作区文档不能覆盖全局硬件设置");
        configuration.validateStoredValue(i.key(), i.value());
    }
    for (auto i = document.at("espValues").begin(); i != document.at("espValues").end(); ++i)
        configuration.validateEspValue(i.key(), i.value());
    const auto binding = document.value("mouseMotion", Json::object());
    require(binding.is_object(), "工作区模型绑定必须为对象");
    if (binding.contains("active"))
        require(binding.at("active").is_boolean(), "工作区模型启用状态无效");
    if (binding.contains("variation"))
        require(binding.at("variation").is_boolean(), "工作区模型变化状态无效");
    if (binding.contains("modelPath"))
        require(binding.at("modelPath").is_string() &&
                    binding.at("modelPath").get_ref<const std::string &>().size() <= 32768 &&
                    binding.at("modelPath").get_ref<const std::string &>().find('\0') == std::string::npos,
                "工作区模型路径无效");
    if (binding.contains("captureDpi"))
        require(binding.at("captureDpi").is_number_integer() && binding.at("captureDpi").get<int>() >= 100 &&
                    binding.at("captureDpi").get<int>() <= 32000,
                "工作区采集DPI无效");
}
void validateApplicationDocument(const Json &document)
{
    require(document.value("schemaVersion", 0) == 1 && document.at("values").is_object(), "全局配置无效");
    for (auto i = document.at("values").begin(); i != document.at("values").end(); ++i)
    {
        require(isApplicationValue(i.key()), "全局配置包含非全局参数");
        validateApplicationValue(i.key(), i.value());
    }
    require(document.at("values").contains("dpi") && document.at("values").contains("renderAdapter"),
            "全局配置缺少必需参数");
}
} // namespace mousebox::application
