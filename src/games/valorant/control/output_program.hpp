#pragma once
#include "model.hpp"
#include "application/ports/output_program.hpp"
#include "control/movement/controller.hpp"
#include "control/movement/pending.hpp"
#include <atomic>
#include <functional>
#include <mutex>

namespace mousebox::games::valorant::aim
{
struct Status
{
    Settings settings;
    Solution solution;
    std::string block;
    std::optional<std::uint64_t> held;
    bool active = false, down = false, manualViewConfirmed = false, canConfirm = false;
    unsigned pending = 0;
};
class OutputProgram final : public application::ports::OutputProgram
{
    std::function<Input()> observe_;
    std::atomic<bool> enabled_ = false;
    std::atomic<bool> sourceRequested_ = false;
    mutable std::mutex mutex_;
    Settings settings_;
    control::HumanMotionRuntime human_;
    control::movement::Controller motion_;
    control::movement::Pending pending_;
    std::optional<Identity> manualView_, identity_;
    std::optional<std::uint64_t> held_;
    std::optional<unsigned> releases_;
    std::optional<control::movement::Vector> feedback_, awaitFeedback_;
    std::uint64_t generation_ = 0, lastTick_ = 0, lastCapture_ = 0, token_ = 0, feedbackCapture_ = 0,
                  unresponsiveSince_ = 0;
    double unconfirmedDegrees_ = 0;
    bool released_ = false, holding_ = false, down_ = false, active_ = false, stopped_ = true, dead_ = false;
    std::string block_ = "disabled";
    struct Proposal
    {
        std::uint64_t token, at, target;
        Identity identity;
        double magnitude;
        control::movement::Vector degrees;
    };
    std::optional<Proposal> proposal_;
    void clearMotion();
    void clearHold();
    bool confirmed(const Input &) const;

  public:
    explicit OutputProgram(std::function<Input()> observe) : observe_(std::move(observe)) {}
    bool requested() const noexcept override
    {
        return enabled_.load() && sourceRequested_.load();
    }
    void setRequested(bool requested) noexcept
    {
        sourceRequested_ = requested;
    }
    void configure(Settings);
    void setHumanModel(std::shared_ptr<const control::HumanMotionModel>, unsigned, bool);
    bool confirmView(bool, std::uint64_t now);
    void revokeView();
    application::ports::ProgramStep tick(const application::ports::OutputTick &) override;
    bool current(std::uint64_t, std::uint64_t) const override;
    void reject(const application::ports::ProgramStep &) override;
    void reset() override;
    Status inspect(std::uint64_t now) const;
};
} // namespace mousebox::games::valorant::aim
