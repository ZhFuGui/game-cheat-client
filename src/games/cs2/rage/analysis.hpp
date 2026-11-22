#pragma once
#include "application/ports/scene_visibility.hpp"
#include "application/ports/shot_prediction.hpp"
#include "games/cs2/control/aim_types.hpp"
#include "scoring.hpp"
#include <stop_token>
#include <vector>

namespace mousebox::games::cs2::rage
{
struct Candidate
{
    TargetKey target;
    unsigned bone = 0, group = 0;
    Vec3 point;
    double damage = 0;
    std::optional<double> threat;
    std::string model, profile;
    ThreatEvidence evidence;
    double targetHealth = 0;
    std::optional<unsigned> hitbox;
};
struct Analysis
{
    std::vector<Candidate> candidates;
    unsigned queries = 0, unknown = 0;
    std::string block = "analysis-pending";
};
// Each eligible bone supplies one current, verified hitbox-center point and
// damage group; there is no hitbox-surface search or adaptive sampling.
Analysis analyze(const Snapshot &, const application::ports::VisibilityScene *,
                 const application::ports::BallisticsScene &, TargetFilter = {}, std::stop_token = {},
                 const ScorePolicy & = {}, bool coverCheck = true);
inline Analysis analyze(const Snapshot &frame, const application::ports::VisibilityScene &visibility,
                        const application::ports::BallisticsScene &ballistics, TargetFilter filter = {},
                        std::stop_token stop = {}, const ScorePolicy &policy = {}, bool coverCheck = true)
{
    return analyze(frame, &visibility, ballistics, filter, stop, policy, coverCheck);
}
const Bone *currentBone(const Snapshot &, const Candidate &, TargetFilter = {});
std::optional<Vec3> currentAimPoint(const Snapshot &, const Candidate &, TargetFilter = {});
const Candidate *select(const Analysis &, double threatWeight);
struct Selection
{
    const Candidate *candidate = nullptr;
    Score score;
};
Selection selectScored(const Analysis &, const ScorePolicy &, std::optional<TargetKey> previous = {},
                       const Snapshot *current = nullptr, TargetFilter = {});
} // namespace mousebox::games::cs2::rage
