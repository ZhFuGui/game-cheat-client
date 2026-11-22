#include "frame_context.hpp"
#include "angles.hpp"
namespace mousebox::games::cs2
{
bool alive(const Player &p, const Body &b) noexcept
{
    return p.alive != false && b.entity.health && *b.entity.health > 0 && b.entity.lifeState == 0 &&
           b.entity.dormant == false;
}
const Body *localBody(const Snapshot &f) noexcept
{
    if (!f.local.controller || !f.local.pawn || f.local.cameraRecoilPath != CameraRecoilPath::Player ||
        f.local.cameraRecoilPawn != f.local.pawn)
        return nullptr;
    for (const auto &p : f.players)
        if (p.controller == *f.local.controller && p.local == true && p.body && p.body->entity.id == *f.local.pawn &&
            alive(p, *p.body))
            return &*p.body;
    return nullptr;
}
bool nativeMouse(const MouseMapping &m) noexcept
{
    return m.source == NativeInputSource::Mouse && m.yawDegreesPerDelta && m.pitchDegreesPerDelta &&
           std::isfinite(*m.yawDegreesPerDelta) && std::isfinite(*m.pitchDegreesPerDelta) &&
           std::abs(*m.yawDegreesPerDelta) >= 1e-7 && std::abs(*m.pitchDegreesPerDelta) >= 1e-7;
}
pipeline::FrameContext pipeline::qualifyFrame(const ControlInput &in, const AimSettings &s, std::uint64_t now,
                                               FrameUse use)
{
    FrameContext c;
    if (!in.current || !in.frame)
        return c;
    c.frame = in.frame.get();
    const auto &f = *c.frame;
    if (!f.directoryVerified)
    {
        c.block = "source-unverified";
        return c;
    }
    c.self = localBody(f);
    if (!c.self)
    {
        c.block = "local-not-playing";
        return c;
    }
    if (use == FrameUse::TargetGeometry &&
        (!f.local.eye || !finite(*f.local.eye) || !f.local.angles || !finite(*f.local.angles)))
    {
        c.block = "eye-missing";
        return c;
    }
    if (!f.local.mouseMapping || !nativeMouse(*f.local.mouseMapping))
    {
        c.block = "mouse-mapping-missing";
        return c;
    }
    if (!in.sample || !in.sample->current || in.sample->reader != f.instance ||
        in.sample->clockDomain != f.clockDomain || in.sample->epoch != f.epoch || in.sample->capture != f.sequence)
    {
        c.block = "clock-unavailable";
        return c;
    }
    control::PredictionPolicy p;
    p.maximumAgeMs = s.maximumAgeMs;
    c.timing = control::estimateSampleTime(in.sample->sourceStartNs, in.sample->sourceFinishNs, in.sample->sourceClock,
                                           now, p);
    c.block = c.timing.usable() ? "ready" : "clock-unavailable";
    return c;
}
} // namespace mousebox::games::cs2
