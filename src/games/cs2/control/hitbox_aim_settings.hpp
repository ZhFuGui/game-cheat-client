#pragma once
#include "application/json.hpp"
#include "games/cs2/hitboxes/volume.hpp"
#include "games/cs2/settings/hitbox_inset.hpp"
#include <string_view>

namespace mousebox::games::cs2
{
struct HitboxAimSettings
{
    double insetPercent = DefaultHitboxInsetPercent;
    unsigned intervalMs = 150;
    hitboxes::PointDistribution distribution = hitboxes::PointDistribution::Uniform;
    bool operator==(const HitboxAimSettings &) const = default;
};

hitboxes::PointDistribution parseAimPointDistribution(std::string_view);
HitboxAimSettings readHitboxAimSettings(const application::Json &);
} // namespace mousebox::games::cs2
