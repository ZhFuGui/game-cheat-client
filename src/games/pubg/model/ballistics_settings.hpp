#pragma once
#include "domain/physics.hpp"
#include <array>
#include <cmath>
#include <string_view>
namespace mousebox::control
{
// User decisions, 2026-10-16. Camera origin is an explicit reference-model
// choice, not evidence of the native game's projectile spawn position.
enum class BallisticOriginRule
{
    Camera
};
enum class ShotDirectionRule
{
    SameAsAimLineBeforeZeroing
};
enum class FlightTimeRule
{
    PublishedReciprocalSpeedTrapezoid
};
struct BallisticsAssumptions
{
    std::string_view revision = "user-model-20261016-v4-configurable-gravity";
    BallisticOriginRule origin = BallisticOriginRule::Camera;
    ShotDirectionRule direction = ShotDirectionRule::SameAsAimLineBeforeZeroing;
    FlightTimeRule flightTime = FlightTimeRule::PublishedReciprocalSpeedTrapezoid;
    double inheritedShooterVelocity = 0;
    double defaultGravityMetersPerSecondSquared = DefaultGravityMetersPerSecondSquared;
    bool liveBallisticsIntegrated = true;
};
inline constexpr BallisticsAssumptions CurrentBallisticsAssumptions{};
inline constexpr unsigned DefaultZeroingDistanceMeters = 100;
inline constexpr std::array<unsigned, 5> ZeroingDistancePresetsMeters{100, 200, 300, 400, 500};
constexpr bool validZeroingDistance(unsigned meters) noexcept
{
    for (const auto value : ZeroingDistancePresetsMeters)
        if (value == meters)
            return true;
    return false;
}
} // namespace mousebox::control
