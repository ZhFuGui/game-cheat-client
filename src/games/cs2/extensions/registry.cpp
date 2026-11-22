#include "registry.hpp"
#include "cover/extension.hpp"
#include "remaining_damage/extension.hpp"
#include "head_damage/extension.hpp"
#include "best_damage/extension.hpp"
#include <stdexcept>

namespace mousebox::games::cs2::extensions
{
void addSettingDefaults(application::Json &values)
{
    values[coverDefinition().enabledSetting] = false;
    values[remainingDamageDefinition().enabledSetting] = false;
    values[headDamageDefinition().enabledSetting] = false;
    values[bestDamageDefinition().enabledSetting] = false;
}
bool permitsSetting(std::string_view key) noexcept
{
    return key == coverDefinition().enabledSetting || key == remainingDamageDefinition().enabledSetting ||
           key == headDamageDefinition().enabledSetting || key == bestDamageDefinition().enabledSetting;
}
bool validateSetting(std::string_view key, const application::Json &value)
{
    if (!permitsSetting(key))
        return false;
    if (!value.is_boolean())
        throw std::invalid_argument("扩展开关须为布尔值");
    return true;
}
std::vector<std::unique_ptr<application::extensions::Extension<Snapshot>>> create(
    std::shared_ptr<application::ports::VisibilityProvider> provider,
    std::shared_ptr<application::ports::BallisticsProvider> ballistics)
{
    std::vector<std::unique_ptr<application::extensions::Extension<Snapshot>>> result;
    result.push_back(std::make_unique<CoverExtension>(std::move(provider)));
    if (ballistics)
    {
        result.push_back(std::make_unique<RemainingDamageExtension>(ballistics));
        result.push_back(std::make_unique<HeadDamageExtension>(ballistics));
        result.push_back(std::make_unique<BestDamageExtension>(std::move(ballistics)));
    }
    return result;
}
} // namespace mousebox::games::cs2::extensions
