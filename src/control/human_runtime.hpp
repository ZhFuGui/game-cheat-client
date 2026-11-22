#pragma once
#include "control/human_motion.hpp"
#include <memory>
#include <random>
namespace mousebox::control
{
struct HumanSpeedLimits
{
    double speed = 0, acceleration = 0, braking = 0;
};
struct HumanVariation
{
    double x = 0, y = 0;
};
class HumanMotionRuntime
{
    std::shared_ptr<const HumanMotionModel> model_;
    unsigned dpi_ = 1600;
    bool variation_ = false;
    double lateral_ = 0;
    std::uint64_t noiseAt_ = 0;
    std::mt19937 random_{std::random_device{}()};

  public:
    HumanMotionRuntime() = default;
    explicit HumanMotionRuntime(std::uint32_t seed) : random_(seed) {}
    bool variationEnabled() const noexcept
    {
        return variation_;
    }
    void configure(std::shared_ptr<const HumanMotionModel>, unsigned dpi, bool variation = false);
    bool enabled() const noexcept
    {
        return model_ != nullptr;
    }
    void reset() noexcept
    {
        lateral_ = 0;
        noiseAt_ = 0;
    }
    HumanSpeedLimits limits() const noexcept;
    HumanVariation offset(double xCounts, double yCounts, double angularDistance, std::uint64_t now);
};
} // namespace mousebox::control
