#pragma once
#include "synchronization/state.hpp"

namespace monitor::synchronization {
bool WorldVelocityValueShape(const Value&, const EntityState&);
bool MuzzleValueShape(const Value&, const State&);
} // namespace monitor::synchronization
