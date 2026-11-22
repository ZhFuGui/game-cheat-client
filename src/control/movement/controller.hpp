#pragma once
#include "algorithm.hpp"
#include "direct.hpp"
#include "recoil_tracking.hpp"
#include <variant>
namespace mousebox::control::movement
{
// A bounded strategy registry. A new movement model supplies step(Input, human)
// and value-owned state here; targeting, pending accounting and output stay outside.
class Controller
{
    using Strategy = std::variant<Smooth, RecoilTracking, Direct>;
    Strategy strategy_;
    std::optional<Strategy> rollback_;
    BezierPath path_;
    std::optional<BezierPath> pathRollback_;
    std::optional<TrajectorySettings> trajectorySettings_;
    bool curved_ = false;
    Algorithm algorithm_ = Algorithm::Smooth;
    const char *status_ = "v1";

  public:
    void reset()
    {
        *this = {};
    }
    Result step(Algorithm, const Input &, HumanMotionRuntime &, Factors = {});
    // Baseline rejection historically clears the host's motion observations;
    // differential/direct proposals instead restore their bounded private state.
    bool retryAfterReject() const noexcept
    {
        return curved_ || algorithm_ != Algorithm::Smooth;
    }
    void reject();
    const char *status() const noexcept
    {
        return status_;
    }
};
} // namespace mousebox::control::movement
