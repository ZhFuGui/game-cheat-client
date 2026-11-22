#pragma once
#include "application/json.hpp"
#include "control/speed_policy.hpp"
#include "control/movement/algorithm.hpp"
#include "games/cs2/model/snapshot.hpp"
#include "games/cs2/model/target_filter.hpp"
#include <string_view>
#include "policy.hpp"
#include "weapon_aim.hpp"
#include "hitbox_aim_settings.hpp"
namespace mousebox::games::cs2
{
struct CountCalibration
{
    unsigned build = 0;
    double x = 0, y = 0, yawReference = 0, pitchReference = 0;
    double responseUpperMs = 0;
    bool operator==(const CountCalibration &) const = default;
};
struct AimSettings
{
    using Version = AlgorithmVersion;
    Version version = Version::V1;
    ComparisonFactor comparison = ComparisonFactor::None;
    control::movement::Algorithm movement = control::movement::Algorithm::Historical;
    control::movement::TrajectorySettings trajectory;
    enum class InputMapping
    {
        NativeFormula,
        Measured
    };
    InputMapping inputMapping = InputMapping::NativeFormula;
    unsigned part = 0, priority = 0;
    WeaponAimParts weaponParts;
    std::optional<HitboxAimSettings> hitboxAim;
    TargetFilter targets;
    double coneDegrees = 15, maximumDistanceUnits = 0, maximumAgeMs = 150, responseMs = 20;
    bool prediction = true, recoil = true;
    control::SpeedPreset speed = control::SpeedPreset::Normal;
    std::uint16_t trigger = 8;
    std::optional<CountCalibration> calibration;
    bool operator==(const AimSettings &) const = default;
};
AimSettings aimSettings(const application::Json &);
TargetFilter targetFilterSettings(const application::Json &);
std::string_view algorithmVersionId(AimSettings::Version);
AimSettings::Version parseAlgorithmVersion(std::string_view);
control::movement::Algorithm historicalMovementAlgorithm(AimSettings::Version);
control::movement::Algorithm resolvedMovementAlgorithm(const AimSettings &);
struct CountMapping
{
    bool ready = false, measured = false;
    double x = 0, y = 0;
};
CountMapping resolveCountMapping(const AimSettings &, unsigned build);
std::optional<CountCalibration> countCalibration(const application::Json &);
application::Json calibrationJson(const CountCalibration &);
} // namespace mousebox::games::cs2
