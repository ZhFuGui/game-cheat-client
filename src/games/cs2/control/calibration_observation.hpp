#pragma once
#include "aim_solution.hpp"
namespace mousebox::games::cs2
{
// Raw input calibration does not solve a target or evaluate an eye/recoil basis.
// It measures the observed native input angle against finite device counts.
struct CalibrationObservation
{
    std::string block = "waiting-source";
    control::SampleTimeEstimate timing;
    std::optional<Angles> angles;
    std::optional<MouseMapping> mapping;
    bool ready() const noexcept
    {
        return block == "ready";
    }
};
CalibrationObservation observeCalibration(const ControlInput &, const AimSettings &, std::uint64_t nowMs);
} // namespace mousebox::games::cs2
