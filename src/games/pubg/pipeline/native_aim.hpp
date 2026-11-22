#pragma once
#include "games/pubg/model/control_frame.hpp"
#include <string_view>
#include <span>
namespace mousebox::control
{
enum class NativeAimBlock
{
    Ready,
    Missing,
    Context,
    Identity,
    Branch,
    Timing,
    Controller,
    AxisSingular
};
struct NativeAimEvaluation
{
    NativeAimBlock block = NativeAimBlock::Missing;
    std::optional<NativeAimReference> reference;
};
inline constexpr std::string_view NativeInputModel = "controller-euler-local-rigid-pose-v1";
// All required native fields keep their own clock/identity qualification.
NativeAimEvaluation qualifyNativeAim(const Frame &, Identity weapon, std::uint64_t now);
std::span<const unsigned> nativeAimRequiredFields(FrameContract) noexcept;
// These are spherical forward coordinates only: x=pitch,y=yaw. No roll is inferred.
std::optional<Vec3> directionCoordinates(Vec3) noexcept;
Vec3 coordinateDirection(double pitchDegrees, double yawDegrees) noexcept;
// Controller Yaw rotates about world Z; Pitch about -Y after Controller Yaw.
// Freeze the local barrel pose for this small command, and reobserve every sample.
Vec3 nativeInputEffect(Vec3 forward, double controllerYaw, double yawDelta, double pitchDelta) noexcept;
std::optional<AngularError> nativeInputError(Vec3 forward, Vec3 desired, double controllerYaw) noexcept;
} // namespace mousebox::control
