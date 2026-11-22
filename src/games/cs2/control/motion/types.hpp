#pragma once
#include "games/cs2/control/settings.hpp"
#include "control/motion_policy.hpp"
#include "control/human_runtime.hpp"
#include "control/movement/types.hpp"
namespace mousebox::games::cs2::motion
{
using Counts = control::movement::Counts;
struct Input
{
    Angles error, recoil;
    double yawPerCount = 0, pitchPerCount = 0, dt = 0;
    control::MotionPolicy policy;
    std::uint64_t now = 0, capture = 0;
    std::int64_t sourceStartNs = 0;
    double simulationTicks = 0, secondsPerTick = 0, uncertaintyMs = 0;
    EyeAngleSource basisSource = EyeAngleSource::Unspecified;
    bool recoilEnabled = false;
};
using Result = control::movement::Result;
inline control::movement::Input movementInput(const Input &in)
{
    control::movement::Input value;
    value.errorDegrees = {in.error.y, in.error.x};
    value.recoilDegrees = {in.recoil.y, in.recoil.x};
    value.degreesPerCount = {in.yawPerCount, in.pitchPerCount};
    value.dt = in.dt;
    value.policy = in.policy;
    value.now = in.now;
    value.capture = in.capture;
    value.sourceStartNs = in.sourceStartNs;
    value.simulationTicks = in.simulationTicks;
    value.secondsPerTick = in.secondsPerTick;
    value.uncertaintyMs = in.uncertaintyMs;
    value.recoilEnabled = in.recoilEnabled;
    value.recoilDeltaQualified = in.basisSource == EyeAngleSource::PredictionInput;
    return value;
}
} // namespace mousebox::games::cs2::motion
