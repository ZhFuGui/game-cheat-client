#pragma once
#include "synchronization/state.hpp"

namespace monitor::synchronization {
bool LocalValueShape(ContextGroup, unsigned field, const Value&, const State&);
bool ValidateLocalBody(const State&, std::string& error);
} // namespace monitor::synchronization
