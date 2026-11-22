#pragma once

namespace mousebox::games::cs2
{
struct Snapshot;
struct WeaponAccuracy;

enum class AccuracyIssue
{
    None,
    Malformed,
    UnverifiedModel,
    OutsideCapture,
    InconsistentInaccuracy
};

AccuracyIssue validateWeaponAccuracy(const Snapshot &, const WeaponAccuracy &) noexcept;
} // namespace mousebox::games::cs2
