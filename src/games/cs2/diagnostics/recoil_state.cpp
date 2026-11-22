#include "recoil_state.hpp"
#include "games/cs2/evaluation/recoil.hpp"
namespace mousebox::games::cs2
{
namespace
{
using application::Json;
template <typename T> Json number(const std::optional<T> &v)
{
    return v ? Json(*v) : Json{};
}
Json vector(const std::optional<Angles> &v)
{
    return v ? Json::array({v->x, v->y, v->z}) : Json{};
}
Json time(const std::optional<GameTime> &v)
{
    return v ? Json{{"tick", number(v->tick)}, {"fraction", number(v->fraction)}} : Json{};
}
Json eye(const std::optional<EyeAngleBasis> &v)
{
    return v ? Json{{"value", vector(v->value)},
                    {"source", static_cast<unsigned>(v->source)},
                    {"nativeContext", number(v->nativeContext)},
                    {"interpolationParameter", number(v->interpolationParameter)},
                    {"resolvedSource", v->resolvedSource ? Json(static_cast<unsigned>(*v->resolvedSource)) : Json{}},
                    {"sourceEntity", v->sourceEntity ? Json(std::to_string(*v->sourceEntity)) : Json{}},
                    {"delegationDepth", number(v->delegationDepth)}}
             : Json{};
}
Json punch(const std::optional<AimPunchState> &v)
{
    if (!v)
        return {};
    Json p, u, t;
    if (v->predictable)
        p = {{"baseTime", time(v->predictable->baseTime)},
             {"baseAngle", vector(v->predictable->baseAngle)},
             {"baseAngularVelocity", vector(v->predictable->baseAngularVelocity)}};
    if (v->unpredictable)
        u = {{"baseTick", number(v->unpredictable->baseTick)}, {"baseAngle", vector(v->unpredictable->baseAngle)}};
    if (v->times)
        t = {{"worldGroup", number(v->times->worldGroup)},
             {"nativeContext", number(v->times->nativeContext)},
             {"visualPolicy", static_cast<unsigned>(v->times->visualPolicy)},
             {"secondsPerTick", number(v->times->secondsPerTick)},
             {"simulation", time(v->times->simulation)},
             {"visualPredictable", time(v->times->visualPredictable)},
             {"visualUnpredictable", time(v->times->visualUnpredictable)}};
    return {{"predictable", p}, {"unpredictable", u}, {"times", t}};
}
const Body *localBody(const Snapshot &f)
{
    for (const auto &p : f.players)
        if (f.local.controller && p.controller == *f.local.controller && p.body)
            return &*p.body;
    return nullptr;
}
} // namespace
application::Json describeRecoil(const Snapshot &f)
{
    const auto evaluated = evaluateRecoil(f);
    Json mapping;
    if (f.local.mouseMapping)
    {
        const auto &m = *f.local.mouseMapping;
        mapping = {{"source", static_cast<unsigned>(m.source)},
                   {"sensitivity", number(m.sensitivity)},
                   {"verticalScale", number(m.verticalScale)},
                   {"yaw", number(m.yaw)},
                   {"pitch", number(m.pitch)},
                   {"zoomRatio", number(m.zoomRatio)},
                   {"invertedY", number(m.invertedY)},
                   {"effectivePitch", number(m.effectivePitch)},
                   {"pawnFovAdjust", number(m.pawnFovAdjust)},
                   {"pawnMouseSensitivity", number(m.pawnMouseSensitivity)},
                   {"yawDegreesPerDelta", number(m.yawDegreesPerDelta)},
                   {"pitchDegreesPerDelta", number(m.pitchDegreesPerDelta)}};
    }
    Json cameraView;
    if (f.local.viewPunch)
    {
        const auto &v = *f.local.viewPunch;
        cameraView = {{"baseAngle", vector(v.baseAngle)},
                      {"baseTime", time(v.baseTime)},
                      {"evaluationSeconds", number(v.evaluationSeconds)},
                      {"decayExponent", number(v.decayExponent)}};
    }
    const auto self = localBody(f);
    Json contextMatches;
    if (self && self->eyeBasis && self->eyeBasis->nativeContext && self->aimPunch && self->aimPunch->times &&
        self->aimPunch->times->nativeContext)
        contextMatches = self->eyeBasis->nativeContext == self->aimPunch->times->nativeContext;
    const Body *camera = nullptr;
    if (f.local.cameraRecoilPawn)
        for (const auto &p : f.players)
            if (p.body && p.body->entity.id == *f.local.cameraRecoilPawn)
                camera = &*p.body;
    return {{"buildSupported", evaluated.supportedBuild},
            {"readOnly", true},
            {"mouse", mapping},
            {"crosshairEnabled", number(f.local.crosshairRecoil)},
            {"localPunch", self ? punch(self->aimPunch) : Json{}},
            {"networkEyeAngles", self ? vector(self->networkEyeAngles) : Json{}},
            {"inputEyeAngles", self ? vector(self->inputEyeAngles) : Json{}},
            {"localEyeBasis", self ? eye(self->eyeBasis) : Json{}},
            {"cameraEyeBasis", camera ? eye(camera->eyeBasis) : Json{}},
            {"localBasisContextMatches", contextMatches},
            {"directionContextIndependent", evaluated.directionContextIndependent},
            {"cameraPawn", f.local.cameraRecoilPawn ? Json(std::to_string(*f.local.cameraRecoilPawn)) : Json{}},
            {"cameraPath", static_cast<unsigned>(f.local.cameraRecoilPath)},
            {"cameraPunch", camera ? punch(camera->aimPunch) : Json{}},
            {"cameraViewState", cameraView},
            {"evaluated",
             {{"shotPredictable", vector(evaluated.shotPredictable)},
              {"shotUnpredictable", vector(evaluated.shotUnpredictable)},
              {"shotOffset", vector(evaluated.shotOffset)},
              {"rawEyePlusRecoil", vector(evaluated.rawEyePlusRecoil)},
              {"contextDirection", vector(evaluated.contextDirection)},
              {"crosshairFollow", vector(evaluated.crosshairFollow)},
              {"cameraPredictable", vector(evaluated.cameraPredictable)},
              {"cameraUnpredictable", vector(evaluated.cameraUnpredictable)},
              {"cameraAim", vector(evaluated.cameraAim)},
              {"cameraView", vector(evaluated.cameraView)},
              {"cameraFeedback", vector(evaluated.cameraFeedback)}}}};
}
} // namespace mousebox::games::cs2
