#pragma once
#include "synchronization/state.hpp"
#include "model/geometry.hpp"

namespace monitor::synchronization {
bool SkeletonValueShape(unsigned field, const Value&);
bool SkeletonGroupValid(const GroupState&, const State&);
bool TransformWireShape(const Value&);
Transform TransformFromWire(const Value&);
} // namespace monitor::synchronization
