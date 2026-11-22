#include "aim_solution.hpp"
#include "modes.hpp"
#include "pipeline/comparisons.hpp"
#include "pipeline/targets.hpp"
#include "pipeline/timing.hpp"
namespace mousebox::games::cs2
{
namespace
{
void applyReference(AimSolution &solution, const pipeline::FrameContext &context, const AimSettings &settings,
                    const PipelinePolicy &policy)
{
    solution.localReady = context.self != nullptr;
    solution.timing = context.timing;
    solution.block = context.block;
    if (!context.ready())
        return;
    const double horizon = pipeline::predictionHorizon(context.timing.ageMs, settings.responseMs);
    const auto reference = pipeline::shotReference(*context.frame, *context.self, settings, horizon, policy);
    solution.direction = reference.direction;
    solution.recoilOffset = reference.recoil;
    solution.inputControllable = reference.inputControllable;
    if (reference.block != "eye-context-missing")
        solution.horizonMs = horizon;
    solution.recoilHorizonMs = reference.recoilHorizonMs;
    solution.block = reference.block;
}

AimSolution solve(const ControlInput &input, const AimSettings &s, std::uint64_t now, std::optional<TargetKey> held,
                  std::optional<TargetPoint> point, const AimPointCheck &check, const AimPointResolver &resolvePoint)
{
    AimSolution r;
    const auto policy = pipelinePolicy({s.version, s.comparison});
    r.directionReference = policy.direction == DirectionReference::GlobalInput ? "global-input" : "native-basis";
    if (!s.targets.any())
    {
        r.block = "targets-disabled";
        return r;
    }
    const auto context = pipeline::qualifyFrame(input, s, now);
    const auto selectionPolicy = s.comparison == ComparisonFactor::None ? policy : PipelinePolicy{};
    applyReference(r, context, s, selectionPolicy);
    if (!r.ready())
        return r;
    pipeline::selectTarget(r, *context.frame, *context.self, s, held, point, check, resolvePoint);
    if (s.comparison != ComparisonFactor::None && r.inputControllable)
        pipeline::applyComparison(r, context, s, policy);
    if (check && r.ready() && s.comparison != ComparisonFactor::None)
        r.block = check(*r.origin, *r.point);
    return r;
}
} // namespace
AimSolution solveAim(const ControlInput &input, const AimSettings &s, std::uint64_t now, std::optional<TargetKey> held,
                     const AimPointCheck &check, const AimPointResolver &resolvePoint)
{
    return solve(input, s, now, held, {}, check, resolvePoint);
}
AimSolution solveAimAt(const ControlInput &input, const AimSettings &s, std::uint64_t now, const TargetPoint &point)
{
    return solve(input, s, now, point.target, point, {}, {});
}
AimSolution solveManualAimReference(const ControlInput &input, const AimSettings &settings, std::uint64_t now)
{
    AimSolution r;
    const auto context = pipeline::qualifyFrame(input, settings, now, pipeline::FrameUse::LocalInput);
    applyReference(r, context, settings, PipelinePolicy{});
    if (r.ready())
    {
        const auto &basis = *context.self->eyeBasis;
        if (basis.source != EyeAngleSource::PredictionInput || basis.interpolationParameter)
            r.block = "eye-input-transform-unverified";
    }
    return r;
}
} // namespace mousebox::games::cs2
