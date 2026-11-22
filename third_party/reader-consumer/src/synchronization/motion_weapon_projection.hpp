#pragma once
#include "synchronization/state.hpp"
#include "model/entities/muzzle.hpp"
#include "model/entities/world_velocity.hpp"

namespace monitor::synchronization {
Value WorldVelocityValue(const WorldVelocitySample&);
Value MuzzleValue(const WeaponMuzzleSample&);
} // namespace monitor::synchronization
