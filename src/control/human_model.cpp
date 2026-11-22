#include "control/human_motion.hpp"
#include <cmath>
namespace mousebox::control
{
bool HumanMotionModel::valid() const noexcept
{
    const double values[]{movingSeconds, speed50,      speed90,       speed95,           acceleration90,
                          braking90,     logSpeedMean, logSpeedSigma, validationSpeed90, validationRelativeGap};
    for (auto v : values)
        if (!std::isfinite(v))
            return false;
    return version == 2 && sessions >= 3 && sessions <= 1000 && trainingSessions >= 2 && validationSessions >= 1 &&
           trainingSessions + validationSessions == sessions && trials >= sessions && trials <= 1000 &&
           trainingRows >= 200 && trainingRows <= 2000000 && validationRows >= 100 && validationRows <= 2000000 &&
           movingSeconds >= 6 && speed50 >= 40 && speed50 <= speed90 && speed90 <= speed95 &&
           speed95 <= maximumRecordedSpeed && acceleration90 > 0 && acceleration90 <= 4000000 && braking90 > 0 &&
           braking90 <= 4000000 && logSpeedSigma >= 0 && logSpeedSigma <= 10 && std::abs(logSpeedMean) <= 20 &&
           validationSpeed90 >= 40 && validationSpeed90 <= maximumRecordedSpeed && validationRelativeGap >= 0 &&
           validationRelativeGap <= 1000;
}
} // namespace mousebox::control
