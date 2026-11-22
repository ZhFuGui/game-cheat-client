#pragma once
#include "application/json.hpp"
#include "domain/random_range.hpp"

namespace mousebox::application
{
// Missing distribution is the legacy normal mode. Present modes are strict.
random::IntegerRange readRandomRange(const Json &, unsigned minimum, unsigned maximum);
Json randomRangeJson(const random::IntegerRange &);
} // namespace mousebox::application
