#include "games/pubg/pipeline/live_controller.hpp"
#include "games/pubg/model/ballistics_settings.hpp"
#include <cmath>
#include <stdexcept>
#include <array>
#include <algorithm>
namespace mousebox::control
{
LiveController::LiveController(MotionPolicy policy, AimPart part, double maximumDistanceMeters, TargetPriority priority,
                               WeaponAimPolicy weaponAim)
    : planner_(policy), aimPart_(part), weaponAimPolicy_(weaponAim), maximumDistanceMeters_(maximumDistanceMeters),
      priority_(priority)
{
    if (aimBoneName(part).empty() || !validAimDistance(maximumDistanceMeters) || !validTargetPriority(priority) ||
        !validWeaponAimPolicy(weaponAim))
        throw std::invalid_argument("Invalid live targeting policy");
}
void LiveController::setBallistics(std::shared_ptr<const BallisticCatalog> catalog, unsigned zeroingMeters,
                                   double gravity, double livingLocalSkewMs)
{
    if (!validGravity(gravity) || !validZeroingDistance(zeroingMeters) || !validLivingLocalSkew(livingLocalSkewMs))
        throw std::invalid_argument("Invalid ballistic configuration");
    catalog_ = std::move(catalog);
    zeroingMeters_ = zeroingMeters;
    gravity_ = gravity;
    livingLocalSkewMs_ = livingLocalSkewMs;
    ballisticsEnabled_ = true;
}
LiveDiagnostic LiveController::inspect(std::uint64_t now, const ObservedFrames &frames, const Calibration &c,
                                       bool device, double skew, std::optional<Identity> required)
{
    LiveDiagnostic d;
    if (!frames.realtime)
    {
        selector_.clear();
        return d;
    }
    if (!device)
    {
        selector_.clear();
        d.block = LiveBlock::DeviceUnavailable;
        return d;
    }
    if (!frames.living)
    {
        selector_.clear();
        d.block = LiveBlock::LivingMissing;
        return d;
    }
    if (!frames.view)
    {
        selector_.clear();
        d.block = LiveBlock::ViewMissing;
        return d;
    }
    d.pair = checkPair(*frames.living, *frames.view, {skew, 800});
    TargetPolicy policy;
    policy.part = resolveWeaponAim(*frames.living, catalog_.get(), aimPart_, weaponAimPolicy_).part;
    policy.requiredIdentity = required ? required : heldTarget_;
    policy.maximumDistanceMeters = maximumDistanceMeters_;
    policy.priority = priority_;
    policy.livingLocalSkewMs = livingLocalSkewMs_;
    d.target = selector_.select(*frames.living, *frames.view, d.pair, policy, now);
    d.selection = selector_.stats();
    if (!d.pair.usable())
        d.block = LiveBlock::PairRejected;
    // Observed source progress, not a claimed cross-machine age bound.
    else if (!frames.livingObservedMs || now < frames.livingObservedMs || now - frames.livingObservedMs > 800)
        d.block = LiveBlock::LivingStale;
    else if (!frames.viewObservedMs || now < frames.viewObservedMs || now - frames.viewObservedMs > 800)
        d.block = LiveBlock::ViewStale;
    else if (!validCalibration(c))
        d.block = LiveBlock::CalibrationMissing;
    else if (std::abs(*frames.view->fov - c.fov) > 0.01)
        d.block = LiveBlock::FovMismatch;
    else if (!frames.living->selfTeam)
        d.block = LiveBlock::SelfTeamMissing;
    else if (!d.target)
        d.block = LiveBlock::NoTarget;
    else
        d.block = LiveBlock::Ready;
    // Current native projections carry field-level rotation timing. A valid
    // position timestamp cannot qualify an independently stale camera angle.
    if (d.block == LiveBlock::Ready && frames.view->rotationSourceFinishNs > 0 &&
        !estimateSampleTime(frames.view->rotationSourceStartNs, frames.view->rotationSourceFinishNs,
                            frames.view->header.sourceClock, now)
             .usable())
        d.block = LiveBlock::RotationTimingUnavailable;
    if (d.block == LiveBlock::Ready && ballisticsEnabled_)
    {
        d.ballistics = prepareBallisticAim(*frames.living, *frames.view, *d.target, catalog_.get(), zeroingMeters_, now,
                                           gravity_, livingLocalSkewMs_);
        if (d.ballistics.block == BallisticBlock::Ready)
            d.target->resolvedAim = d.ballistics.aim;
        else
            d.block = LiveBlock::BallisticsUnavailable;
    }
    if (d.block == LiveBlock::Ready && d.ballistics.weapon && d.target->resolvedAim)
    {
        d.nativeAim = qualifyNativeAim(*frames.view, *d.ballistics.weapon, now);
        if ((!heldTarget_ || heldNative_) && d.nativeAim.reference)
            d.target->nativeAim = d.nativeAim.reference;
        if (heldTarget_ && heldNative_ && !d.nativeAim.reference)
            d.block = LiveBlock::NativeReferenceUnavailable;
    }
    return d;
}
bool LiveController::changedContext(const ObservedFrames &frames) const
{
    const auto changed = [](const std::optional<Frame> &current, const std::optional<HoldContext> &previous) {
        if (!current || !previous)
            return false;
        const auto &h = current->header;
        const auto &p = previous->header;
        return (!h.reader.empty() && h.reader != p.reader) || (h.epoch && h.epoch != p.epoch) ||
               (h.world && h.world != p.world) || (h.level && h.level != p.level) ||
               (!h.convention.empty() && h.convention != p.convention) ||
               (!h.clockDomain.empty() && h.clockDomain != p.clockDomain) ||
               (h.mappingRevision && h.mappingRevision != p.mappingRevision) ||
               (current->self && previous->self && current->self != previous->self) ||
               (current->controller && current->controller != previous->controller);
    };
    if (changed(frames.living, heldLiving_) || changed(frames.view, heldView_))
        return true;
    if (frames.living && heldTarget_)
        for (const auto &character : frames.living->characters)
            if ((character.identity.id == heldTarget_->id || sameNativeAddress(character.identity, *heldTarget_)) &&
                character.identity != *heldTarget_)
                return true;
    return false;
}
MotionStep LiveController::step(bool down, std::uint64_t generation, std::uint64_t now, const ObservedFrames &frames,
                                const Calibration &c, bool device, double skew)
{
    if (!down)
    {
        heldTarget_.reset();
        heldLiving_.reset();
        heldView_.reset();
        heldGeneration_ = 0;
        suspended_ = terminated_ = heldNative_ = false;
        selector_.clear();
        heldNativeContext_.clear();
    }
    if (down && !heldTarget_)
        selector_.clear(); // Select at this press, not from a previous release's preview hysteresis.
    diagnostic_ = inspect(now, frames, c, device, skew);
    const auto view = frames.view ? &*frames.view : nullptr;
    if (down && heldTarget_)
    {
        if (terminated_)
            return {};
        if (generation != heldGeneration_ || changedContext(frames) ||
            (heldNative_ && diagnostic_.nativeAim.reference &&
             diagnostic_.nativeAim.reference->context != heldNativeContext_))
        {
            suspended_ = false;
            terminated_ = true;
            auto result = planner_.cancel(MotionEvent::ContextChanged);
            result.stop = true;
            return result;
        }
        if (diagnostic_.block != LiveBlock::Ready)
        {
            const auto block = diagnostic_.block;
            const bool temporary = block == LiveBlock::LivingMissing || block == LiveBlock::ViewMissing ||
                                   block == LiveBlock::LivingStale || block == LiveBlock::ViewStale ||
                                   block == LiveBlock::NoTarget || block == LiveBlock::SelfTeamMissing ||
                                   block == LiveBlock::PairRejected || block == LiveBlock::BallisticsUnavailable ||
                                   block == LiveBlock::RotationTimingUnavailable ||
                                   block == LiveBlock::NativeReferenceUnavailable;
            if (temporary)
            {
                auto result = planner_.cancel(MotionEvent::Suspended);
                suspended_ = true;
                return result;
            }
            suspended_ = false;
            terminated_ = true;
            return planner_.cancel(MotionEvent::EligibilityLost);
        }
        if (suspended_)
        {
            // Reinitialize only the motion estimate. Physical side-key state and the same Reader identity remain
            // authoritative.
            planner_.step(false, generation, now, diagnostic_.target, c, true, view);
            suspended_ = false;
            auto result = planner_.step(true, generation, now, diagnostic_.target, c, true, view);
            result.event = MotionEvent::Resumed;
            return result;
        }
    }
    auto result =
        planner_.step(down, generation, now, diagnostic_.target, c, diagnostic_.block == LiveBlock::Ready, view);
    if (result.begin && !heldTarget_)
    {
        heldTarget_ = diagnostic_.target->identity;
        heldGeneration_ = generation;
        heldNative_ = diagnostic_.target->nativeAim.has_value();
        heldNativeContext_ = heldNative_ ? diagnostic_.target->nativeAim->context : std::string{};
        heldLiving_ = HoldContext{frames.living->header, frames.living->self, frames.living->controller};
        heldView_ = HoldContext{frames.view->header, frames.view->self, frames.view->controller};
    }
    if (result.stop && down)
        terminated_ = true;
    return result;
}
} // namespace mousebox::control
