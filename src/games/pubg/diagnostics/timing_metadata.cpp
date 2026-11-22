#include "timing_metadata.hpp"
#include "control/sample_time.hpp"
#include <array>
#include <cmath>

namespace mousebox::application
{
namespace
{
const char *reasonName(control::PredictionReason reason)
{
    constexpr std::array names{"applied",
                               "velocity_missing",
                               "geometry_missing",
                               "velocity_invalid",
                               "source_interval_invalid",
                               "velocity_sample_skew",
                               "clock_unavailable",
                               "clock_stale",
                               "clock_uncertain",
                               "age_out_of_range"};
    const auto index = static_cast<unsigned>(reason);
    return index < names.size() ? names[index] : "unknown";
}
Json number(double value)
{
    return std::isfinite(value) ? Json(value) : Json{};
}
Json clockMetadata(const std::optional<control::SourceClockEstimate> &clock, std::uint64_t now)
{
    if (!clock)
        return nullptr;
    return {{"source_anchor_ns", std::to_string(clock->sourceAnchorNs)},
            {"local_anchor_ms", number(clock->localAnchorMs)},
            {"uncertainty_ms", number(clock->uncertaintyMs)},
            {"calibrated_at_ms", std::to_string(clock->calibratedAtMs)},
            {"probe_age_ms", now >= clock->calibratedAtMs ? Json(now - clock->calibratedAtMs) : Json{}}};
}
Json sample(std::int64_t start, std::int64_t finish, const std::optional<control::SourceClockEstimate> &clock,
            std::uint64_t now)
{
    const auto evaluated = control::estimateSampleTime(start, finish, clock, now);
    Json mappedAge;
    Json uncertainty;
    if (clock && start > 0 && finish >= start && clock->sourceAnchorNs > 0 && std::isfinite(clock->localAnchorMs) &&
        std::isfinite(clock->uncertaintyMs) && clock->uncertaintyMs >= 0)
    {
        const long double midpoint = (static_cast<long double>(start) + finish) / 2;
        mappedAge =
            number(static_cast<double>(now - (clock->localAnchorMs + (midpoint - clock->sourceAnchorNs) / 1e6L)));
        uncertainty = number(clock->uncertaintyMs + static_cast<double>(finish - start) / 2e6);
    }
    return {{"source_start_ns", start > 0 ? Json(std::to_string(start)) : Json{}},
            {"source_finish_ns", finish > 0 ? Json(std::to_string(finish)) : Json{}},
            {"usable", evaluated.usable()},
            {"reason", static_cast<unsigned>(evaluated.reason)},
            {"reason_name", reasonName(evaluated.reason)},
            {"qualified_age_ms", evaluated.usable() ? Json(evaluated.ageMs) : Json{}},
            {"mapped_age_ms_unqualified", std::move(mappedAge)},
            {"uncertainty_ms", std::move(uncertainty)}};
}
Json observation(const control::NativeObservation *value, const std::optional<control::SourceClockEstimate> &clock,
                 std::uint64_t now)
{
    Json fields = Json::object();
    if (value)
        for (const auto &[id, source] : value->fields)
        {
            auto timing = sample(source.startNs, source.finishNs, clock, now);
            timing["capture"] = std::to_string(source.capture);
            fields[std::to_string(id)] = std::move(timing);
        }
    return {{"present", value != nullptr},
            {"current", value && value->current},
            {"status", value ? Json(value->status) : Json{}},
            {"capture", value ? Json(std::to_string(value->capture)) : Json{}},
            {"fields", std::move(fields)}};
}
Json frameMetadata(const std::optional<control::Frame> &frame, std::uint64_t observed, std::uint64_t now)
{
    if (!frame)
        return nullptr;
    const auto &h = frame->header;
    return {{"current", h.current},
            {"reader_instance", h.reader},
            {"clock_domain", h.clockDomain},
            {"session", std::to_string(h.session)},
            {"epoch", std::to_string(h.epoch)},
            {"capture", std::to_string(h.capture)},
            {"mapping_revision", std::to_string(h.mappingRevision)},
            {"consumer_connection", std::to_string(h.consumerConnection)},
            {"observed_at_ms", std::to_string(observed)},
            {"observed_age_ms", observed && now >= observed ? Json(now - observed) : Json{}},
            {"characters", frame->characters.size()},
            {"clock", clockMetadata(h.sourceClock, now)},
            {"frame_interval", sample(h.sourceStartNs, h.sourceFinishNs, h.sourceClock, now)}};
}
} // namespace
Json describeTimingMetadata(const Snapshot &s, const control::LiveDiagnostic &d, std::uint64_t now)
{
    const auto view = s.view ? &*s.view : nullptr;
    const auto clock = view ? view->header.sourceClock : std::nullopt;
    const auto interval = [&](std::int64_t start, std::int64_t finish) { return sample(start, finish, clock, now); };
    const auto controller = view && view->controllerRotation ? &*view->controllerRotation : nullptr;
    const auto weapon = view && view->activeWeapon ? &*view->activeWeapon : nullptr;
    const auto recoil = weapon && weapon->recoilTargets ? &*weapon->recoilTargets : nullptr;
    const auto basis = view && view->nativeAimBasis ? &*view->nativeAimBasis : nullptr;
    const auto sway = view && view->nativeSway ? &*view->nativeSway : nullptr;
    const auto holding = view && view->nativeHolding ? &*view->nativeHolding : nullptr;
    const bool feedbackCurrent =
        s.controlActive && s.motionFeedback && !(basis && basis->branch == 2 && s.motionFeedback->nativeReference);
    Json target;
    if (d.target)
    {
        const auto &t = *d.target;
        target = {{"clock", clockMetadata(t.sourceClock, now)},
                  {"position", sample(t.sourceStartNs, t.sourceFinishNs, t.sourceClock, now)},
                  {"velocity", t.bodyWorldVelocity ? sample(t.bodyWorldVelocity->sourceStartNs,
                                                            t.bodyWorldVelocity->sourceFinishNs, t.sourceClock, now)
                                                   : Json{}}};
    }
    const control::PredictionPolicy policy;
    constexpr std::array parts{"none", "target", "camera"};
    return {
        {"schema_version", 1},
        {"now_ms", std::to_string(now)},
        {"inspection", "fresh_readonly_inspection"},
        {"control_active", s.controlActive},
        {"held_target_present", s.heldTarget.has_value()},
        {"actual_feedback_reference",
         feedbackCurrent ? Json(s.motionFeedback->nativeReference ? "native_barrel_basis" : "camera") : Json{}},
        {"live_block", static_cast<unsigned>(d.block)},
        {"ballistic_block", static_cast<unsigned>(d.ballistics.block)},
        {"timing_part", static_cast<unsigned>(d.ballistics.timingPart)},
        {"timing_part_name", parts.at(static_cast<unsigned>(d.ballistics.timingPart))},
        {"timing_reason", d.ballistics.timingReason ? Json(static_cast<unsigned>(*d.ballistics.timingReason)) : Json{}},
        {"timing_reason_name", d.ballistics.timingReason ? Json(reasonName(*d.ballistics.timingReason)) : Json{}},
        {"policy",
         {{"maximum_age_ms", policy.maximumAgeMs},
          {"maximum_uncertainty_ms", policy.maximumUncertaintyMs},
          {"maximum_target_velocity_skew_ms", policy.maximumVelocitySkewMs},
          {"maximum_living_local_skew_ms", s.motion.livingLocalSkewMs},
          {"maximum_probe_age_ms", policy.maximumProbeAgeMs}}},
        {"living", frameMetadata(s.living, s.livingObservedMs, now)},
        {"local", frameMetadata(s.view, s.viewObservedMs, now)},
        {"probe_rtt_ms",
         {{"living", s.livingRtt ? number(*s.livingRtt) : Json{}}, {"local", s.viewRtt ? number(*s.viewRtt) : Json{}}}},
        {"target", std::move(target)},
        {"rotation_sample", interval(view ? view->rotationSourceStartNs : 0, view ? view->rotationSourceFinishNs : 0)},
        {"camera_sample", interval(view ? view->cameraSourceStartNs : 0, view ? view->cameraSourceFinishNs : 0)},
        {"reader_telemetry",
         {{"controller_rotation",
           interval(controller ? controller->sourceStartNs : 0, controller ? controller->sourceFinishNs : 0)},
          {"recoil_targets", interval(recoil ? recoil->sourceStartNs : 0, recoil ? recoil->sourceFinishNs : 0)},
          {"aim_basis", observation(basis ? &basis->observation : nullptr, clock, now)},
          {"native_sway", observation(sway ? &sway->observation : nullptr, clock, now)},
          {"native_holding", observation(holding ? &holding->observation : nullptr, clock, now)}}}};
}
} // namespace mousebox::application
