#pragma once
#include "detection.hpp"
#include "click_timing.hpp"
#include "reaction_monitor.hpp"
#include "games/cs2/control/aim_types.hpp"
#include "games/cs2/control/foreground_policy.hpp"
#include "application/json.hpp"
#include "application/ports/output_program.hpp"
#include <functional>
#include <mutex>
namespace mousebox::games::cs2::trigger
{
class Controller
{
    std::function<ControlInput()> source_;
    std::function<std::uint32_t()> foreground_;
    ForegroundPolicy foregroundPolicy_;
    mutable std::mutex mutex_;
    bool enabled_ = false, supported_ = false;
    unsigned ageLimit_ = 150;
    random::IntegerRange interval_ = DefaultInterval, hold_ = DefaultHold;
    std::mt19937 random_{std::random_device{}()};
    unsigned nextIntervalMs_ = 0;
    std::optional<unsigned> nextHoldMs_;
    double inset_ = 0, responseMs_ = 20;
    int timeOffsetMs_ = 0;
    float sniperInaccuracyLimit_ = static_cast<float>(DefaultSniperInaccuracyLimit);
    mutable std::shared_ptr<const Snapshot> geometryFrame_;
    mutable std::unique_ptr<DetectionScene> geometry_;
    bool coverCheck_ = false;
    bool reactionEnabled_ = false;
    unsigned reactionMs_ = 0;
    WeaponFilter weapons_;
    TargetFilter targets_;
    mutable StaticCover cover_;
    ReactionMonitor reaction_;
    struct Proposal
    {
        std::shared_ptr<const Snapshot> frame;
        TargetKey target;
        application::ports::ButtonPulseStep step;
        std::uint64_t atMs, policy;
    };
    mutable std::optional<Proposal> proposal_;
    std::shared_ptr<const Snapshot> firedFrame_;
    std::uint64_t policy_ = 0, lastClick_ = 0, clicks_ = 0;
    Detection detection_;
    mutable std::string block_ = "disabled";
    bool current(const ControlInput &, std::uint64_t) const;
    Detection detectAt(const ControlInput &, std::uint64_t) const;
    std::string shotStatus(const ControlInput &, const StaticCover::Access &, const Detection &,
                           std::uint64_t now) const;

  public:
    Controller(std::function<ControlInput()> source, std::function<std::uint32_t()> foreground,
               std::shared_ptr<application::ports::VisibilityProvider> geometry = {})
        : source_(std::move(source)), foreground_(std::move(foreground)), cover_(std::move(geometry))
    {
    }
    void configure(const application::Json &);
    void reset();
    bool enabled() const
    {
        std::lock_guard lock(mutex_);
        return enabled_;
    }
    std::optional<application::ports::ButtonPulseStep> tick(const application::ports::ButtonPulseTick &);
    bool current(const application::ports::ButtonPulseStep &, std::uint64_t) const;
    void accepted(bool);
    application::Json snapshot(bool eligible) const;
};
// Composition preserves the existing movement program and its activation unchanged.
class OutputProgram final : public application::ports::OutputProgram
{
    std::shared_ptr<application::ports::OutputProgram> movement_;
    std::shared_ptr<Controller> trigger_;

  public:
    OutputProgram(std::shared_ptr<application::ports::OutputProgram> movement, std::shared_ptr<Controller> trigger)
        : movement_(std::move(movement)), trigger_(std::move(trigger))
    {
    }
    bool requested() const noexcept override
    {
        return movement_->requested();
    }
    std::optional<control::ActivationBinding> activation() const noexcept override
    {
        return movement_->activation();
    }
    application::ports::ProgramStep tick(const application::ports::OutputTick &t) override
    {
        return movement_->tick(t);
    }
    bool current(std::uint64_t r, std::uint64_t at) const override
    {
        return movement_->current(r, at);
    }
    void reject(const application::ports::ProgramStep &s) override
    {
        movement_->reject(s);
    }
    void reset() override
    {
        movement_->reset();
        trigger_->reset();
    }
    std::optional<application::ports::ButtonPulseStep> buttonPulse(
        const application::ports::ButtonPulseTick &t) override
    {
        return trigger_->tick(t);
    }
    bool buttonPulseCurrent(const application::ports::ButtonPulseStep &s, std::uint64_t at) const override
    {
        return trigger_->current(s, at);
    }
    void buttonPulseAccepted(bool accepted) override
    {
        trigger_->accepted(accepted);
    }
};
} // namespace mousebox::games::cs2::trigger
