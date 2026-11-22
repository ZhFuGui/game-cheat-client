#include "kinematic.hpp"
namespace mousebox::games::cs2::motion
{
Result Kinematic::step(const Input &in, control::HumanMotionRuntime &human, std::optional<Counts> caps,
                       MotionFactors factors)
{
    return engine_.step(movementInput(in), human, caps, factors);
}
} // namespace mousebox::games::cs2::motion
