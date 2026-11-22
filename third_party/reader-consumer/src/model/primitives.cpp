#include "model/primitives.hpp"
#include <cmath>
namespace monitor {
bool ValidPosition(const Vec3& v) noexcept {
    constexpr float bound = 50000000.0f;
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) && std::fabs(v.x) < bound &&
           std::fabs(v.y) < bound && std::fabs(v.z) < bound;
}
} // namespace monitor
