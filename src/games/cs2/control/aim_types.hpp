#pragma once
#include "settings.hpp"
#include "games/cs2/model/target_key.hpp"
#include "control/sample_time.hpp"
#include "domain/source_sample.hpp"
#include <functional>
namespace mousebox::games::cs2
{
struct ControlInput
{
    std::shared_ptr<const Snapshot> frame;
    std::optional<domain::SourceSample> sample;
    std::uint64_t revision = 0;
    bool requested = false, current = false;
    // Clock captured after acquiring this immutable observation, not before a
    // concurrently published frame/clock can become newer than the caller.
    std::uint64_t observedAtMs = 0;
    bool weaponAccuracyEvaluation = false;
};
// A target policy may supply a current point without replacing the shared
// reference, timing, prediction, comparison, feedback or motion pipeline.
struct TargetPoint
{
    TargetKey target;
    Vec3 point;
    unsigned id = 0;
};
struct AimCandidatePoint
{
    Vec3 position;
    std::optional<unsigned> hitboxId;
    std::optional<std::uint64_t> sampleSeed;
};
struct AimPointResolution
{
    std::optional<AimCandidatePoint> candidate;
    std::string block;
};
// Empty means the point is outside the normal range, cone or initial screen bounds.
using AimPointProbe = std::function<std::string(Vec3)>;
using AimPointResolver =
    std::function<AimPointResolution(const Body &, TargetKey, unsigned part, const AimPointProbe &)>;
using AimPointCheck = std::function<std::string(Vec3 origin, Vec3 point)>;
struct AimSolution
{
    std::string block = "waiting-source", targetName, directionReference = "native-basis";
    bool localReady = false, inputControllable = false;
    std::optional<TargetKey> target;
    std::optional<unsigned> pointId;
    std::optional<std::uint64_t> pointSeed;
    std::optional<Vec3> point, origin, sourcePoint;
    std::optional<Angles> desired, direction, error;
    std::optional<Angles> recoilOffset;
    double distanceUnits = 0, horizonMs = 0, recoilHorizonMs = 0;
    control::SampleTimeEstimate timing;
    bool ready() const noexcept
    {
        return block == "ready";
    }
};
} // namespace mousebox::games::cs2
