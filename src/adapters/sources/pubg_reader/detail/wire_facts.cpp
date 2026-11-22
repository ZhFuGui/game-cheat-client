#include "wire_facts.hpp"
#include <chrono>
#include <climits>
#include <cmath>
#include <limits>
namespace mousebox::reader::detail
{
const s::Value *field(const Object &o, unsigned id)
{
    const auto f = o.find(id);
    return f == o.end() ? nullptr : &f->second;
}
const Object *object(const s::Value *v)
{
    return v ? std::get_if<Object>(&v->data) : nullptr;
}
std::optional<u64> integer(const s::Value *v)
{
    if (!v)
        return {};
    if (const auto x = std::get_if<u64>(&v->data))
        return *x;
    if (const auto x = std::get_if<std::int64_t>(&v->data); x && *x >= 0)
        return static_cast<u64>(*x);
    return {};
}
std::optional<double> number(const s::Value *v)
{
    if (!v)
        return {};
    if (const auto x = std::get_if<double>(&v->data))
        return *x;
    if (const auto x = std::get_if<u64>(&v->data))
        return static_cast<double>(*x);
    if (const auto x = std::get_if<std::int64_t>(&v->data))
        return static_cast<double>(*x);
    return {};
}
std::string text(const s::Value *v)
{
    const auto x = v ? std::get_if<std::string>(&v->data) : nullptr;
    return x ? *x : "";
}
std::optional<control::Vec3> vector(const s::Value *v)
{
    if (!v)
        return {};
    std::optional<double> x, y, z;
    if (const auto list = std::get_if<s::Value::List>(&v->data); list && list->size() == 3)
    {
        x = number(&(*list)[0]);
        y = number(&(*list)[1]);
        z = number(&(*list)[2]);
    }
    else if (const auto o = object(v))
    {
        x = number(field(*o, 1));
        y = number(field(*o, 2));
        z = number(field(*o, 3));
    }
    if (!x || !y || !z)
        return {};
    control::Vec3 result{*x, *y, *z};
    return control::finite(result) ? std::optional(result) : std::nullopt;
}
bool current(const s::GroupState &g, u64 cap)
{
    return (g.status == monitor::FactStatus::Available || g.status == monitor::FactStatus::Partial) &&
           g.verified_capture_sequence == cap;
}
const s::GroupState *context(const s::State &state, s::ContextGroup id)
{
    const auto f = state.context.find(id);
    return f != state.context.end() && current(f->second, state.capture_sequence) ? &f->second : nullptr;
}
const s::Value *fresh(const s::State &state, const s::GroupState *g, unsigned id)
{
    if (!g)
        return nullptr;
    const auto f = g->field_sources.find(id);
    if (f == g->field_sources.end() || f->second.capture_sequence != state.capture_sequence || f->second.reused ||
        f->second.source_category != state.batch.category)
        return nullptr;
    return field(g->fields, id);
}
const s::GroupState *property(const s::EntityState &e, monitor::PropertyGroup id, u64 cap)
{
    const auto f = e.groups.find(id);
    return f != e.groups.end() && current(f->second, cap) && f->second.source_capture_sequence == cap ? &f->second
                                                                                                      : nullptr;
}
control::Identity identity(const s::EntityState &e)
{
    return {e.id, e.key.instance.actor_address, e.key.instance.internal_index, e.key.instance.serial_number};
}
std::optional<int> team(const s::EntityState &e, u64 cap)
{
    const auto g = property(e, monitor::PropertyGroup::CharacterState, cap);
    const auto value = g ? number(field(g->fields, 1)) : std::nullopt;
    if (!value || *value < std::numeric_limits<int>::min() || *value > std::numeric_limits<int>::max())
        return {};
    return static_cast<int>(*value);
}
std::optional<control::Identity> instance(const s::Value *value)
{
    const auto o = object(value);
    if (!o)
        return {};
    const auto actor = integer(field(*o, 1)), index = integer(field(*o, 2)), serial = integer(field(*o, 3));
    if (!actor || !*actor || !index || *index > INT32_MAX || !serial || *serial > UINT32_MAX)
        return {};
    return control::Identity{0, *actor, static_cast<std::int32_t>(*index), static_cast<std::uint32_t>(*serial)};
}
std::int64_t ns(monitor::Clock::time_point time)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count();
}
const s::GroupState::NestedSample *nested(const s::GroupState &g, const std::vector<unsigned> &path, u64 capture)
{
    const auto found = g.nested_samples.find(path);
    if (found == g.nested_samples.end())
        return nullptr;
    const auto &value = found->second;
    const auto usable = [](auto status) {
        return status == monitor::OwnedReadStatus::Available || status == monitor::OwnedReadStatus::Partial;
    };
    if (value.capture_sequence != capture || !usable(value.latest_status) || !value.value_status ||
        !usable(*value.value_status) || ns(value.source.started) <= 0 || value.source.finished < value.source.started)
        return nullptr;
    return &value;
}
bool sameInstance(control::Identity a, control::Identity b)
{
    return a.actor == b.actor && a.index == b.index && a.serial == b.serial;
}
std::optional<bool> boolean(const s::Value *value)
{
    if (value)
        if (const auto b = std::get_if<bool>(&value->data))
            return *b;
    return {};
}
std::optional<double> real(const s::Value *value)
{
    auto n = number(value);
    return n && std::isfinite(*n) ? n : std::nullopt;
}
} // namespace mousebox::reader::detail
