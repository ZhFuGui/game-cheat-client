#pragma once
#include "games/pubg/pipeline/eligibility.hpp"
#include "games/pubg/model/aim_part.hpp"
#include "games/pubg/model/timing_settings.hpp"
#include <string_view>
namespace mousebox::control
{
enum class TargetPriority
{
    NearestDistance,
    ClosestToCrosshair
};
bool validTargetPriority(TargetPriority) noexcept;
std::string_view targetPriorityKey(TargetPriority) noexcept;
inline constexpr double DefaultMaximumAimDistanceMeters = 200, MaximumAimDistanceSettingMeters = 5000,
                        WorldUnitsPerMeter = 100;
bool validAimDistance(double meters) noexcept;
struct TargetPolicy
{
    double maximumConeDegrees = 15;
    bool requireTeam = true;
    AimPart part = AimPart::Head;
    std::optional<Identity> requiredIdentity;
    double maximumDistanceMeters = DefaultMaximumAimDistanceMeters;
    TargetPriority priority = TargetPriority::NearestDistance;
    double livingLocalSkewMs = DefaultLivingLocalSkewMs;
};
struct SelectionStats
{
    unsigned self = 0, sameTeam = 0, missingTeam = 0, missingAimPoint = 0, outsideCone = 0, invalidGeometry = 0,
             candidates = 0, outsideDistance = 0;
};
class TargetSelector
{
    SelectionStats stats_;

  public:
    const SelectionStats &stats() const noexcept
    {
        return stats_;
    }
    std::optional<Target> select(const Frame &, const Frame &, PairVerdict, TargetPolicy = {}, std::uint64_t nowMs = 0);
    void clear() noexcept
    {
        stats_ = {};
    }
};
} // namespace mousebox::control
