#pragma once
#include "games/cs2/model/snapshot.hpp"

namespace mousebox::games::cs2
{
// Read-only results at the sample's native game times, never output commands.
struct RecoilEvaluation
{
    bool supportedBuild = false;
    bool directionContextIndependent = false;
    std::optional<Angles> shotPredictable, shotUnpredictable, shotOffset, rawEyePlusRecoil;
    std::optional<Angles> contextDirection;
    std::optional<Angles> crosshairFollow, cameraPredictable, cameraUnpredictable, cameraAim, cameraView,
        cameraFeedback;
};
std::optional<Angles> evaluateComponent(Angles base, Angles velocity, double elapsedSeconds, bool scaleTwice = true);
RecoilEvaluation evaluateRecoil(const Snapshot &);
// Explicit prediction: advance only the sampled game state, assuming no new impulses.
std::optional<Angles> evaluateShotOffset(const AimPunchState &, double secondsAhead);
} // namespace mousebox::games::cs2
