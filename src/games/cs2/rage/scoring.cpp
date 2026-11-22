#include "scoring.hpp"
#include <algorithm>
#include <cmath>

namespace mousebox::games::cs2::rage
{
std::string_view scoreModelId(ScoreModel model) noexcept
{
    return model == ScoreModel::Legacy ? "legacy" : "threat-v2";
}
double damageScore(double damage, double health) noexcept
{
    if (!std::isfinite(damage) || !std::isfinite(health) || damage < 0 || health <= 0)
        return 0;
    // Algebraically D/(D+H), without overflowing D+H.
    return damage == 0 ? 0 : 100 / (1 + health / damage);
}
Score evaluateScore(const ThreatEvidence &e, double damage, double health, double maximumOutgoing,
                    double maximumIncoming, const ScorePolicy &p)
{
    Score out;
    const auto relative = [](double d, double maximum) { return maximum > 0 ? 100 * (d / maximum) : 0.; };
    if (p.model == ScoreModel::Legacy)
    {
        out.aim = e.legacyAim ? std::optional(*e.legacyAim * 100) : std::nullopt;
        out.threat = out.aim.value_or(0);
        out.outgoingDamageScore = relative(damage, maximumOutgoing);
        out.total = p.threatWeight * *out.threat + (1 - p.threatWeight) * out.outgoingDamageScore;
        out.degraded = !e.legacyAim;
        return out;
    }
    out.aim = e.aim ? std::optional(std::clamp(*e.aim * 100, 0., 100.)) : std::nullopt;
    if (e.incomingDamage && (!p.healthNormalization || (e.localHealth && *e.localHealth > 0)))
        out.incomingDamageScore = p.healthNormalization ? damageScore(*e.incomingDamage, *e.localHealth)
                                                        : relative(*e.incomingDamage, maximumIncoming);
    out.outgoingDamageScore = p.healthNormalization ? damageScore(damage, health) : relative(damage, maximumOutgoing);
    // Missing or sentinel ammunition is unknown, not an empty magazine.
    if (e.clip && *e.clip >= 0 && *e.clip <= 1)
        out.lowAmmoDeduction = p.lowAmmoPenalty;
    if (e.reloading == true)
        out.reloadDeduction = p.reloadPenalty;
    out.degraded = !out.aim || !out.incomingDamageScore || !e.clip || *e.clip < 0 || !e.reloading;
    double sum = 0, weights = 0;
    if (out.aim && p.aimWeight > 0)
    {
        sum += p.aimWeight * *out.aim;
        weights += p.aimWeight;
    }
    if (out.incomingDamageScore && p.aimWeight < 1)
    {
        sum += (1 - p.aimWeight) * *out.incomingDamageScore;
        weights += 1 - p.aimWeight;
    }
    if (e.noDamage)
        out.threat = 0;
    else if (weights > 0)
        out.threat = std::clamp(sum / weights - out.lowAmmoDeduction - out.reloadDeduction, 0., 100.);
    // With no positive threat evidence, explicitly degrade to outgoing damage.
    out.total = out.threat ? p.threatWeight * *out.threat + (1 - p.threatWeight) * out.outgoingDamageScore
                           : out.outgoingDamageScore;
    return out;
}
} // namespace mousebox::games::cs2::rage
