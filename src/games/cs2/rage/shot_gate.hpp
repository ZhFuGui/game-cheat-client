#pragma once
#include "analysis.hpp"
#include "games/cs2/trigger/detection.hpp"

namespace mousebox::games::cs2::rage
{
struct ShotEvaluation
{
    std::string block = "shot-pending";
    trigger::Detection hit;
    std::optional<double> damage;
};
// Recheck one already-selected point against one immutable current snapshot.
// Scenes must be loaded by the analysis worker; this function performs no I/O,
// target ranking, threat evaluation, or all-bone damage traversal.
ShotEvaluation evaluateShot(const Snapshot &, const Candidate &, TargetFilter, bool coverCheck, bool otherPlayerCheck,
                            double inset, const application::ports::VisibilityScene *,
                            const application::ports::BallisticsScene *);
} // namespace mousebox::games::cs2::rage
