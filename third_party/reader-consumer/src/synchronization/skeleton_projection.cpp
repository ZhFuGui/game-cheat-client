#include "synchronization/skeleton_projection.hpp"

namespace monitor::synchronization {
namespace {
Value Identity(const InstanceIdentity& i) {
    Value::Object out;
    out.emplace(1, Value(i.actor_address));
    out.emplace(2, Value(std::int64_t(i.internal_index)));
    out.emplace(3, Value(u64(i.serial_number)));
    return Value(std::move(out));
}
Value Vector(const Vec3& v) {
    Value::Object out;
    out.emplace(1, Value(double(v.x)));
    out.emplace(2, Value(double(v.y)));
    out.emplace(3, Value(double(v.z)));
    return Value(std::move(out));
}
} // namespace
Value TransformValue(const Transform& t) {
    Value::Object rotation;
    rotation.emplace(1, Value(double(t.rotation.x)));
    rotation.emplace(2, Value(double(t.rotation.y)));
    rotation.emplace(3, Value(double(t.rotation.z)));
    rotation.emplace(4, Value(double(t.rotation.w)));
    Value::Object out;
    out.emplace(1, Value(std::move(rotation)));
    out.emplace(2, Vector(t.translation));
    out.emplace(3, Vector(t.scale));
    return Value(std::move(out));
}
namespace {
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
std::map<unsigned, Value> SkeletonFields(const CharacterSkeletonSample& sample) {
    if (!sample.source || !sample.observation.value_status)
        return {};
    const auto& s = *sample.source;
    auto source = SkeletonSourceValue(s);
    Value::List bones;
    bones.reserve(sample.bones.size());
    for (const auto& bone : sample.bones) {
        Value::Object b;
        b.emplace(1, Value(u64(bone.index)));
        b.emplace(6, Value(std::int64_t(bone.pose_index)));
        if (bone.parent_index)
            b.emplace(2, Value(std::int64_t(*bone.parent_index)));
        if (bone.name)
            b.emplace(3, Name(*bone.name));
        if (bone.component_transform)
            b.emplace(4, TransformValue(*bone.component_transform));
        if (bone.world_position)
            b.emplace(5, Vector(*bone.world_position));
        bones.emplace_back(std::move(b));
    }
    // initializer_list 的元素为 const，会再次复制完整骨骼树；直接转交新建树的所有权。
    Value::Object out;
    out.emplace(1, std::move(source));
    if (!bones.empty() || !s.bone_count)
        out.emplace(2, Value(std::move(bones)));
    return out;
}
Value SkeletonSourceValue(const SkeletonSource& s) {
    Value::Object source;
    source.emplace(1, Identity(s.mesh));
    source.emplace(2, Identity(s.skeletal_mesh));
    source.emplace(3, Identity(s.pose_component));
    source.emplace(4, Value(u64(s.read_buffer_index)));
    source.emplace(5, Value(u64(s.bone_count)));
    source.emplace(6, Value(u64(s.pose_transform_count)));
    source.emplace(7, TransformValue(s.component_to_world));
    Value::Object origin;
    origin.emplace(1, Value(std::int64_t(s.origin.x)));
    origin.emplace(2, Value(std::int64_t(s.origin.y)));
    origin.emplace(3, Value(std::int64_t(s.origin.z)));
    source.emplace(8, Value(std::move(origin)));
    return Value(std::move(source));
}
} // namespace monitor::synchronization
