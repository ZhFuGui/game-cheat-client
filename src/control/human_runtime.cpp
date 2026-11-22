#include "control/human_runtime.hpp"
#include <stdexcept>
#include <algorithm>
#include <cmath>
namespace mousebox::control
{
void HumanMotionRuntime::configure(std::shared_ptr<const HumanMotionModel> model, unsigned dpi, bool variation)
{
    if (model && (!model->valid() || dpi < 100 || dpi > 32000))
        throw std::invalid_argument("Invalid speed profile");
    model_ = std::move(model);
    dpi_ = dpi;
    variation_ = variation;
    reset();
}
HumanSpeedLimits HumanMotionRuntime::limits() const noexcept
{
    if (!model_)
        return {};
    const double factor = dpi_ / 1600.0;
    return {model_->speed90 * factor, model_->acceleration90 * factor, model_->braking90 * factor};
}
HumanVariation HumanMotionRuntime::offset(double x, double y, double angle, std::uint64_t now)
{
    // A bounded lateral target offset, never an accumulating random walk or a learned residual.
    if (!variation_ || angle <= 0.6)
    {
        reset();
        return {};
    }
    const double distance = std::hypot(x, y);
    if (distance < 1 || !std::isfinite(distance))
        return {};
    if (!noiseAt_ || now - noiseAt_ >= 60)
    {
        lateral_ = 0.85 * lateral_ + 0.15 * std::uniform_real_distribution<double>(-0.2, 0.2)(random_);
        noiseAt_ = now;
    }
    const double amount = lateral_ * std::clamp((angle - 0.6) / 0.6, 0.0, 1.0);
    return {-y / distance * amount, x / distance * amount};
}
} // namespace mousebox::control
