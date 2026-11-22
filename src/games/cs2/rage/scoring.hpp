#pragma once
#include <optional>
#include <string_view>

namespace mousebox::games::cs2::rage
{
enum class ScoreModel
{
    Legacy,
    ThreatV2
};
struct ScorePolicy
{
    ScoreModel model = ScoreModel::ThreatV2;
    double threatWeight = .7, aimWeight = .6, lowAmmoPenalty = 10, reloadPenalty = 60;
    bool healthNormalization = true, stabilize = true;
    double switchMargin = 1;
    bool operator==(const ScorePolicy &) const = default;
};
struct ThreatEvidence
{
    std::optional<double> legacyAim, aim, incomingDamage, localHealth;
    std::optional<unsigned> weapon;
    std::optional<int> clip;
    std::optional<bool> reloading;
    bool noDamage = false, assumedFriendlyFire = false;
};
struct Score
{
    std::optional<double> aim, incomingDamageScore, threat;
    double outgoingDamageScore = 0, lowAmmoDeduction = 0, reloadDeduction = 0, total = 0;
    bool degraded = false, stabilized = false;
};
std::string_view scoreModelId(ScoreModel) noexcept;
double damageScore(double damage, double health) noexcept;
Score evaluateScore(const ThreatEvidence &, double damage, double targetHealth, double maximumOutgoing,
                    double maximumIncoming, const ScorePolicy &);
} // namespace mousebox::games::cs2::rage
