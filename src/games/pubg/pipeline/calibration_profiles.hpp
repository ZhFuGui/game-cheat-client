#pragma once
#include "games/pubg/model/control_frame.hpp"
namespace mousebox::control
{
enum class ViewMode
{
    ThirdPerson,
    FirstPerson,
    Aim,
    Ads
};
enum class InputMapping
{
    DynamicFov,
    MeasuredOnly
};
struct GameInputSettings
{
    double general = 50, aim = 50, ads = 60, vertical = 1;
    bool inverted = false;
    bool operator==(const GameInputSettings &) const = default;
};
bool validGameInputSettings(const GameInputSettings &) noexcept;
std::optional<Calibration> pubgInputResponse(std::optional<double> fov, ViewMode, const GameInputSettings &);
bool inputResponseContextChanged(const Calibration &, const Calibration &) noexcept;
struct CalibrationRecord
{
    ViewMode mode = ViewMode::ThirdPerson;
    Calibration value;
};
enum class CalibrationMatchStatus
{
    Ready,
    ViewMissing,
    Missing,
    Ambiguous
};
struct CalibrationMatch
{
    CalibrationMatchStatus status = CalibrationMatchStatus::ViewMissing;
    std::optional<CalibrationRecord> record;
};
class CalibrationProfiles
{
    std::vector<CalibrationRecord> records_;

  public:
    void store(CalibrationRecord);
    void clear() noexcept
    {
        records_.clear();
    }
    const std::vector<CalibrationRecord> &records() const noexcept
    {
        return records_;
    }
    CalibrationMatch match(std::optional<double> fov, std::optional<ViewMode> mode = {}) const;
};
} // namespace mousebox::control
