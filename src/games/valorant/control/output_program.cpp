#include "output_program.hpp"
#include <cmath>
#include <stdexcept>

namespace mousebox::games::valorant::aim
{
void OutputProgram::clearMotion()
{
    motion_.reset();
    human_.reset();
    pending_.clear();
    proposal_.reset();
    lastTick_ = 0;
    active_ = false;
}
void OutputProgram::clearHold()
{
    held_.reset();
    identity_.reset();
    holding_ = dead_ = false;
    lastCapture_ = 0;
    feedback_.reset();
    awaitFeedback_.reset();
    feedbackCapture_ = unresponsiveSince_ = 0;
    unconfirmedDegrees_ = 0;
}
bool OutputProgram::confirmed(const Input &input) const
{
    return input.frame && manualView_ && input.frame->identity == *manualView_;
}
void OutputProgram::reset()
{
    const std::lock_guard lock(mutex_);
    clearMotion();
    clearHold();
    releases_.reset();
    released_ = down_ = false;
    stopped_ = true;
    generation_ = 0;
    block_ = enabled_ ? "release-then-hold" : "disabled";
}
void OutputProgram::configure(Settings settings)
{
    const std::lock_guard lock(mutex_);
    if (settings_ == settings)
        return;
    settings_ = settings;
    enabled_ = settings.enabled;
    clearMotion();
    clearHold();
    released_ = down_ = false;
    releases_.reset();
    block_ = enabled_ ? "release-then-hold" : "disabled";
}
void OutputProgram::setHumanModel(std::shared_ptr<const control::HumanMotionModel> model, unsigned dpi, bool variation)
{
    const std::lock_guard lock(mutex_);
    human_.configure(std::move(model), dpi, variation);
    clearMotion();
}
bool OutputProgram::confirmView(bool confirmedValue, std::uint64_t now)
{
    const auto input = observe_();
    const std::lock_guard lock(mutex_);
    if (confirmedValue && !canConfirmView(input, settings_, now))
        throw std::runtime_error("等待本地人物、有效相机和新鲜同源数据后才能确认视角");
    manualView_ = confirmedValue ? std::optional(input.frame->identity) : std::nullopt;
    clearMotion();
    clearHold();
    released_ = false;
    releases_.reset();
    block_ = confirmedValue ? "release-then-hold" : "manual-view-unconfirmed";
    return manualView_.has_value();
}
void OutputProgram::revokeView()
{
    const std::lock_guard lock(mutex_);
    manualView_.reset();
    clearMotion();
    clearHold();
    released_ = false;
    releases_.reset();
}
Status OutputProgram::inspect(std::uint64_t now) const
{
    auto input = observe_();
    const std::lock_guard lock(mutex_);
    input.manualViewConfirmed = confirmed(input);
    const auto solution = solve(input, settings_, now, held_);
    return {settings_,
            solution,
            !enabled_           ? std::string("disabled")
            : active_           ? std::string("tracking")
            : !solution.ready() ? solution.block
                                : block_,
            held_,
            active_,
            down_,
            input.manualViewConfirmed,
            canConfirmView(input, settings_, now),
            static_cast<unsigned>(pending_.size())};
}
bool OutputProgram::current(std::uint64_t token, std::uint64_t now) const
{
    auto input = observe_();
    const std::lock_guard lock(mutex_);
    if (!requested() || !proposal_ || proposal_->token != token || now < proposal_->at || now - proposal_->at > 50 ||
        !input.frame || input.frame->identity != proposal_->identity)
        return false;
    input.manualViewConfirmed = confirmed(input);
    const auto solution = solve(input, settings_, now, proposal_->target);
    if (!solution.ready())
        return false;
    if (settings_.trajectory.kind == control::movement::TrajectoryKind::Linear)
    {
        const auto residual =
            pending_.residualAt(solution.errorDegrees, solution.sampleMs, settings_.responseMs, token);
        const auto increases = [](double error, double change, double scale) {
            return std::abs(error - change) > std::abs(error) + std::abs(scale) * .5 + 1e-9;
        };
        if (increases(residual.x, proposal_->degrees.x, settings_.yawPerCount) ||
            increases(residual.y, proposal_->degrees.y, settings_.pitchPerCount))
            return false;
    }
    return true;
}
void OutputProgram::reject(const application::ports::ProgramStep &step)
{
    const std::lock_guard lock(mutex_);
    if (!proposal_ || proposal_->token != step.sourceRevision)
        return;
    pending_.reject(proposal_->token);
    motion_.reject();
    human_.reset();
    unconfirmedDegrees_ = std::max(0.0, unconfirmedDegrees_ - proposal_->magnitude);
    if (unconfirmedDegrees_ <= 1e-9)
        unresponsiveSince_ = 0;
    proposal_.reset();
    active_ = false;
    stopped_ = true;
    block_ = "output-rejected";
}
} // namespace mousebox::games::valorant::aim
