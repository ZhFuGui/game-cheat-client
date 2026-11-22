#include "games/pubg/pipeline/calibration_profiles.hpp"
#include <cmath>
#include <stdexcept>
namespace mousebox::control
{
bool validGameInputSettings(const GameInputSettings &s) noexcept
{
    const auto slider = [](double value) { return std::isfinite(value) && value >= 0 && value <= 100; };
    return slider(s.general) && slider(s.aim) && slider(s.ads) && std::isfinite(s.vertical) && s.vertical >= 0.1 &&
           s.vertical <= 5;
}
std::optional<Calibration> pubgInputResponse(std::optional<double> fov, ViewMode mode, const GameInputSettings &s)
{
    if (!fov || !std::isfinite(*fov) || *fov <= 0 || *fov >= 180 || !validGameInputSettings(s) ||
        mode < ViewMode::ThirdPerson || mode > ViewMode::Ads)
        return {};
    const double sensitivity = mode == ViewMode::Aim ? s.aim : mode == ViewMode::Ads ? s.ads : s.general;
    // PUBG input-response hypothesis: exponential slider conversion and linear camera-FOV scaling.
    // 50 at FOV 80/90 agrees with the existing local yaw measurements; this is not universal game validation.
    // Injected HID counts already are counts: physical mouse DPI must not multiply this coefficient again.
    const double yaw = 0.002 * std::pow(10.0, sensitivity / 50.0) * 2.5 * (*fov / 90.0);
    Calibration result{false,
                       "pubg-fov-estimate-v1:" + std::to_string(sensitivity) + ":" + std::to_string(s.vertical) +
                           (s.inverted ? ":inverted" : ":normal"),
                       yaw,
                       (s.inverted ? 1 : -1) * yaw * s.vertical,
                       *fov,
                       true};
    return validCalibration(result) ? std::optional(result) : std::nullopt;
}
bool inputResponseContextChanged(const Calibration &a, const Calibration &b) noexcept
{
    if (a.measured != b.measured || a.formulaEstimated != b.formulaEstimated || a.profile != b.profile)
        return true;
    // A continuous zoom changes the response estimate, not the user's configuration or the held gesture.
    if (a.formulaEstimated && b.formulaEstimated)
        return false;
    return a.fov != b.fov || a.yawDegreesPerCount != b.yawDegreesPerCount ||
           a.pitchDegreesPerCount != b.pitchDegreesPerCount;
}
void CalibrationProfiles::store(CalibrationRecord record)
{
    if (!record.value.measured || record.value.formulaEstimated || !validCalibration(record.value) ||
        record.mode < ViewMode::ThirdPerson || record.mode > ViewMode::Ads)
        throw std::invalid_argument("Invalid measured calibration profile");
    for (auto &previous : records_)
        if (previous.mode == record.mode && std::abs(previous.value.fov - record.value.fov) <= 0.01)
        {
            previous = std::move(record);
            return;
        }
    if (records_.size() >= 64)
        throw std::runtime_error("Calibration profile limit reached");
    records_.push_back(std::move(record));
}
CalibrationMatch CalibrationProfiles::match(std::optional<double> fov, std::optional<ViewMode> mode) const
{
    if (!fov || !std::isfinite(*fov) || *fov <= 0 || *fov >= 180)
        return {};
    CalibrationMatch result{CalibrationMatchStatus::Missing, {}};
    for (const auto &record : records_)
    {
        if (mode && record.mode != *mode)
            continue;
        if (std::abs(record.value.fov - *fov) > 0.01)
            continue;
        // FOV alone cannot distinguish different sensitivity modes. Require an explicit choice.
        if (result.record)
            return {CalibrationMatchStatus::Ambiguous, {}};
        result = {CalibrationMatchStatus::Ready, record};
    }
    return result;
}
} // namespace mousebox::control
