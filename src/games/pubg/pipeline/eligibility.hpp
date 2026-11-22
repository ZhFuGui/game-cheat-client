#pragma once
#include "games/pubg/model/control_frame.hpp"

namespace mousebox::control
{
enum class Reason
{
    Ready,
    NotCurrent,
    ReaderMismatch,
    EpochMismatch,
    WorldMissing,
    WorldMismatch,
    LevelMissing,
    LevelMismatch,
    CoordinatesMissing,
    CoordinatesMismatch,
    ClockMismatch,
    SourceSkew,
    LocalIdentityMissing,
    ControllerMismatch,
    ViewMissing,
    CalibrationMissing,
    CalibrationMismatch,
    AgeUnknown,
    TooOld,
    DeviceUnavailable
};
struct PairVerdict
{
    Reason reason = Reason::NotCurrent;
    double sourceSkewMs = 0;
    bool usable() const
    {
        return reason == Reason::Ready;
    }
};
struct UsePolicy
{
    double maximumSourceSkewMs = 800, maximumAgeUpperMs = 100;
};
PairVerdict checkPair(const Frame &living, const Frame &view, UsePolicy = {});
Reason checkMotion(const Frame &view, PairVerdict, const Calibration &, std::optional<double> endToEndAgeUpperMs,
                   bool deviceReady, UsePolicy = {});
const char *reasonName(Reason) noexcept;
} // namespace mousebox::control
