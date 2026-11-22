#include "controller.hpp"
#include "path_input.hpp"
#include <type_traits>
namespace mousebox::control::movement
{
Result Controller::step(Algorithm algorithm, const Input &in, HumanMotionRuntime &human, Factors factors)
{
    if (algorithm == Algorithm::Historical)
        throw std::invalid_argument("历史方案必须先解析为具体鼠标移动算法");
    if (algorithm != algorithm_ || (trajectorySettings_ && *trajectorySettings_ != in.policy.trajectory))
    {
        switch (algorithm)
        {
        case Algorithm::Smooth:
            strategy_.emplace<Smooth>();
            break;
        case Algorithm::RecoilTracking:
            strategy_.emplace<RecoilTracking>();
            break;
        case Algorithm::Direct:
            strategy_.emplace<Direct>();
            break;
        default:
            throw std::invalid_argument("未知鼠标移动算法");
        }
        algorithm_ = algorithm;
        path_.reset();
    }
    trajectorySettings_ = in.policy.trajectory;
    curved_ = in.policy.trajectory.kind == TrajectoryKind::Bezier;
    rollback_.reset();
    pathRollback_.reset();
    if (retryAfterReject())
        rollback_ = strategy_;
    if (curved_)
        pathRollback_ = path_;
    const auto input = curved_ ? pathInput(in, path_) : in;
    if (curved_)
        factors.variation = false;
    auto result = std::visit(
        [&](auto &strategy) {
            if constexpr (std::is_same_v<std::decay_t<decltype(strategy)>, Smooth>)
                return strategy.step(input, human, {}, factors);
            else
                return strategy.step(input, human);
        },
        strategy_);
    if (curved_)
        result = pathResult(result, in, path_);
    status_ = result.status;
    return result;
}
void Controller::reject()
{
    if (rollback_)
        strategy_ = *rollback_;
    else if (!retryAfterReject())
        strategy_.emplace<Smooth>();
    rollback_.reset();
    if (pathRollback_)
        path_ = *pathRollback_;
    pathRollback_.reset();
    status_ = "output-rejected";
}
} // namespace mousebox::control::movement
