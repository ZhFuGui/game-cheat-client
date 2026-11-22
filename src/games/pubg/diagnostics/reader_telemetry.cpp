#include "reader_telemetry.hpp"
#include "games/pubg/pipeline/latency_prediction.hpp"
#include "games/pubg/pipeline/native_aim.hpp"
#include "games/pubg/model/native_aim_profiles.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace mousebox::application
{
namespace
{
Json identity(const control::Identity &value)
{
    if (value.kind != control::IdentityKind::NativeLifecycle)
        return {{"id", std::to_string(value.id)},
                {"identity_kind", static_cast<unsigned>(value.kind)},
                {"token", std::to_string(value.token)},
                {"actor", Json{}},
                {"index", Json{}},
                {"serial", Json{}}};
    return {{"id", std::to_string(value.id)},
            {"actor", std::to_string(value.actor)},
            {"index", value.index},
            {"serial", value.serial}};
}
Json timing(std::int64_t start, std::int64_t finish, const control::Frame *frame, std::uint64_t now)
{
    const auto t = control::estimateSampleTime(start, finish, frame ? frame->header.sourceClock : std::nullopt, now);
    return {{"source_start_ns", start > 0 ? Json(std::to_string(start)) : Json{}},
            {"source_finish_ns", finish > 0 ? Json(std::to_string(finish)) : Json{}},
            {"timing_usable", t.usable()},
            {"timing_reason", static_cast<int>(t.reason)},
            {"age_ms", t.usable() ? Json(t.ageMs) : Json{}},
            {"uncertainty_ms", t.usable() ? Json(t.uncertaintyMs) : Json{}}};
}
Json vector(control::Vec3 value)
{
    return Json::array({value.x, value.y, value.z});
}
bool same(const control::Identity &a, const control::Identity &b)
{
    return control::sameLifecycle(a, b);
}
const char *status(unsigned value)
{
    switch (value)
    {
    case 0:
        return "available";
    case 1:
        return "partial";
    case 2:
        return "read_failed";
    case 3:
        return "dependency_blocked";
    case 4:
        return "not_attempted";
    case 5:
        return "not_implemented";
    default:
        return "invalid";
    }
}
Json observation(const control::NativeObservation *value, const control::Frame *frame, std::uint64_t now)
{
    Json result{{"provided", value != nullptr},
                {"current", value && value->current},
                {"status", value ? Json(status(value->status)) : Json{}},
                {"capture", value ? Json(std::to_string(value->capture)) : Json{}},
                {"field_sources", Json::object()},
                {"verified_start_ns",
                 value && value->verifiedStartNs > 0 ? Json(std::to_string(value->verifiedStartNs)) : Json{}},
                {"verified_finish_ns",
                 value && value->verifiedFinishNs > 0 ? Json(std::to_string(value->verifiedFinishNs)) : Json{}}};
    result["source_contract"] =
        value ? Json(value->contract == control::FrameContract::ConsumerV3 ? "consumer_v3" : "native_proof") : Json{};
    result["source_start_ns"] = value && value->sourceStartNs > 0 ? Json(std::to_string(value->sourceStartNs)) : Json{};
    result["source_finish_ns"] =
        value && value->sourceFinishNs > 0 ? Json(std::to_string(value->sourceFinishNs)) : Json{};
    if (value)
        for (const auto &[id, source] : value->fields)
        {
            auto field = timing(source.startNs, source.finishNs, frame, now);
            field["capture"] = std::to_string(source.capture);
            field["origin"] = source.origin ? Json(source.origin) : Json{};
            result["field_sources"][std::to_string(id)] = std::move(field);
        }
    return result;
}
Json sampleTiming(const control::NativeObservation *value, unsigned field, const control::Frame *frame,
                  std::uint64_t now)
{
    if (value)
        if (const auto found = value->fields.find(field); found != value->fields.end())
            return timing(found->second.startNs, found->second.finishNs, frame, now);
    return timing(0, 0, frame, now);
}
} // namespace
Json describeReaderTelemetry(const std::optional<control::Frame> &view, std::uint64_t now, bool nativeControlActive)
{
    const auto frame = view ? &*view : nullptr;
    const auto rotation = frame && frame->controllerRotation ? &*frame->controllerRotation : nullptr;
    const auto weapon = frame && frame->activeWeapon ? &*frame->activeWeapon : nullptr;
    const auto targets = weapon && weapon->recoilTargets ? &*weapon->recoilTargets : nullptr;
    const auto generation = weapon && weapon->trajectoryGeneration ? &*weapon->trajectoryGeneration : nullptr;
    const auto camera = frame && frame->rotation ? &*frame->rotation : nullptr;
    const auto holding = frame && frame->nativeHolding ? &*frame->nativeHolding : nullptr;
    const auto sway = frame && frame->nativeSway ? &*frame->nativeSway : nullptr;
    const auto basis = frame && frame->nativeAimBasis ? &*frame->nativeAimBasis : nullptr;
    const auto item = frame && frame->currentItem ? &*frame->currentItem : nullptr;
    auto ci = observation(item ? &item->observation : nullptr, frame, now);
    ci["available"] = item && item->hasCurrentItem.has_value();
    ci["has_current_item"] = item && item->hasCurrentItem ? Json(*item->hasCurrentItem) : Json{};
    ci["character"] = item && item->character ? identity(*item->character) : Json{};
    ci["presence_sample"] = sampleTiming(item ? &item->observation : nullptr, 7, frame, now);
    Json c = timing(frame ? frame->rotationSourceStartNs : 0, frame ? frame->rotationSourceFinishNs : 0, frame, now);
    c["available"] = camera != nullptr;
    c["pitch_degrees"] = camera ? Json(camera->x) : Json{};
    c["yaw_degrees"] = camera ? Json(camera->y) : Json{};
    c["roll_degrees"] = camera ? Json(camera->z) : Json{};
    c["existing_feedback_source"] = true;
    Json r = timing(rotation ? rotation->sourceStartNs : 0, rotation ? rotation->sourceFinishNs : 0, frame, now);
    r["available"] = rotation != nullptr;
    r["controller"] = rotation ? identity(rotation->controller) : Json{};
    r["pitch_degrees"] = rotation ? Json(rotation->rotation.x) : Json{};
    r["yaw_degrees"] = rotation ? Json(rotation->rotation.y) : Json{};
    r["roll_degrees"] = rotation ? Json(rotation->rotation.z) : Json{};
    Json t = timing(targets ? targets->sourceStartNs : 0, targets ? targets->sourceFinishNs : 0, frame, now);
    t["available"] = targets != nullptr;
    t["target_pitch"] = targets && targets->pitch ? Json(*targets->pitch) : Json{};
    t["target_yaw"] = targets && targets->yaw ? Json(*targets->yaw) : Json{};
    t["recovery_target"] = targets && targets->recovery ? Json(*targets->recovery) : Json{};
    t["units"] = "native_remaining_input";
    t["atomic_rotation"] = false;
    Json g =
        timing(generation ? generation->sourceStartNs : 0, generation ? generation->sourceFinishNs : 0, frame, now);
    g["available"] = generation != nullptr;
    g["counter"] = generation ? Json(generation->counter) : Json{};
    // Deliberately expose only the original uint32, without polling-derived
    // events, shot counts, counter/time rates or a synthetic firing boolean.
    g["derived_events_enabled"] = false;
    auto h = observation(holding ? &holding->observation : nullptr, frame, now);
    h["available"] = holding && (holding->accepted.has_value() || holding->blend.has_value());
    h["character"] = holding && holding->character ? identity(*holding->character) : Json{};
    h["component"] = holding && holding->component ? identity(*holding->component) : Json{};
    h["accepted_holding"] = holding && holding->accepted ? Json(*holding->accepted) : Json{};
    h["holding_blend"] = holding && holding->blend ? Json(*holding->blend) : Json{};
    h["accepted_sample"] = sampleTiming(holding ? &holding->observation : nullptr, 3, frame, now);
    h["blend_sample"] = sampleTiming(holding ? &holding->observation : nullptr, 4, frame, now);
    auto sw = observation(sway ? &sway->observation : nullptr, frame, now);
    sw["available"] = sway && sway->rotation.has_value();
    sw["character"] = sway && sway->character ? identity(*sway->character) : Json{};
    sw["weapon"] = sway && sway->weapon ? identity(*sway->weapon) : Json{};
    sw["rotation_degrees"] = sway && sway->rotation ? vector(*sway->rotation) : Json{};
    sw["rotation_sample"] = sampleTiming(sway ? &sway->observation : nullptr, 3, frame, now);
    sw["pure_breath"] = false;
    sw["added_to_basis"] = false;
    auto a = observation(basis ? &basis->observation : nullptr, frame, now);
    a["available"] = basis && basis->direction.has_value();
    a["character"] = basis && basis->character ? identity(*basis->character) : Json{};
    a["weapon"] = basis && basis->weapon ? identity(*basis->weapon) : Json{};
    a["mesh"] = basis && basis->mesh ? identity(*basis->mesh) : Json{};
    a["gun_data"] = basis && basis->gunData ? identity(*basis->gunData) : Json{};
    a["trajectory_config"] = basis && basis->trajectoryConfig ? identity(*basis->trajectoryConfig) : Json{};
    a["direction"] = basis && basis->direction ? vector(*basis->direction) : Json{};
    a["direction_sample"] = sampleTiming(basis ? &basis->observation : nullptr, 6, frame, now);
    a["zeroing_distance_m"] = basis && basis->zeroingDistanceMeters ? Json(*basis->zeroingDistanceMeters) : Json{};
    a["zeroing_correction_degrees"] =
        basis && basis->zeroingCorrectionDegrees ? Json(*basis->zeroingCorrectionDegrees) : Json{};
    a["integrated_steps"] = basis && basis->integratedSteps ? Json(*basis->integratedSteps) : Json{};
    a["zeroing_reached_distance"] = basis && basis->reachedDistance ? Json(*basis->reachedDistance) : Json{};
    a["integrated_distance_cm"] = basis && basis->integratedDistanceCm ? Json(*basis->integratedDistanceCm) : Json{};
    a["branch"] = basis && basis->branch ? Json(basis->branch) : Json{};
    a["stage"] = basis && basis->stage ? Json(basis->stage) : Json{};
    a["native_zeroing_applied"] = basis && basis->direction ? Json(basis->nativeZeroingApplied) : Json{};
    a["native_profile"] = basis && !basis->nativeProfile.empty() ? Json(basis->nativeProfile) : Json{};
    a["branch_name"] = basis && basis->branch == 2   ? "view_no_trace"
                       : basis && basis->branch == 1 ? "barrel"
                                                     : "unknown";
    a["observation_only_branch"] = basis && basis->branch == 2;
    const bool basisUsed = nativeControlActive && !(basis && basis->branch == 2);
    a["has_roll"] = false;
    a["input_mapping_verified"] = false;
    a["input_mapping_model"] = control::NativeInputModel;
    a["used_for_control"] = basisUsed;
    a["native_context_token"] =
        basis && basis->nativeContextToken ? Json(std::to_string(*basis->nativeContextToken)) : Json{};
    a["controller_token"] = basis && basis->controllerToken ? Json(std::to_string(*basis->controllerToken)) : Json{};
    return {{"descriptor_sha256",
             frame && !frame->header.descriptorSha256.empty() ? Json(frame->header.descriptorSha256) : Json{}},
            {"used_for_control", basisUsed},
            {"reader", frame ? Json(frame->header.reader) : Json{}},
            {"session", frame ? Json(std::to_string(frame->header.session)) : Json{}},
            {"epoch", frame ? Json(std::to_string(frame->header.epoch)) : Json{}},
            {"capture", frame ? Json(std::to_string(frame->header.capture)) : Json{}},
            {"body", frame && frame->self ? identity(*frame->self) : Json{}},
            {"weapon", weapon ? identity(weapon->instance) : Json{}},
            {"camera_rotation", std::move(c)},
            {"controller_rotation", std::move(r)},
            {"recoil_targets", std::move(t)},
            {"trajectory_generation", std::move(g)},
            {"current_item", std::move(ci)},
            {"native_holding", std::move(h)},
            {"native_sway", std::move(sw)},
            {"aim_basis", std::move(a)},
            {"actual_firing", Json{}},
            {"applied_recoil_degrees", Json{}},
            {"pure_breath_degrees", Json{}},
            {"verified_pre_spread_direction", basis && basis->direction ? vector(*basis->direction) : Json{}}};
}
Json describeNativeAimComparison(const std::optional<control::Frame> &view,
                                 const control::BallisticDiagnostic &ballistic, std::uint64_t now)
{
    Json result{{"available", false},
                {"reason", "basis_missing"},
                {"observed_launch_separation_degrees", Json{}},
                {"basis_stage", "post_branch_pre_spread"},
                {"target_stage", "launch_before_random_spread"},
                {"time_aligned", false},
                {"angular_extrapolation_applied", false},
                {"used_for_control", false},
                {"input_mapping_verified", false},
                {"zeroing_subtracted_again", false},
                {"sway_added", false},
                {"recoil_targets_added", false}};
    if (!view || !view->nativeAimBasis || !view->nativeAimBasis->direction)
        return result;
    const auto &basis = *view->nativeAimBasis;
    const auto reject = [&](const char *reason) {
        result["reason"] = reason;
        return result;
    };
    if (!view->header.current || !view->header.world || !view->header.level ||
        view->header.convention != "UNREAL_LEFT_HANDED_Z_UP")
        return reject("context_unavailable");
    if (!basis.observation.current || !view->self || !basis.character || !same(*view->self, *basis.character) ||
        !view->activeWeapon || !basis.weapon || !same(view->activeWeapon->instance, *basis.weapon))
        return reject("current_identity_unavailable");
    if (basis.branch != 1 || basis.stage != 1 || !basis.nativeZeroingApplied ||
        !control::knownBarrelAimProfile(basis.nativeProfile))
        return reject("profile_or_stage_unsupported");
    for (const auto field : control::nativeAimRequiredFields(view->header.contract))
    {
        const auto found = basis.observation.fields.find(field);
        if (found == basis.observation.fields.end() || found->second.capture != view->header.capture ||
            !control::estimateSampleTime(found->second.startNs, found->second.finishNs, view->header.sourceClock, now)
                 .usable())
            return reject("basis_source_timing_unavailable");
    }
    if (ballistic.block != control::BallisticBlock::Ready || !ballistic.weapon ||
        !same(*ballistic.weapon, *basis.weapon))
        return reject("current_ballistic_launch_unavailable");
    const auto a = *basis.direction, d = ballistic.solution.launchDirection;
    const double length = std::hypot(a.x, a.y, a.z) * std::hypot(d.x, d.y, d.z);
    if (!control::finite(a) || !control::finite(d) || !std::isfinite(length) || length < 1e-9)
        return reject("invalid_direction");
    result["observed_launch_separation_degrees"] =
        std::acos(std::clamp((a.x * d.x + a.y * d.y + a.z * d.z) / length, -1.0, 1.0)) * 180 / std::numbers::pi;
    result["available"] = true;
    result["reason"] = "observation_only_input_mapping_unverified";
    return result;
}
} // namespace mousebox::application
