#include "analysis.hpp"
#include <algorithm>
namespace mousebox::games::cs2::rage
{
Selection selectScored(const Analysis &a, const ScorePolicy &policy, std::optional<TargetKey> previous,
                       const Snapshot *currentFrame, TargetFilter filter)
{
    std::vector<const Candidate *> candidates;
    candidates.reserve(a.candidates.size());
    double outgoing = 0, incoming = 0;
    for (const auto &c : a.candidates)
    {
        if (policy.model != ScoreModel::Legacy && currentFrame && !currentBone(*currentFrame, c, filter))
            continue;
        candidates.push_back(&c);
        outgoing = std::max(outgoing, c.damage);
        incoming = std::max(incoming, c.evidence.incomingDamage.value_or(0));
    }
    const auto score = [&](const Candidate &c) {
        auto evidence = c.evidence;
        evidence.legacyAim = c.threat;
        return evaluateScore(evidence, c.damage, c.targetHealth, outgoing, incoming, policy);
    };
    if (policy.model == ScoreModel::Legacy)
    {
        const auto c = select(a, policy.threatWeight);
        return c ? Selection{c, score(*c)} : Selection{};
    }
    Selection best, held;
    for (const auto *candidate : candidates)
    {
        const auto &c = *candidate;
        const auto current = score(c);
        if (!best.candidate || current.total > best.score.total ||
            (current.total == best.score.total && c.damage > best.candidate->damage))
            best = {&c, current};
        if (previous == c.target && (!held.candidate || c.damage > held.candidate->damage))
            held = {&c, current};
    }
    if (policy.stabilize && best.candidate && held.candidate && best.candidate->target != held.candidate->target &&
        best.score.total <= held.score.total + policy.switchMargin + 1e-9)
    {
        held.score.stabilized = true;
        return held;
    }
    return best;
}
} // namespace mousebox::games::cs2::rage
