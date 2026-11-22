#include "assets.hpp"
#include "generated/agents_14192.hpp"
#include <algorithm>

namespace mousebox::games::cs2::hitboxes
{
const ModelAsset *findModel(unsigned /*build*/, std::string_view profile, std::string_view name) noexcept
{
    if (profile != generated::Profile)
        return nullptr;
    const auto &models = generated::Models;
    const auto found = std::lower_bound(models.begin(), models.end(), name,
                                        [](const ModelAsset &a, std::string_view b) { return a.name < b; });
    return found != models.end() && found->name == name ? &*found : nullptr;
}
std::span<const ModelAsset> reviewedModels() noexcept
{
    return generated::Models;
}
} // namespace mousebox::games::cs2::hitboxes
