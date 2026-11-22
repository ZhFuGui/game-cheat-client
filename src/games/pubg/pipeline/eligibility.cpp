#include "games/pubg/pipeline/eligibility.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace mousebox::control
{
namespace
{
void policyValid(UsePolicy p)
{
    if (!std::isfinite(p.maximumSourceSkewMs) || p.maximumSourceSkewMs < 0 || !std::isfinite(p.maximumAgeUpperMs) ||
        p.maximumAgeUpperMs <= 0)
        throw std::invalid_argument("Invalid use policy");
}
} // namespace
PairVerdict checkPair(const Frame &l, const Frame &v, UsePolicy p)
{
    policyValid(p);
    const auto &a = l.header;
    const auto &b = v.header;
    const auto fail = [](Reason r) { return PairVerdict{r, 0}; };
    if (!a.current || !b.current || a.category != Category::Living || b.category != Category::Local)
        return fail(Reason::NotCurrent);
    if (a.reader.empty() || a.reader != b.reader || a.contract != b.contract)
        return fail(Reason::ReaderMismatch);
    if (!a.epoch || a.epoch != b.epoch)
        return fail(Reason::EpochMismatch);
    if (!a.world || !b.world)
        return fail(Reason::WorldMissing);
    if (a.world != b.world)
        return fail(Reason::WorldMismatch);
    if (!a.level || !b.level)
        return fail(Reason::LevelMissing);
    if (a.level != b.level)
        return fail(Reason::LevelMismatch);
    if (a.convention.empty() || b.convention.empty())
        return fail(Reason::CoordinatesMissing);
    if (a.convention != "UNREAL_LEFT_HANDED_Z_UP" || a.convention != b.convention)
        return fail(Reason::CoordinatesMismatch);
    if (a.clockDomain.empty() || a.clockDomain != b.clockDomain || !a.mappingRevision ||
        a.mappingRevision != b.mappingRevision || a.sourceFinishNs < a.sourceStartNs ||
        b.sourceFinishNs < b.sourceStartNs)
        return fail(Reason::ClockMismatch);
    // Compare only intervals in this one Reader clock domain. No consumer-now subtraction.
    const long double gap = std::max(std::abs(static_cast<long double>(a.sourceStartNs) - b.sourceStartNs),
                                     std::abs(static_cast<long double>(a.sourceFinishNs) - b.sourceFinishNs));
    const double skew = static_cast<double>(gap / 1000000);
    if (skew > p.maximumSourceSkewMs)
        return {Reason::SourceSkew, skew};
    if (!l.self)
        return {Reason::LocalIdentityMissing, skew};
    if (!l.controller || !v.controller || l.controller != v.controller)
        return {Reason::ControllerMismatch, skew};
    if (!v.camera || !v.cameraOrigin || !v.rotation || !v.fov || !finite(*v.camera) || !finite(*v.cameraOrigin) ||
        !finite(*v.rotation) || !std::isfinite(*v.fov) || *v.fov <= 0 || *v.fov >= 180)
        return {Reason::ViewMissing, skew};
    return {Reason::Ready, skew};
}
Reason checkMotion(const Frame &v, PairVerdict pair, const Calibration &c, std::optional<double> age, bool device,
                   UsePolicy p)
{
    policyValid(p);
    if (!pair.usable())
        return pair.reason;
    if (!validCalibration(c))
        return Reason::CalibrationMissing;
    if (!v.fov || std::abs(*v.fov - c.fov) > 0.01)
        return Reason::CalibrationMismatch;
    if (!age || !std::isfinite(*age) || *age < 0)
        return Reason::AgeUnknown;
    if (*age > p.maximumAgeUpperMs)
        return Reason::TooOld;
    if (!device)
        return Reason::DeviceUnavailable;
    return Reason::Ready;
}
const char *reasonName(Reason r) noexcept
{
    switch (r)
    {
#define R(name)                                                                                                        \
    case Reason::name:                                                                                                 \
        return #name
        R(Ready);
        R(NotCurrent);
        R(ReaderMismatch);
        R(EpochMismatch);
        R(WorldMissing);
        R(WorldMismatch);
        R(LevelMissing);
        R(LevelMismatch);
        R(CoordinatesMissing);
        R(CoordinatesMismatch);
        R(ClockMismatch);
        R(SourceSkew);
        R(LocalIdentityMissing);
        R(ControllerMismatch);
        R(ViewMissing);
        R(CalibrationMissing);
        R(CalibrationMismatch);
        R(AgeUnknown);
        R(TooOld);
        R(DeviceUnavailable);
#undef R
    }
    return "Unknown";
}
} // namespace mousebox::control
