#include "synchronization/motion_weapon_validation.hpp"
#include "synchronization/skeleton_validation.hpp"
#include "unreal/transform.hpp"
#include "memory/address.hpp"
#include <algorithm>
#include <limits>

namespace monitor::synchronization {
namespace {
using Object = Value::Object;
const Object* Fields(const Value& v, std::initializer_list<unsigned> allowed) {
    const auto* out = std::get_if<Object>(&v.data);
    if (!out)
        return nullptr;
    for (const auto& [field, value] : *out)
        if (std::find(allowed.begin(), allowed.end(), field) == allowed.end())
            return nullptr;
    return out;
}
bool UInt(const Value& v) {
    const auto* n = std::get_if<u64>(&v.data);
    return n && *n <= UINT32_MAX;
}
bool Int(const Value& v) {
    const auto* n = std::get_if<std::int64_t>(&v.data);
    return n && *n >= 0 && *n <= INT32_MAX;
}
bool Identity(const Value& v) {
    const auto* o = Fields(v, {1, 2, 3});
    if (!o || o->size() != 3 || !o->contains(1) || !o->contains(2) || !o->contains(3))
        return false;
    const auto* address = std::get_if<u64>(&o->at(1).data);
    return address && UserAddress(*address) && Int(o->at(2)) && UInt(o->at(3));
}
bool Vector(const Value& v) {
    const auto* o = Fields(v, {1, 2, 3});
    if (!o || o->size() != 3)
        return false;
    for (unsigned field : {1u, 2u, 3u}) {
        if (!o->contains(field))
            return false;
        const auto* n = std::get_if<double>(&o->at(field).data);
        if (!n || !std::isfinite(*n) || std::abs(*n) > std::numeric_limits<float>::max())
            return false;
    }
    return true;
}
bool Name(const Value& v) {
    const auto* o = Fields(v, {1, 2, 3, 4, 5});
    if (!o || !o->contains(1) || !o->contains(2) || !o->contains(3) || !o->contains(4))
        return false;
    for (const auto& [field, value] : *o)
        if (field == 5 ? !std::holds_alternative<std::string>(value.data) : !UInt(value))
            return false;
    return true;
}
bool Near(float a, float b) {
    return std::abs(a - b) <= std::max(.05f, std::abs(b) * std::numeric_limits<float>::epsilon() * 8);
}
bool Near(const Vec3& a, const Vec3& b) {
    return Near(a.x, b.x) && Near(a.y, b.y) && Near(a.z, b.z);
}
bool Near(const Transform& actual, const Transform& expected) {
    if (!Near(actual.translation, expected.translation) || !Near(actual.scale, expected.scale))
        return false;
    const auto& a = actual.rotation;
    const auto& b = expected.rotation;
    const float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    const float na = a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w, nb = b.x * b.x + b.y * b.y + b.z * b.z + b.w * b.w;
    const float length = std::sqrt(na * nb);
    return std::isfinite(dot) && std::isfinite(length) && length > 0 && std::abs(std::abs(dot) / length - 1) <= .001f;
}
} // namespace
bool WorldVelocityValueShape(const Value& v, const EntityState& entity) {
    const auto* o = Fields(v, {1, 2, 3, 4, 5});
    if (!o || o->empty())
        return o != nullptr;
    if (!o->contains(1) || !o->contains(2) || !o->contains(3) || !Vector(o->at(1)) || !UInt(o->at(2)) ||
        !Identity(o->at(3)))
        return false;
    const auto source = std::get<u64>(o->at(2).data);
    if (source < 1 || source > 5)
        return false;
    const auto& actor = std::get<Object>(o->at(3).data);
    if (std::get<u64>(actor.at(1).data) != entity.key.instance.actor_address ||
        std::get<std::int64_t>(actor.at(2).data) != entity.key.instance.internal_index ||
        std::get<u64>(actor.at(3).data) != entity.key.instance.serial_number)
        return false;
    if (o->contains(4) && !Identity(o->at(4)) || o->contains(5) && !Identity(o->at(5)))
        return false;
    if ((source == 3 || source == 4) != o->contains(5))
        return false;
    if ((source == 1 || source == 2) && !o->contains(4))
        return false;
    if (source == 5) {
        if (o->contains(4))
            return false;
        for (const auto& [field, n] : std::get<Object>(o->at(1).data))
            if (std::get<double>(n.data) != 0)
                return false;
    }
    return true;
}
bool MuzzleValueShape(const Value& v, const State& state) {
    const auto* o = Fields(v, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    if (!o || o->empty())
        return o != nullptr;
    for (unsigned required : {1u, 2u, 3u, 4u, 5u, 6u, 8u, 9u, 10u})
        if (!o->contains(required))
            return false;
    if (!SkeletonValueShape(1, o->at(1)) || !Name(o->at(2)) || !Name(o->at(3)) || !UInt(o->at(4)) || !UInt(o->at(5)) ||
        !UInt(o->at(6)) || !TransformWireShape(o->at(8)) || !TransformWireShape(o->at(9)) ||
        !TransformWireShape(o->at(10)))
        return false;
    const auto binding = std::get<u64>(o->at(6).data);
    if (binding < 1 || binding > 3 || (binding != 3) != o->contains(7) || o->contains(7) && !Identity(o->at(7)))
        return false;
    const auto& source = std::get<Object>(o->at(1).data);
    if (std::get<u64>(o->at(4).data) >= std::get<u64>(source.at(5).data) ||
        std::get<u64>(o->at(5).data) >= std::get<u64>(source.at(6).data))
        return false;
    const auto coordinates = state.context.find(ContextGroup::Coordinates);
    if (coordinates == state.context.end() || !coordinates->second.fields.contains(3))
        return false;
    const auto* origin = std::get_if<Value::List>(&coordinates->second.fields.at(3).data);
    const auto& own_origin = std::get<Object>(source.at(8).data);
    if (!origin || origin->size() != 3)
        return false;
    for (unsigned i = 0; i < 3; ++i)
        if ((*origin)[i] != own_origin.at(i + 1))
            return false;
    const auto relative = TransformFromWire(o->at(8));
    if (binding == 3 && (!Near(relative, Transform{}) || o->at(2) != o->at(3)))
        return false;
    const auto bone_world = unreal::ComposeTransform(TransformFromWire(o->at(9)), TransformFromWire(source.at(7)));
    const auto expected = bone_world ? unreal::ComposeTransform(relative, *bone_world) : std::nullopt;
    return expected && Near(TransformFromWire(o->at(10)), *expected);
}
} // namespace monitor::synchronization
