#pragma once
#include "synchronization/state.hpp"
#include "model/entities/skeleton.hpp"

namespace monitor::synchronization {
std::map<unsigned, Value> SkeletonFields(const CharacterSkeletonSample&);
Value SkeletonSourceValue(const SkeletonSource&);
Value TransformValue(const Transform&);
} // namespace monitor::synchronization
