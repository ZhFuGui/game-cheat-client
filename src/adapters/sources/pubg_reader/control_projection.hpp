#pragma once
#include "games/pubg/model/control_frame.hpp"
#include "synchronization/codec/wire.hpp"
namespace monitor::synchronization::consumer
{
struct State;
}
namespace mousebox::reader
{
control::Frame projectControlFrame(const monitor::synchronization::State &validated,
                                   const monitor::synchronization::codec::Binding &);
control::Frame projectControlFrame(const monitor::synchronization::consumer::State &validated,
                                   const monitor::synchronization::codec::Binding &);
// Join the current Local body's complete registry identity to a current Living
// character. This derives consumer metadata without replacing either source's
// sample times, positions or velocities; Local loss clears the derived fields.
void bindLocalControl(control::Frame &living, const control::Frame *local);
} // namespace mousebox::reader
