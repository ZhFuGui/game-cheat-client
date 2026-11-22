#pragma once
#include "scoring.hpp"
#include "application/ports/shot_prediction.hpp"
#include "games/cs2/hitboxes/frame.hpp"
#include "games/cs2/hitboxes/intersection.hpp"
#include <stop_token>
namespace mousebox::games::cs2::rage
{
class ThreatEvaluator
{
    const Snapshot &frame_;
    const application::ports::BallisticsScene &scene_;
    const Player *local_ = nullptr;
    hitboxes::Pose pose_;
    hitboxes::TargetIntersector body_;
    std::optional<application::ports::PlayerDamageTarget> damageTarget_;

  public:
    ThreatEvaluator(const Snapshot &, const application::ports::BallisticsScene &);
    ThreatEvidence evaluate(const Player &, std::stop_token, unsigned &queries) const;
};
} // namespace mousebox::games::cs2::rage
