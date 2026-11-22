#include "comparisons.hpp"
#include "timing.hpp"
#include "angles.hpp"
namespace mousebox::games::cs2::pipeline
{
void applyComparison(AimSolution &r, const FrameContext &c, const AimSettings &s, const PipelinePolicy &policy)
{
    const auto &f = *c.frame;
    const auto &self = *c.self;
    const double horizon = predictionHorizon(c.timing.ageMs, s.responseMs, policy.time);
    const auto reference = shotReference(f, self, s, horizon, policy);
    if (reference.block != "ready")
    {
        r.block = reference.block;
        return;
    }
    r.horizonMs = horizon;
    r.recoilHorizonMs = reference.recoilHorizonMs;
    r.direction = reference.direction;
    r.recoilOffset = reference.recoil;
    r.directionReference = reference.kind;
    if (!r.ready() || !r.target || !r.sourcePoint)
        return;
    // A comparison always uses the v1-qualified, v1-selected current target.
    // Turning a component off cannot change which enemy won selection.
    for (const auto &p : f.players)
        if (p.controller == r.target->controller && p.body && p.body->entity.id == r.target->pawn)
        {
            const auto g =
                predictGeometry(*r.sourcePoint, *f.local.eye, p.body->entity.velocity.value_or(Vec3{}),
                                self.entity.velocity.value_or(Vec3{}), horizon, s.prediction, policy.geometry);
            r.point = g.point;
            r.origin = g.origin;
            r.desired = g.desired;
            r.error = angularError(g.desired, *r.direction);
            return;
        }
    r.block = "held-target-missing";
}
} // namespace mousebox::games::cs2::pipeline
