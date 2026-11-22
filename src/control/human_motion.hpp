#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace mousebox::control
{
// Broad numeric input protection in 1600-DPI units, not a human performance limit.
inline constexpr double maximumRecordedSpeed = 1000000;
struct MouseSample
{
    std::uint64_t us = 0;
    double x = 0, y = 0;
};
struct MouseTrial
{
    unsigned dpi = 1600;
    std::vector<MouseSample> samples;
    std::string session; // One explicit start/pause recording, not one training window.
};
struct HumanMotionModel
{
    unsigned version = 2, sessions = 0, trainingSessions = 0, validationSessions = 0, trials = 0, trainingRows = 0,
             validationRows = 0;
    double movingSeconds = 0, speed50 = 0, speed90 = 0, speed95 = 0, acceleration90 = 0, braking90 = 0;
    double logSpeedMean = 0, logSpeedSigma = 0, validationSpeed90 = 0, validationRelativeGap = 0;
    bool valid() const noexcept;
};
HumanMotionModel trainHumanMotion(const std::vector<MouseTrial> &);
} // namespace mousebox::control
