#pragma once
#include "control/movement/types.hpp"
#include "control/movement/algorithm.hpp"
#include "control/speed_policy.hpp"
#include "domain/math.hpp"
#include "domain/source_sample.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mousebox::games::valorant::aim
{
struct Settings
{
    bool enabled = false, enemies = true, teammates = false, checkForeground = true;
    unsigned part = 0, priority = 1, maximumAgeMs = 150, responseMs = 20;
    double coneDegrees = 15, maximumDistance = 0;
    // Explicit user measurements, signed degrees per positive physical mouse count.
    // Zero remains unknown; this is not a native input coefficient.
    double yawPerCount = 0, pitchPerCount = 0;
    control::SpeedPreset speed = control::SpeedPreset::Normal;
    control::movement::Algorithm algorithm = control::movement::Algorithm::Smooth;
    control::movement::TrajectorySettings trajectory;
    bool operator==(const Settings &) const = default;
};
struct Identity
{
    std::string reader, clockDomain;
    std::uint64_t epoch = 0, localEntity = 0;
    std::array<std::uint64_t, 3> connections{};
    bool operator==(const Identity &) const = default;
};
struct Target
{
    std::uint64_t id = 0;
    // Reader relationship values: 0 unknown, 1 self, 2 ally, 3 opponent.
    unsigned relationship = 0;
    std::optional<bool> alive;
    // Main22 head, neck, chest and pelvis, without invented Hitbox geometry.
    std::array<std::optional<domain::Vec3>, 4> points;
};
struct Frame
{
    Identity identity;
    std::array<domain::SourceSample, 3> samples;
    std::uint64_t revision = 0, cameraCapture = 0;
    domain::Vec3 origin;
    control::movement::Vector viewDegrees;
    bool localResolved = false;
    std::vector<Target> targets;
};
struct Input
{
    std::shared_ptr<const Frame> frame;
    bool foreground = false;
    // An explicit user assertion scoped to this connection/Reader/local identity.
    // It does not manufacture an upstream native control-qualification field.
    bool manualViewConfirmed = false;
};
struct Solution
{
    std::string block = "waiting-source";
    std::optional<std::uint64_t> target;
    domain::Vec3 point;
    control::movement::Vector errorDegrees;
    double distance = 0, angularDistance = 0, sampleMs = 0, ageMs = 0;
    bool lifeKnown = false;
    bool ready() const noexcept
    {
        return block.empty();
    }
};
Solution solve(const Input &, const Settings &, std::uint64_t nowMs, std::optional<std::uint64_t> held = {});
bool canConfirmView(const Input &, const Settings &, std::uint64_t nowMs);
} // namespace mousebox::games::valorant::aim
