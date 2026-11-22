#include "weapon_accuracy.hpp"
#include "games/cs2/model/snapshot.hpp"
#include <cmath>

namespace mousebox::games::cs2
{
AccuracyIssue validateWeaponAccuracy(const Snapshot &frame, const WeaponAccuracy &accuracy) noexcept
{
    const auto hasValidFormat = [](const AccuracyScalar &result) {
        return !result.malformed && result.status >= 1 && result.status <= 14 &&
               (result.status == 1 ? result.value && std::isfinite(*result.value) : !result.value);
    };
    if (accuracy.malformed || !hasValidFormat(accuracy.inaccuracy) || !hasValidFormat(accuracy.spread))
        return AccuracyIssue::Malformed;
    if (accuracy.modelId != "cs2.weapon_accuracy.14193.v1")
        return AccuracyIssue::UnverifiedModel;
    if (!frame.frequency || !frame.started || !frame.readFinished || !accuracy.started || !accuracy.readFinished ||
        *frame.started > *accuracy.started || *accuracy.started > *accuracy.readFinished ||
        *accuracy.readFinished > *frame.readFinished)
        return AccuracyIssue::OutsideCapture;

    if (const auto value = accuracy.inaccuracy.value)
    {
        const bool noContributions = !accuracy.movement && !accuracy.airborne && !accuracy.turning && !accuracy.lateral;
        const bool noOwner = accuracy.branch == 1 && !accuracy.owner && *value == 0 && noContributions;
        const bool owned = accuracy.owner == frame.local.pawn;
        const bool normal = accuracy.branch == 4 && accuracy.mode && accuracy.movement && accuracy.airborne &&
                            accuracy.turning && accuracy.lateral && accuracy.penalty;
        const bool early = (accuracy.branch == 2 || (accuracy.branch == 3 && *value == 0)) && noContributions;
        if ((!noOwner && (!owned || (!normal && !early))) || *value > 1)
            return AccuracyIssue::InconsistentInaccuracy;
    }
    return AccuracyIssue::None;
}
} // namespace mousebox::games::cs2
