#pragma once
#include "games/pubg/model/control_frame.hpp"
#include "synchronization/consumer/state.hpp"
#include <chrono>
#include <initializer_list>

namespace mousebox::reader::consumer_detail
{
namespace s = monitor::synchronization;
namespace c = s::consumer;
inline std::int64_t ns(monitor::Clock::time_point value)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(value.time_since_epoch()).count();
}
inline bool interval(const monitor::SampleInterval &sample, const c::State &state)
{
    return ns(sample.started) > 0 && sample.finished >= sample.started && sample.started >= state.capture.started &&
           sample.finished <= state.capture.finished;
}
inline bool current(const c::GroupSample &group, const c::State &state)
{
    return state.accepted && c::HasValues(group.status) && group.value &&
           group.source_capture_sequence == state.capture_sequence && interval(group.source, state);
}
inline bool fresh(const c::SourceTiming &source, const c::State &state)
{
    return c::HasValues(source.status) && !source.reused && source.capture_sequence == state.capture_sequence &&
           source.source_category == state.category && interval(source.source, state);
}
inline const c::SourceTiming *source(const c::GroupSample &group, std::initializer_list<unsigned> path,
                                     const c::State &state)
{
    const auto found = group.sources.find(std::vector<unsigned>(path));
    return current(group, state) && found != group.sources.end() && fresh(found->second, state) ? &found->second
                                                                                                : nullptr;
}
inline bool field(const c::GroupSample &group, unsigned id, const c::State &state)
{
    return source(group, {id}, state) != nullptr;
}
inline bool contextField(const c::State &state, unsigned id)
{
    const auto found = state.context.sources.find({id});
    if (found == state.context.sources.end() || !state.accepted)
        return false;
    const auto &timing = found->second;
    if (fresh(timing, state))
        return true;
    // Only the formal static-context path may reuse an independent directory
    // sample. Preserve its old time; it cannot qualify dynamic geometry/angles.
    return timing.reused && timing.source_category == monitor::DataCategory::None && c::HasValues(timing.status) &&
           timing.capture_sequence && ns(timing.source.started) > 0 &&
           timing.source.finished >= timing.source.started && timing.source.finished <= state.capture.finished;
}
template <class T> const T *value(const c::GroupSample *group, const c::State &state)
{
    return group && current(*group, state) ? std::get_if<T>(&*group->value) : nullptr;
}
inline const c::GroupSample *local(const c::State &state, s::ContextGroup id)
{
    const auto found = state.context.local_groups.find(id);
    return found == state.context.local_groups.end() ? nullptr : &found->second;
}
inline const c::GroupSample *property(const c::Entity &entity, monitor::PropertyGroup id)
{
    const auto found = entity.groups.find(id);
    return found == entity.groups.end() ? nullptr : &found->second;
}
inline control::Vec3 vector(c::Vector v)
{
    return {v.x, v.y, v.z};
}
inline control::Vec3 vector(c::Origin v)
{
    return {double(v.x), double(v.y), double(v.z)};
}
inline control::Vec3 vector(c::Rotation v)
{
    return {v.pitch, v.yaw, v.roll};
}
inline std::optional<control::Identity> character(const std::optional<c::Reference> &reference)
{
    return reference && reference->id && reference->kind == monitor::EntityKind::Character
               ? std::optional(control::Identity::entity(reference->id))
               : std::nullopt;
}
inline control::NativeObservation observation(const c::GroupSample &group, const c::State &state)
{
    control::NativeObservation out;
    out.contract = control::FrameContract::ConsumerV3;
    out.status = static_cast<unsigned>(group.status);
    out.current = current(group, state);
    out.capture = group.source_capture_sequence;
    if (out.current)
    {
        out.sourceStartNs = ns(group.source.started);
        out.sourceFinishNs = ns(group.source.finished);
        for (const auto &[path, timing] : group.sources)
            if (path.size() == 1 && fresh(timing, state))
                out.fields.emplace(path.front(),
                                   control::NativeFieldSource{ns(timing.source.started), ns(timing.source.finished),
                                                              timing.capture_sequence, 0});
    }
    return out;
}
void projectLocal(const c::State &, control::Frame &);
} // namespace mousebox::reader::consumer_detail
