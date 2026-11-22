#include "output_program.hpp"
#include "settings.hpp"
#include "games/cs2/control/pipeline/frame_context.hpp"
#include "games/cs2/extensions/frame_input.hpp"
#include "games/cs2/extensions/remaining_damage/inputs.hpp"
#include "games/cs2/hitboxes/frame.hpp"
#include "games/cs2/hitboxes/intersection.hpp"
#include "games/cs2/settings/hitbox_inset.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace mousebox::games::cs2::rage
{
namespace
{
bool compatibleSource(const ControlInput &previous, const ControlInput &current)
{
    if (!previous.frame || !current.frame || !previous.sample || !current.sample ||
        extensions::scope(*previous.frame) != extensions::scope(*current.frame) ||
        previous.sample->session != current.sample->session ||
        previous.sample->consumerConnection != current.sample->consumerConnection ||
        previous.frame->sequence > current.frame->sequence ||
        previous.sample->sourceStartNs > current.sample->sourceStartNs)
        return false;
    const auto old = extensions::shotContext(*previous.frame), now = extensions::shotContext(*current.frame);
    return old && now && old->pawn == now->pawn && old->controller == now->controller && old->weapon == now->weapon &&
           old->definition == now->definition && old->team == now->team;
}
} // namespace
OutputProgram::OutputProgram(std::function<ControlInput()> source, std::function<std::uint32_t()> foreground,
                             std::shared_ptr<application::ports::VisibilityProvider> v,
                             std::shared_ptr<application::ports::BallisticsProvider> b)
    : source_(std::move(source)), foreground_(std::move(foreground)), worker_(std::move(v), std::move(b)),
      aiming_(source_, [this](const ControlInput &in) -> std::optional<TargetPoint> {
          if (!selected_ || !in.frame)
              return {};
          const auto point = currentAimPoint(*in.frame, *selected_, settings_.targets);
          return point ? std::optional(TargetPoint{selected_->target, *point, selected_->bone}) : std::nullopt;
      })
{
}
void OutputProgram::clear()
{
    worker_.reset();
    aiming_.reset();
    selected_.reset();
    selectedScore_ = {};
    active_ = false;
    proposed_.reset();
    shotCheck_.reset();
    shotBlock_.clear();
}
void OutputProgram::configure(const application::Json &v)
{
    auto settings = aimSettings(v);
    settings.movement = control::movement::Algorithm::Direct;
    settings.trajectory = {};
    settings.speed = MovementSpeed;
    std::lock_guard lock(mutex_);
    const auto enabled = v.value("cs2RageEnabled", false);
    const auto coverCheck = v.value("cs2RageCoverCheck", true);
    const auto otherPlayerCheck = v.value("cs2RageOtherPlayerCheck", true);
    const ForegroundPolicy foregroundPolicy{v.value("cs2CheckForeground", true)};
    const auto scoring = scorePolicy(v);
    const auto inset = readHitboxInsetPercent(v) / 100;
    const auto interval = v.value("cs2RageIntervalMs", 100u), hold = v.value("cs2RageHoldMs", 20u);
    if (enabled_ == enabled && settings_ == settings && scoring_ == scoring && inset_ == inset &&
        interval_ == interval && hold_ == hold && coverCheck_ == coverCheck && otherPlayerCheck_ == otherPlayerCheck &&
        foregroundPolicy_ == foregroundPolicy)
        return;
    settings_ = settings;
    coverCheck_ = coverCheck;
    otherPlayerCheck_ = otherPlayerCheck;
    foregroundPolicy_ = foregroundPolicy;
    aiming_.configure(settings_);
    enabled_ = enabled;
    scoring_ = scoring;
    inset_ = inset;
    interval_ = interval;
    hold_ = hold;
    clear();
    fired_.reset();
    block_ = enabled ? "waiting-source" : "disabled";
}
void OutputProgram::reset()
{
    std::lock_guard lock(mutex_);
    clear();
    generation_ = 0;
    fired_.reset();
    block_ = enabled_ ? "waiting-source" : "disabled";
}
bool OutputProgram::qualified(const ControlInput &in, std::uint64_t now) const
{
    return qualification(in, now) == "ready";
}
std::string OutputProgram::qualification(const ControlInput &in, std::uint64_t now) const
{
    if (!settings_.targets.any())
        return "targets-disabled";
    if (!requested() || !in.requested)
        return "reader-not-requested";
    if (!in.current || !in.frame)
        return "waiting-source";
    if (!foregroundPolicy_.permits(in.frame->processId, foreground_))
        return "waiting-game-foreground";
    if (!in.frame->map || !in.frame->map->available())
        return "map-unavailable";
    if (!in.sample || !in.sample->current)
        return "sample-missing";
    if (in.sample->reader != in.frame->instance || in.sample->clockDomain != in.frame->clockDomain ||
        in.sample->epoch != in.frame->epoch || in.sample->capture != in.frame->sequence)
        return "sample-identity-mismatch";
    const auto c = pipeline::qualifyFrame(in, settings_, std::max(now, in.observedAtMs));
    if (c.block != "clock-unavailable")
        return c.block;
    switch (c.timing.reason)
    {
    case control::PredictionReason::ClockStale:
        return "clock-stale";
    case control::PredictionReason::ClockUncertain:
        return "clock-uncertain";
    case control::PredictionReason::AgeOutOfRange:
        return "sample-too-old";
    case control::PredictionReason::SourceTimingInvalid:
        return "sample-timing-invalid";
    default:
        return "clock-unavailable";
    }
}
std::shared_ptr<const Result> OutputProgram::analysis(const ControlInput &in, std::uint64_t now) const
{
    const auto r = worker_.result();
    if (!r || r->filter != settings_.targets || r->policy != scoring_ || r->coverCheck != coverCheck_ ||
        r->otherPlayerCheck != otherPlayerCheck_ || !compatibleSource(r->input, in))
        return {};
    control::PredictionPolicy policy;
    policy.maximumAgeMs = std::min(100., settings_.maximumAgeMs);
    const auto &s = *r->input.sample;
    if (!control::estimateSampleTime(s.sourceStartNs, s.sourceFinishNs, s.sourceClock, std::max(now, in.observedAtMs),
                                     policy)
             .usable())
        return {};
    return r;
}
application::ports::ProgramStep OutputProgram::tick(const application::ports::OutputTick &t)
{
    const auto in = source_();
    const auto now = std::max(t.nowMs, in.observedAtMs);
    std::lock_guard lock(mutex_);
    application::ports::ProgramStep out;
    out.sourceRevision = in.revision;
    const auto pause = [&](std::string reason) {
        block_ = std::move(reason);
        active_ = false;
        proposed_.reset();
        shotCheck_.reset();
        shotBlock_.clear();
        aiming_.reset();
        out.motion.stop = !stopped_;
        stopped_ = true;
        return out;
    };
    if (generation_ && generation_ != t.generation)
        clear();
    generation_ = t.generation;
    if (!enabled_)
        return pause("disabled");
    if (!settings_.targets.any())
        return pause("targets-disabled");
    if (in.frame && !foregroundPolicy_.permits(in.frame->processId, foreground_))
    {
        clear();
        return pause("waiting-game-foreground");
    }
    if (!t.buttonFresh || !t.limits.valid())
        return pause("device-unavailable");
    if (!supported_)
        return pause("firmware-required");
    const auto reason = qualification(in, now);
    if (reason != "ready")
    {
        clear();
        return pause(reason);
    }
    worker_.submit(in, settings_.targets, scoring_, coverCheck_, otherPlayerCheck_);
    const auto r = analysis(in, now);
    if (!r)
        return pause("analysis-pending");
    const auto previous = selected_ && currentBone(*in.frame, *selected_, settings_.targets)
                              ? std::optional(selected_->target)
                              : std::nullopt;
    const auto selection = selectScored(r->analysis, scoring_, previous, in.frame.get(), settings_.targets);
    const auto candidate = selection.candidate;
    if (!candidate)
    {
        selected_.reset();
        return pause(r->analysis.block);
    }
    if (!currentAimPoint(*in.frame, *candidate, settings_.targets))
        return pause("target-unavailable");
    if (!selected_ || selected_->target != candidate->target || selected_->bone != candidate->bone ||
        selected_->hitbox != candidate->hitbox)
    {
        proposed_.reset();
        shotCheck_.reset();
        shotBlock_.clear();
    }
    selected_ = *candidate;
    selectedScore_ = selection.score;
    out = aiming_.tick(t);
    if (out.motion.begin || out.motion.dx || out.motion.dy)
        stopped_ = false;
    if (out.motion.stop)
        stopped_ = true;
    active_ = out.active;
    block_ = aiming_.lastBlock();
    return out;
}
bool OutputProgram::current(std::uint64_t revision, std::uint64_t now) const
{
    const auto in = source_();
    std::lock_guard lock(mutex_);
    return active_ && in.revision == revision && qualified(in, now) && selected_ &&
           currentAimPoint(*in.frame, *selected_, settings_.targets) && analysis(in, now) && aiming_.current(in, now);
}
void OutputProgram::reject(const application::ports::ProgramStep &step)
{
    std::lock_guard lock(mutex_);
    aiming_.reject(step);
    active_ = false;
    proposed_.reset();
    block_ = "output-busy";
}
const ShotEvaluation &OutputProgram::checkShot(const ControlInput &in, const Result &r) const
{
    const auto hitbox = selected_->hitbox.value_or(0);
    if (!shotCheck_ || shotCheck_->input.frame != in.frame || shotCheck_->input.revision != in.revision ||
        shotCheck_->target != selected_->target || shotCheck_->bone != selected_->bone || shotCheck_->hitbox != hitbox)
        shotCheck_ = ShotCheck{in, selected_->target, selected_->bone, hitbox,
                               evaluateShot(*in.frame, *selected_, settings_.targets, coverCheck_, otherPlayerCheck_,
                                            inset_, r.visibility.get(), r.ballistics.get())};
    return shotCheck_->result;
}
std::optional<application::ports::ButtonPulseStep> OutputProgram::buttonPulse(
    const application::ports::ButtonPulseTick &t)
{
    const auto in = source_();
    const auto now = std::max(t.nowMs, in.observedAtMs);
    std::lock_guard lock(mutex_);
    supported_ = t.supported;
    const auto reject = [&](std::string reason) -> std::optional<application::ports::ButtonPulseStep> {
        shotBlock_ = std::move(reason);
        proposed_.reset();
        return {};
    };
    const auto reason = qualification(in, now);
    if (reason != "ready")
        return reject(reason);
    if (!active_ || !selected_ || t.generation != generation_)
        return reject("shot-pending");
    if (!t.fresh)
        return reject("button-stale");
    if (!t.supported)
        return reject("firmware-required");
    if (t.physicalLeftDown)
        return reject("physical-left-held");
    if (!aiming_.current(in, now))
        return reject("aim-not-current");
    if (fired_ && fired_->sequence == in.frame->sequence && fired_->instance == in.frame->instance &&
        fired_->epoch == in.frame->epoch)
        return reject("capture-consumed");
    if (lastClick_ && (now < lastClick_ || now - lastClick_ < interval_))
        return reject("cooldown");
    const auto r = analysis(in, now);
    if (!r)
        return reject("analysis-pending");
    const auto &shot = checkShot(in, *r);
    if (shot.block != "ready")
        return reject(shot.block);
    proposed_ =
        ShotProposal{in, selected_->target, selected_->bone, selected_->hitbox.value_or(0), in.revision, now, now};
    shotBlock_ = "shot-ready";
    return application::ports::ButtonPulseStep{in.revision, hold_};
}
bool OutputProgram::buttonPulseCurrent(const application::ports::ButtonPulseStep &s, std::uint64_t now) const
{
    const auto in = source_();
    now = std::max(now, in.observedAtMs);
    const auto started = std::chrono::steady_clock::now();
    std::lock_guard lock(mutex_);
    const auto reject = [&](std::string reason) {
        shotBlock_ = std::move(reason);
        proposed_.reset();
        return false;
    };
    if (!proposed_ || !active_ || !selected_ || s.sourceRevision != proposed_->requestedRevision || s.holdMs != hold_ ||
        proposed_->target != selected_->target || proposed_->bone != selected_->bone ||
        proposed_->hitbox != selected_->hitbox.value_or(0))
        return reject("shot-pending");
    const auto reason = qualification(in, now);
    if (reason != "ready")
        return reject(reason);
    if (!compatibleSource(proposed_->input, in))
        return reject("context-changed");
    if (now < proposed_->createdAtMs || now - proposed_->createdAtMs > std::min(100., settings_.maximumAgeMs))
        return reject("shot-expired");
    if (!aiming_.current(in, now))
        return reject("aim-not-current");
    if (fired_ && fired_->sequence == in.frame->sequence && fired_->instance == in.frame->instance &&
        fired_->epoch == in.frame->epoch)
        return reject("capture-consumed");
    if (lastClick_ && (now < lastClick_ || now - lastClick_ < interval_))
        return reject("cooldown");
    const auto r = analysis(in, now);
    if (!r)
        return reject("analysis-pending");
    const auto &shot = checkShot(in, *r);
    if (shot.block != "ready")
        return reject(shot.block);
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count();
    const auto completedAt = now + static_cast<std::uint64_t>(elapsed);
    if (completedAt - proposed_->createdAtMs > std::min(100., settings_.maximumAgeMs))
        return reject("shot-expired");
    const auto completedReason = qualification(in, completedAt);
    if (completedReason != "ready")
        return reject(completedReason);
    // A newer capture is revalidated in full rather than cancelling a safe
    // transaction merely because the background ranker has not processed it.
    proposed_->input = in;
    proposed_->checkedAtMs = completedAt;
    shotBlock_ = "shot-ready";
    return true;
}
void OutputProgram::buttonPulseAccepted(bool accepted)
{
    std::lock_guard lock(mutex_);
    if (accepted && proposed_)
    {
        ++clicks_;
        lastClick_ = proposed_->checkedAtMs;
        fired_ = proposed_->input.frame;
    }
    if (accepted)
        block_ = shotBlock_ = "clicked";
    else if (shotBlock_ == "shot-ready" || shotBlock_.empty())
        block_ = shotBlock_ = "output-busy";
    proposed_.reset();
}
application::Json OutputProgram::snapshot(bool eligible) const
{
    std::lock_guard lock(mutex_);
    using application::Json;
    const auto value = [](const auto &v) { return v ? Json(*v) : Json(nullptr); };
    Json scoring = nullptr;
    if (selected_)
    {
        const auto &e = selected_->evidence;
        scoring = {{"model", scoreModelId(scoring_.model)},
                   {"aim", value(selectedScore_.aim)},
                   {"incomingDamage", value(e.incomingDamage)},
                   {"incomingDamageScore", value(selectedScore_.incomingDamageScore)},
                   {"outgoingDamageScore", selectedScore_.outgoingDamageScore},
                   {"threatScore", value(selectedScore_.threat)},
                   {"lowAmmoDeduction", selectedScore_.lowAmmoDeduction},
                   {"reloadDeduction", selectedScore_.reloadDeduction},
                   {"totalScore", selectedScore_.total},
                   {"degraded", selectedScore_.degraded},
                   {"stabilized", selectedScore_.stabilized},
                   {"weapon", value(e.weapon)},
                   {"clip", value(e.clip)},
                   {"reloading", value(e.reloading)},
                   {"assumedFriendlyFire", e.assumedFriendlyFire}};
    }
    return {{"enabled", enabled_.load()},
            {"active", active_ && eligible},
            {"supported", supported_},
            {"block", !eligible && enabled_                                                  ? "session-ineligible"
                      : enabled_ && !settings_.targets.any()                                 ? "targets-disabled"
                      : (block_ == "tracking" || block_ == "clicked") && !shotBlock_.empty() ? shotBlock_
                                                                                             : block_},
            {"clicks", clicks_},
            {"target", selected_ ? Json(selected_->target.pawn) : Json{}},
            {"bone", selected_ ? Json(selected_->bone) : Json{}},
            {"damage", selected_ ? Json(selected_->damage) : Json{}},
            {"threat", selected_ && selectedScore_.threat ? Json(*selectedScore_.threat / 100) : Json{}},
            {"scoring", std::move(scoring)}};
}
} // namespace mousebox::games::cs2::rage
