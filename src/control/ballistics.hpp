#pragma once
#include "domain/math.hpp"
#include "domain/physics.hpp"
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mousebox::control
{
// SI units. This is the user's reference model, not a recovered game integrator.
struct BallisticProfile
{
    std::string id, name, packRevision, source;
    std::vector<std::string> actorClasses, itemIds;
    double initialSpeed = 0;
    std::optional<double> maximumTravel;
    std::vector<std::pair<double, double>> velocityKeys;
    std::vector<std::string> notes;
};
class FlightTimeCurve
{
    std::vector<std::pair<double, double>> keys_;
    std::vector<double> times_;
    double initial_ = 0;
    bool valid_ = false, curve_ = false;

  public:
    explicit FlightTimeCurve(const BallisticProfile &);
    bool valid() const noexcept
    {
        return valid_;
    }
    bool usesCurve() const noexcept
    {
        return curve_;
    }
    double initialSpeed() const noexcept
    {
        return keys_.empty() ? initial_ : keys_.front().second;
    }
    std::optional<double> timeAt(double distance) const noexcept;
};
enum class BallisticStatus
{
    Ready,
    InvalidInput,
    InvalidProfile,
    NoIntercept,
    ZeroingUnreachable,
    ResidualRejected
};
enum class ZeroingRule
{
    None,
    EquivalentAngle
};
struct BallisticRequest
{
    Vec3 origin, camera, target, targetVelocity, sightDirection{1, 0, 0};
    double zeroingMeters = 100, gravity = DefaultGravityMetersPerSecondSquared;
    ZeroingRule zeroing = ZeroingRule::EquivalentAngle;
};
struct BallisticSolution
{
    BallisticStatus status = BallisticStatus::InvalidInput;
    double flightSeconds = 0, travelMeters = 0, dropMeters = 0, zeroingSeconds = 0, zeroingPitchDegrees = 0,
           residualMeters = 0;
    Vec3 launchDirection, aimDirection, impactPoint, targetAtImpact, zeroingDirection;
    bool usedCurve = false;
    bool ready() const noexcept
    {
        return status == BallisticStatus::Ready;
    }
};
// Curve progress s is distance along the initial launch direction. Gravity adds
// G*t^2/2 independently; the public calculator does not define a 3-D drag law.
// The earliest bracketed low-time solution is validated by forward substitution.
BallisticSolution solveBallistic(const BallisticProfile &, const BallisticRequest &);
BallisticSolution solveBallistic(const BallisticProfile &, const FlightTimeCurve &, const BallisticRequest &) noexcept;

struct CompiledBallisticProfile
{
    BallisticProfile profile;
    FlightTimeCurve curve;
    explicit CompiledBallisticProfile(BallisticProfile value) : profile(std::move(value)), curve(profile) {}
};
enum class ProfileMatchStatus
{
    Matched,
    Unrecognized,
    Ambiguous,
    Unsupported
};
struct ProfileMatch
{
    ProfileMatchStatus status = ProfileMatchStatus::Unrecognized;
    const CompiledBallisticProfile *entry = nullptr;
};
struct BallisticCatalog
{
    std::string revision, sourcePath, sourceBuild;
    std::vector<CompiledBallisticProfile> entries;
    ProfileMatch find(const std::string &actorClass, const std::string &itemId = {}) const noexcept;
};
} // namespace mousebox::control
