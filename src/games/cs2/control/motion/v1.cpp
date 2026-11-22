#include "v1.hpp"
namespace mousebox::games::cs2::motion
{
Result V1::step(const Input &in, control::HumanMotionRuntime &human, std::optional<Counts> caps, MotionFactors factors)
{
    return engine_.step(in, human, caps, factors);
}
} // namespace mousebox::games::cs2::motion
