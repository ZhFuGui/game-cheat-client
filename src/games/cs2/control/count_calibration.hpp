#pragma once
#include "aim_solution.hpp"
#include "calibration_observation.hpp"
#include "application/ports/output_program.hpp"
#include <array>
namespace mousebox::games::cs2
{
// Four finite, bidirectional experiments. Merely constructing/arming never moves.
// The owner must independently validate the physical side key for every emitted step.
class CountCalibrationSession
{
    bool active_ = false, released_ = false;
    unsigned leg_ = 0, phase_ = 0;
    int total_ = 0, sent_ = 0;
    std::uint64_t started_ = 0, phaseAt_ = 0, lastSent_ = 0, firstSent_ = 0, capture_ = 0;
    double baseline_ = 0, crossBaseline_ = 0, derivative_ = 0, otherDerivative_ = 0;
    double responseUpper_ = 0;
    std::array<double, 4> ratios_{};
    std::optional<CountCalibration> result_;
    std::string status_ = "idle", identity_;

  public:
    void start();
    void cancel(std::string reason = "cancelled");
    bool active() const noexcept
    {
        return active_;
    }
    unsigned leg() const noexcept
    {
        return leg_;
    }
    const std::string &status() const noexcept
    {
        return status_;
    }
    const std::optional<CountCalibration> &result() const noexcept
    {
        return result_;
    }
    control::MotionStep step(const ControlInput &, const application::ports::OutputTick &, const AimSettings &);
};
} // namespace mousebox::games::cs2
