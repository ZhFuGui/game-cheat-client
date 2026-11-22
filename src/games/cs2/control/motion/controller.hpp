#pragma once
#include "types.hpp"
#include "control/movement/controller.hpp"
namespace mousebox::games::cs2::motion
{
// The game boundary adapts angles and verified recoil provenance only.
// Strategy selection, state and rollback are owned by the common movement layer.
class Controller
{
    control::movement::Controller engine_;

  public:
    void reset()
    {
        engine_.reset();
    }
    Result step(control::movement::Algorithm algorithm, const Input &in, control::HumanMotionRuntime &human,
                MotionFactors factors = {})
    {
        return engine_.step(algorithm, movementInput(in), human, factors);
    }
    // Historical fixtures keep exercising the exact original preset mappings.
    Result step(AimSettings::Version version, const Input &in, control::HumanMotionRuntime &human,
                MotionFactors factors = {})
    {
        return step(historicalMovementAlgorithm(version), in, human, factors);
    }
    bool retryAfterReject() const noexcept
    {
        return engine_.retryAfterReject();
    }
    void reject()
    {
        engine_.reject();
    }
    void reject(AimSettings::Version)
    {
        reject();
    }
    void rejectV2()
    {
        reject();
    }
    const char *status() const noexcept
    {
        return engine_.status();
    }
};
} // namespace mousebox::games::cs2::motion
