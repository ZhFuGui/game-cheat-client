#include "synchronization/motion_weapon_projection.hpp"
#include "synchronization/skeleton_projection.hpp"

namespace monitor::synchronization {
namespace {
Value Identity(const InstanceIdentity& i) {
    return Value(Value::Object{
        {1, Value(i.actor_address)}, {2, Value(std::int64_t(i.internal_index))}, {3, Value(u64(i.serial_number))}});
}
Value Name(const ObjectName& name) {
    Value::Object out;
    if (name.raw_index)
        out.emplace(1, Value(u64(*name.raw_index)));
    if (name.raw_number)
        out.emplace(2, Value(u64(*name.raw_number)));
    if (name.index)
        out.emplace(3, Value(u64(*name.index)));
    if (name.number)
        out.emplace(4, Value(u64(*name.number)));
    if (name.verified)
        out.emplace(5, Value(name.full));
    return Value(std::move(out));
}
} // namespace
Value WorldVelocityValue(const WorldVelocitySample& s) {
    if (!s.observation.value_status || !s.velocity || !s.source || !s.actor)
        return Value(Value::Object{});
    Value::Object out{{1, Value(Value::Object{{1, Value(double(s.velocity->x))},
                                              {2, Value(double(s.velocity->y))},
                                              {3, Value(double(s.velocity->z))}})},
                      {2, Value(u64(*s.source))},
                      {3, Identity(*s.actor)}};
    if (s.component)
        out.emplace(4, Identity(*s.component));
    if (s.carrier)
        out.emplace(5, Identity(*s.carrier));
    return Value(std::move(out));
}
Value MuzzleValue(const WeaponMuzzleSample& s) {
    if (!s.observation.value_status || !s.source || !s.world_transform)
        return Value(Value::Object{});
    Value::Object out{{1, SkeletonSourceValue(*s.source)}, {10, TransformValue(*s.world_transform)}};
    if (s.attach_name)
        out.emplace(2, Name(*s.attach_name));
    if (s.bone_name)
        out.emplace(3, Name(*s.bone_name));
    if (s.bone_index)
        out.emplace(4, Value(u64(*s.bone_index)));
    if (s.pose_index)
        out.emplace(5, Value(u64(*s.pose_index)));
    if (s.binding)
        out.emplace(6, Value(u64(*s.binding)));
    if (s.socket)
        out.emplace(7, Identity(*s.socket));
    if (s.relative_transform)
        out.emplace(8, TransformValue(*s.relative_transform));
    if (s.bone_component_transform)
        out.emplace(9, TransformValue(*s.bone_component_transform));
    return Value(std::move(out));
}
} // namespace monitor::synchronization
