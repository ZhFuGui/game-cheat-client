#pragma once
#include "games/pubg/model/control_frame.hpp"
#include "synchronization/codec/wire.hpp"
namespace mousebox::reader::detail
{
namespace s = monitor::synchronization;
using monitor::u64;
using Object = s::Value::Object;
const s::Value *field(const Object &, unsigned);
const Object *object(const s::Value *);
std::optional<u64> integer(const s::Value *);
std::optional<double> number(const s::Value *);
std::string text(const s::Value *);
std::optional<control::Vec3> vector(const s::Value *);
bool current(const s::GroupState &, u64);
const s::GroupState *context(const s::State &, s::ContextGroup);
const s::Value *fresh(const s::State &, const s::GroupState *, unsigned);
const s::GroupState *property(const s::EntityState &, monitor::PropertyGroup, u64);
control::Identity identity(const s::EntityState &);
std::optional<int> team(const s::EntityState &, u64);
std::optional<control::Identity> instance(const s::Value *);
std::int64_t ns(monitor::Clock::time_point);
const s::GroupState::NestedSample *nested(const s::GroupState &, const std::vector<unsigned> &, u64);
bool sameInstance(control::Identity, control::Identity);
std::optional<bool> boolean(const s::Value *);
std::optional<double> real(const s::Value *);
} // namespace mousebox::reader::detail
