#include "v2.hpp"
namespace mousebox::games::cs2::motion
{
Result V2::step(const Input &in, control::HumanMotionRuntime &human)
{
    return engine_.step(movementInput(in), human);
}
} // namespace mousebox::games::cs2::motion
