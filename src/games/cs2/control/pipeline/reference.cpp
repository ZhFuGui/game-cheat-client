#include "reference.hpp"
#include "games/cs2/evaluation/recoil.hpp"
namespace mousebox::games::cs2::pipeline
{
Reference shotReference(const Snapshot &f, const Body &self, const AimSettings &s, double horizon,
                        const PipelinePolicy &p)
{
    Reference r;
    r.kind = p.direction == DirectionReference::GlobalInput ? "global-input" : "native-basis";
    if (p.nativeQualification)
    {
        const auto full = evaluateRecoil(f);
        if (!full.contextDirection || !self.eyeBasis || !self.eyeBasis->value)
        {
            r.block = "eye-context-missing";
            return r;
        }
    }
    else if (!self.aimPunch || !evaluateShotOffset(*self.aimPunch, 0))
    {
        r.block = "recoil-unavailable";
        return r;
    }
    // Every comparison is qualified with the full unmodified current groups.
    // Only the selected numeric contribution changes, never data availability.
    auto punch = *self.aimPunch;
    if (!p.recoil.predictable || !p.recoil.baseAngle)
        punch.predictable->baseAngle = Angles{};
    if (!p.recoil.predictable || !p.recoil.baseVelocity)
        punch.predictable->baseAngularVelocity = Angles{};
    if (!p.recoil.unpredictable)
        punch.unpredictable->baseAngle = Angles{};
    r.recoilHorizonMs = s.recoil && p.time.recoilLead ? horizon : 0;
    r.recoil = s.recoil ? evaluateShotOffset(punch, r.recoilHorizonMs / 1000) : std::optional(Angles{});
    if (!r.recoil)
    {
        r.block = "recoil-unavailable";
        return r;
    }
    const auto basis = p.direction == DirectionReference::GlobalInput ? *f.local.angles : *self.eyeBasis->value;
    r.direction = Angles{basis.x + r.recoil->x, basis.y + r.recoil->y, basis.z + r.recoil->z};
    r.inputControllable = !p.nativeQualification || static_cast<unsigned>(self.eyeBasis->source) <= 4;
    return r;
}
ResponseInput responseInput(const Snapshot &f, const PipelinePolicy &p)
{
    if (p.globalResponse)
        return {*f.local.angles, "global-input"};
    const auto self = localBody(f);
    if (self && self->eyeBasis && self->eyeBasis->source == EyeAngleSource::PredictionInput && self->eyeBasis->value)
        return {*self->eyeBasis->value, "pawn-prediction-input"};
    if (self && self->inputEyeAngles)
        return {*self->inputEyeAngles, "pawn-input"};
    return {*f.local.angles, "global-input"};
}
} // namespace mousebox::games::cs2::pipeline
