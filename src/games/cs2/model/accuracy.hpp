#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace mousebox::games::cs2
{
// Independent read-only observations of the two native getter results.
struct AccuracyScalar
{
    unsigned status = 0;
    std::optional<double> value;
    bool malformed = false;
};
struct WeaponAccuracy
{
    std::string modelId;
    std::optional<std::uint32_t> owner;
    AccuracyScalar inaccuracy, spread;
    unsigned branch = 0;
    std::optional<double> movement, airborne, turning, lateral, penalty, stateTime;
    std::optional<int> mode;
    std::optional<std::uint64_t> started, readFinished;
    bool malformed = false;
};
} // namespace mousebox::games::cs2
