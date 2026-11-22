#pragma once
#include "worker.hpp"
#include "shot_gate.hpp"
#include "application/json.hpp"
#include "application/ports/output_program.hpp"
#include "games/cs2/control/output_program.hpp"
#include "games/cs2/control/foreground_policy.hpp"
#include <atomic>
#include <functional>

namespace mousebox::games::cs2::rage
{
class OutputProgram final : public application::ports::OutputProgram
{
    std::function<ControlInput()> source_;
    std::function<std::uint32_t()> foreground_;
    ForegroundPolicy foregroundPolicy_;
    std::atomic<bool> enabled_ = false, requested_ = false;
    mutable std::mutex mutex_;
    Worker worker_;
    cs2::OutputProgram aiming_;
    AimSettings settings_;
    unsigned interval_ = 100, hold_ = 20;
    ScorePolicy scoring_;
    Score selectedScore_;
    double inset_ = .1;
    std::uint64_t generation_ = 0;
    std::uint64_t lastClick_ = 0, clicks_ = 0;
    std::shared_ptr<const Snapshot> fired_;
    struct ShotCheck
    {
        ControlInput input;
        TargetKey target;
        unsigned bone, hitbox;
        ShotEvaluation result;
    };
    struct ShotProposal
    {
        ControlInput input;
        TargetKey target;
        unsigned bone, hitbox;
        std::uint64_t requestedRevision, createdAtMs, checkedAtMs;
    };
    mutable std::optional<ShotCheck> shotCheck_;
    mutable std::optional<ShotProposal> proposed_;
    std::optional<Candidate> selected_;
    bool stopped_ = true, active_ = false, supported_ = false, coverCheck_ = true, otherPlayerCheck_ = true;
    std::string block_ = "disabled";
    mutable std::string shotBlock_;
    bool qualified(const ControlInput &, std::uint64_t) const;
    std::string qualification(const ControlInput &, std::uint64_t) const;
    std::shared_ptr<const Result> analysis(const ControlInput &, std::uint64_t) const;
    const ShotEvaluation &checkShot(const ControlInput &, const Result &) const;
    void clear();

  public:
    static constexpr auto MovementSpeed = control::SpeedPreset::Fastest;

    OutputProgram(std::function<ControlInput()>, std::function<std::uint32_t()>,
                  std::shared_ptr<application::ports::VisibilityProvider>,
                  std::shared_ptr<application::ports::BallisticsProvider>);
    void configure(const application::Json &);
    void setRequested(bool value) noexcept
    {
        requested_ = value;
        aiming_.setRequested(value);
    }
    bool enabled() const noexcept
    {
        return enabled_.load();
    }
    bool requested() const noexcept override
    {
        return enabled_.load() && requested_.load();
    }
    std::optional<control::ActivationBinding> activation() const noexcept override
    {
        return control::ActivationBinding{control::ActivationSource::Continuous, 0};
    }
    application::ports::ProgramStep tick(const application::ports::OutputTick &) override;
    bool current(std::uint64_t, std::uint64_t) const override;
    void reject(const application::ports::ProgramStep &) override;
    void reset() override;
    std::optional<application::ports::ButtonPulseStep> buttonPulse(
        const application::ports::ButtonPulseTick &) override;
    bool buttonPulseCurrent(const application::ports::ButtonPulseStep &, std::uint64_t) const override;
    void buttonPulseAccepted(bool) override;
    application::Json snapshot(bool eligible) const;
};
} // namespace mousebox::games::cs2::rage
