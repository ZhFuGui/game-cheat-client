#include "controller.hpp"
#include "application/settings/random_range.hpp"
#include "control/sample_time.hpp"
#include "games/cs2/settings/hitbox_inset.hpp"
#include <algorithm>
#include <chrono>
namespace mousebox::games::cs2::trigger
{
namespace
{
TargetKey targetKey(const Detection &detection)
{
    return {detection.controller, detection.target};
}
} // namespace
void Controller::configure(const application::Json &v)
{
    std::lock_guard lock(mutex_);
    // Dispatch rechecks both settings without clearing exposure or click history.
    timeOffsetMs_ = v.value("cs2TriggerTimeOffsetMs", 0);
    // The Reader transmits Inaccuracy as a float. Compare at that precision.
    sniperInaccuracyLimit_ =
        static_cast<float>(v.value("cs2TriggerSniperInaccuracyLimit", DefaultSniperInaccuracyLimit));
    const WeaponFilter weapons{v.value("cs2TriggerSnipers", true), v.value("cs2TriggerOtherGuns", true)};
    const auto targets = targetFilterSettings(v);
    const ForegroundPolicy foregroundPolicy{v.value("cs2CheckForeground", true)};
    const auto enabled = v.value("cs2TriggerEnabled", false);
    const auto cover = v.value("cs2TriggerCoverCheck", false);
    const auto reaction = cover && v.value("cs2TriggerReactionEnabled", false);
    const auto reactionMs = v.value("cs2TriggerReactionMs", 0u);
    const auto ageLimit = v.value("cs2TriggerMaxAgeMs", 150u);
    const auto inset = readHitboxInsetPercent(v) / 100;
    const auto responseMs = v.value("cs2ResponseMs", 20.);
    const auto storedHold = v.find("cs2TriggerHoldRangeMs");
    const auto hold = storedHold == v.end() ? DefaultHold :
                                             application::readRandomRange(*storedHold, HoldBounds.min, HoldBounds.max);
    const bool coverChanged = coverCheck_ != cover || enabled_ != enabled;
    const bool exposureChanged = coverChanged || reactionEnabled_ != reaction || targets_ != targets ||
                                 foregroundPolicy_ != foregroundPolicy || ageLimit_ != ageLimit;
    const bool geometryChanged = weapons_ != weapons || targets_ != targets;
    if (exposureChanged || geometryChanged || hold_ != hold || (reaction && reactionMs_ != reactionMs) ||
        inset_ != inset || responseMs_ != responseMs)
        ++policy_;
    if (coverChanged)
        cover_.reset();
    if (exposureChanged)
        reaction_.reset();
    if (geometryChanged)
    {
        geometry_.reset();
        geometryFrame_.reset();
        detection_ = {};
    }
    if (hold_ != hold || enabled_ != enabled)
        nextHoldMs_.reset();
    minimumIntervalMs_ = v.value("cs2TriggerIntervalMs", DefaultIntervalMs);
    hold_ = hold;
    enabled_ = enabled;
    ageLimit_ = ageLimit;
    coverCheck_ = cover;
    reactionEnabled_ = reaction;
    reactionMs_ = reactionMs;
    weapons_ = weapons;
    targets_ = targets;
    foregroundPolicy_ = foregroundPolicy;
    inset_ = inset;
    responseMs_ = responseMs;
    if (exposureChanged)
        block_ = enabled_ ? "waiting-source" : "disabled";
}
void Controller::reset()
{
    std::lock_guard lock(mutex_);
    cover_.reset();
    reaction_.reset();
    geometry_.reset();
    geometryFrame_.reset();
    lastClick_ = 0;
    nextIntervalMs_ = 0;
    nextHoldMs_.reset();
    detection_ = {};
    proposal_.reset();
    firedFrame_.reset();
    block_ = enabled_ ? "waiting-source" : "disabled";
}
bool Controller::current(const ControlInput &in, std::uint64_t now) const
{
    now = std::max(now, in.observedAtMs);
    if (!in.current || !in.requested || !in.frame || !in.sample || !in.sample->current)
        return false;
    const auto &f = *in.frame;
    if (!foregroundPolicy_.permits(f.processId, foreground_))
        return false;
    const auto &s = *in.sample;
    if (s.reader != f.instance || s.clockDomain != f.clockDomain || s.epoch != f.epoch || s.capture != f.sequence)
        return false;
    control::PredictionPolicy p;
    p.maximumAgeMs = ageLimit_;
    return control::estimateSampleTime(s.sourceStartNs, s.sourceFinishNs, s.sourceClock, now, p).usable();
}
Detection Controller::detectAt(const ControlInput &in, std::uint64_t now) const
{
    const auto started = std::chrono::steady_clock::now();
    if (!in.frame || !in.sample)
        return {"source-unavailable"};
    if (geometryFrame_ != in.frame || !geometry_)
    {
        geometry_ = std::make_unique<DetectionScene>(*in.frame, weapons_, targets_);
        geometryFrame_ = in.frame;
    }
    const auto evaluate = [&](std::uint64_t at) {
        control::PredictionPolicy policy;
        policy.maximumAgeMs = ageLimit_;
        const auto &s = *in.sample;
        const auto timing = control::estimateSampleTime(s.sourceStartNs, s.sourceFinishNs, s.sourceClock, at, policy);
        if (!timing.usable())
            return Detection{"source-unavailable"};
        const double horizonMs = std::max(0., timing.ageMs + responseMs_ + timeOffsetMs_);
        return geometry_->detect({inset_, horizonMs, sniperInaccuracyLimit_, in.weaponAccuracyEvaluation});
    };
    now = std::max(now, in.observedAtMs);
    auto result = evaluate(now);
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count();
    // Include cold pose preparation without rebuilding the frame. Dispatch
    // recomputes time, using the immutable geometry of this capture.
    return elapsed > 0 ? evaluate(now + static_cast<std::uint64_t>(elapsed)) : result;
}
std::string Controller::shotStatus(const ControlInput &in, const StaticCover::Access &access, const Detection &hit,
                                   std::uint64_t now) const
{
    if (!coverCheck_)
        return "ready";
    // The shot path uses the predicted impact. Exposure uses sampled bones.
    const auto coverStatus = access.check(hit.origin, hit.point);
    if (coverStatus != "ready" || !reactionEnabled_)
        return coverStatus;
    const auto target = targetKey(hit);
    const auto evidence = reaction_.evidence(*in.frame, target);
    if (!evidence)
        return "reaction-pending";
    if (evidence->failed)
        return evidence->current ? "reaction-unavailable" : "reaction-pending";
    const auto &exposure = evidence->exposure;
    if (!exposure || exposure->visibility != application::ports::Visibility::Visible || !exposure->visibleSinceNs)
    {
        if (!evidence->current)
            return "reaction-pending";
        return !exposure || exposure->visibility == application::ports::Visibility::Occluded ? "reaction-hidden"
                                                                                             : "cover-unknown";
    }
    if (remainingReactionMs(*exposure->visibleSinceNs, *in.sample->sourceClock, now, reactionMs_) > 0)
        return "reaction-waiting";
    if (evidence->current)
        return "ready";
    // A completed adjacent observation keeps its start only when the same
    // bone is independently verified visible in this capture.
    try
    {
        return exposure->visibleBone && remainsVisible(*in.frame, *access.scene, target, *exposure->visibleBone)
                   ? "ready"
                   : "reaction-pending";
    }
    catch (...)
    {
        return "reaction-unavailable";
    }
}
std::optional<application::ports::ButtonPulseStep> Controller::tick(const application::ports::ButtonPulseTick &t)
{
    const auto in = source_();
    const auto now = std::max(t.nowMs, in.observedAtMs);
    std::lock_guard lock(mutex_);
    proposal_.reset();
    supported_ = t.supported;
    if (!enabled_)
    {
        block_ = "disabled";
        return {};
    }
    if (!targets_.any())
    {
        block_ = "targets-disabled";
        return {};
    }
    const bool qualified = current(in, now);
    StaticCover::Access access;
    if (qualified)
    {
        if (coverCheck_)
            access = cover_.acquire(in);
        // Observe exposures even when there is no aimed hit or a click is on cooldown.
        if (reactionEnabled_)
            reaction_.observe(in, access, targets_);
        detection_ = detectAt(in, now);
    }
    else
    {
        reaction_.reset();
        detection_ = {};
    }
    if (!t.supported)
    {
        block_ = "firmware-required";
        return {};
    }
    if (!t.fresh)
    {
        block_ = "device-unavailable";
        return {};
    }
    if (t.physicalLeftDown)
    {
        block_ = "physical-left-held";
        return {};
    }
    if (in.frame && !foregroundPolicy_.permits(in.frame->processId, foreground_))
    {
        block_ = "waiting-game-foreground";
        return {};
    }
    if (!qualified)
    {
        block_ = "source-unavailable";
        return {};
    }
    block_ = detection_.block;
    if (block_ != "ready")
        return {};
    const bool sameCapture = firedFrame_ && firedFrame_->sequence == in.frame->sequence &&
                             firedFrame_->instance == in.frame->instance && firedFrame_->epoch == in.frame->epoch &&
                             firedFrame_->clockDomain == in.frame->clockDomain;
    if (sameCapture || (lastClick_ && (now < lastClick_ || now - lastClick_ < nextIntervalMs_)))
    {
        block_ = "cooldown";
        return {};
    }
    block_ = shotStatus(in, access, detection_, now);
    if (block_ != "ready")
        return {};
    if (!nextHoldMs_)
        nextHoldMs_ = hold_.sample(random_);
    const application::ports::ButtonPulseStep step{in.revision, *nextHoldMs_};
    proposal_ = Proposal{in.frame, targetKey(detection_), step, now, policy_};
    return step;
}
bool Controller::current(const application::ports::ButtonPulseStep &step, std::uint64_t now) const
{
    const auto in = source_();
    now = std::max(now, in.observedAtMs);
    std::lock_guard lock(mutex_);
    if (!enabled_ || !proposal_ || proposal_->policy != policy_ || proposal_->step.holdMs != step.holdMs ||
        proposal_->step.sourceRevision != step.sourceRevision)
        return false;
    if (in.frame != proposal_->frame || in.revision != step.sourceRevision || !current(in, now))
    {
        block_ = "source-unavailable";
        return false;
    }
    const auto latest = detectAt(in, now);
    block_ = latest.block;
    if (block_ != "ready")
        return false;
    if (targetKey(latest) != proposal_->target)
    {
        block_ = "no-hit";
        return false;
    }
    const auto access = coverCheck_ ? cover_.acquire(in) : StaticCover::Access{};
    block_ = shotStatus(in, access, latest, now);
    if (block_ != "ready")
        return false;
    proposal_->atMs = now;
    return true;
}
void Controller::accepted(bool value)
{
    std::lock_guard lock(mutex_);
    if (!proposal_)
        return;
    if (value)
    {
        lastClick_ = proposal_->atMs;
        firedFrame_ = proposal_->frame;
        nextIntervalMs_ = minimumIntervalMs_;
        nextHoldMs_.reset();
        ++clicks_;
        block_ = "clicked";
    }
    else if (block_ == "ready")
        block_ = "output-busy";
    proposal_.reset();
}
application::Json Controller::snapshot(bool eligible) const
{
    std::lock_guard lock(mutex_);
    const auto block = !enabled_         ? "disabled"
                       : !eligible       ? "session-ineligible"
                       : !targets_.any() ? "targets-disabled"
                                         : block_;
    return {{"enabled", enabled_},
            {"supported", supported_},
            {"block", block},
            {"clicks", clicks_},
            {"target", detection_.target ? application::Json(detection_.target) : application::Json(nullptr)},
            {"hitbox", detection_.target ? application::Json(detection_.hitbox) : application::Json(nullptr)}};
}
} // namespace mousebox::games::cs2::trigger
