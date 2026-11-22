#include "synchronization/skeleton_validation.hpp"
#include "model/geometry.hpp"
#include "model/entities/body_skeleton.hpp"
#include <algorithm>
#include <limits>

namespace monitor::synchronization {
namespace {
using Object = Value::Object;
const Object* Fields(const Value& value, std::initializer_list<unsigned> allowed, std::size_t count = 0) {
    const auto* out = std::get_if<Object>(&value.data);
    if (!out || (count && out->size() != count))
        return nullptr;
    for (const auto& [field, v] : *out)
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
    return n && *n >= INT32_MIN && *n <= INT32_MAX;
}
bool Real(const Value& v) {
    const auto* n = std::get_if<double>(&v.data);
    return n && std::isfinite(*n) && std::abs(*n) <= std::numeric_limits<float>::max();
}
bool Vector(const Value& v) {
    const auto* o = Fields(v, {1, 2, 3}, 3);
    return o && Real(o->at(1)) && Real(o->at(2)) && Real(o->at(3));
}
Vec3 Position(const Value& v) {
    const auto& o = std::get<Object>(v.data);
    return {float(std::get<double>(o.at(1).data)), float(std::get<double>(o.at(2).data)),
            float(std::get<double>(o.at(3).data))};
}
bool TransformShape(const Value& v) {
    const auto* o = Fields(v, {1, 2, 3}, 3);
    if (!o || !Vector(o->at(2)) || !Vector(o->at(3)))
        return false;
    const auto* q = Fields(o->at(1), {1, 2, 3, 4}, 4);
    if (!q)
        return false;
    double norm = 0;
    for (const auto& [f, x] : *q) {
        if (!Real(x))
            return false;
        norm += std::get<double>(x.data) * std::get<double>(x.data);
    }
    return std::abs(norm - 1) <= .011;
}
Transform TransformData(const Value& v) {
    const auto& o = std::get<Object>(v.data);
    const auto& q = std::get<Object>(o.at(1).data);
    return {{float(std::get<double>(q.at(1).data)), float(std::get<double>(q.at(2).data)),
             float(std::get<double>(q.at(3).data)), float(std::get<double>(q.at(4).data))},
            Position(o.at(2)),
            Position(o.at(3))};
}
bool Identity(const Value& v) {
    const auto* o = Fields(v, {1, 2, 3}, 3);
    if (!o)
        return false;
    const auto* address = std::get_if<u64>(&o->at(1).data);
    return address && *address && Int(o->at(2)) && std::get<std::int64_t>(o->at(2).data) >= 0 && UInt(o->at(3));
}
bool Name(const Value& v) {
    const auto* o = Fields(v, {1, 2, 3, 4, 5});
    if (!o || o->empty())
        return false;
    for (const auto& [f, x] : *o)
        if (f == 5 ? !std::holds_alternative<std::string>(x.data) : !UInt(x))
            return false;
    return true;
}
bool BoneShape(const Value& v) {
    const auto* o = Fields(v, {1, 2, 3, 4, 5, 6});
    if (!o || !o->contains(1) || !UInt(o->at(1)) || !o->contains(6) || !Int(o->at(6)) ||
        std::get<std::int64_t>(o->at(6).data) < -1)
        return false;
    if (o->contains(2) && (!Int(o->at(2)) || std::get<std::int64_t>(o->at(2).data) < -1 ||
                           std::get<std::int64_t>(o->at(2).data) >= std::int64_t(std::get<u64>(o->at(1).data))))
        return false;
    if (o->contains(3) && !Name(o->at(3)))
        return false;
    if (o->contains(4) != o->contains(5) || (o->contains(4) && (!TransformShape(o->at(4)) || !Vector(o->at(5)) ||
                                                                std::get<std::int64_t>(o->at(6).data) < 0)))
        return false;
    return true;
}
bool Near(float a, float b) {
    return std::abs(a - b) <= std::max(.05f, std::abs(b) * std::numeric_limits<float>::epsilon() * 4);
}
} // namespace
bool TransformWireShape(const Value& value) {
    return TransformShape(value);
}
Transform TransformFromWire(const Value& value) {
    return TransformData(value);
}
bool SkeletonValueShape(unsigned field, const Value& value) {
    if (field == 1) {
        const auto* s = Fields(value, {1, 2, 3, 4, 5, 6, 7, 8}, 8);
        if (!s || !Identity(s->at(1)) || !Identity(s->at(2)) || !Identity(s->at(3)) || !UInt(s->at(4)) ||
            std::get<u64>(s->at(4).data) > 1 || !UInt(s->at(5)) || !UInt(s->at(6)) || !TransformShape(s->at(7)))
            return false;
        const auto* origin = Fields(s->at(8), {1, 2, 3}, 3);
        return origin && Int(origin->at(1)) && Int(origin->at(2)) && Int(origin->at(3));
    }
    if (field == 2) {
        const auto* bones = std::get_if<Value::List>(&value.data);
        if (!bones)
            return false;
        for (const auto& bone : *bones)
            if (!BoneShape(bone))
                return false;
        return true;
    }
    return false;
}
bool SkeletonGroupValid(const GroupState& group, const State& state) {
    if (group.fields.empty())
        return true;
    if (!group.fields.contains(1))
        return false;
    const auto& s = std::get<Object>(group.fields.at(1).data);
    const auto coords = state.context.find(ContextGroup::Coordinates);
    if (coords == state.context.end() || !coords->second.fields.contains(3))
        return false;
    const auto* origin = std::get_if<Value::List>(&coords->second.fields.at(3).data);
    const auto& own = std::get<Object>(s.at(8).data);
    if (!origin || origin->size() != 3)
        return false;
    for (unsigned i = 0; i < 3; ++i)
        if ((*origin)[i] != own.at(i + 1))
            return false;
    if (!group.fields.contains(2))
        return group.status == FactStatus::Partial;
    const auto& bones = std::get<Value::List>(group.fields.at(2).data);
    const auto source_count = std::get<u64>(s.at(5).data);
    if (bones.size() > BodyBoneNames.size() || (bones.empty() && source_count))
        return false;
    std::array<bool, BodyBoneNames.size()> seen{};
    u64 previous = 0;
    const auto world = TransformData(s.at(7));
    for (std::size_t i = 0; i < bones.size(); ++i) {
        const auto& bone = std::get<Object>(bones[i].data);
        const auto index = std::get<u64>(bone.at(1).data);
        if (index >= source_count || (i && index <= previous) || !bone.contains(3))
            return false;
        previous = index;
        const auto& name = std::get<Object>(bone.at(3).data);
        if (!name.contains(5))
            return false;
        const auto role = BodyBoneRole(std::get<std::string>(name.at(5).data));
        if (!role || seen[*role])
            return false;
        seen[*role] = true;
        if (bone.contains(4)) {
            if (u64(std::get<std::int64_t>(bone.at(6).data)) >= std::get<u64>(s.at(6).data))
                return false;
            const auto expected = TransformPosition(world, TransformData(bone.at(4)).translation);
            const auto actual = Position(bone.at(5));
            if (!Finite(expected) || !Near(actual.x, expected.x) || !Near(actual.y, expected.y) ||
                !Near(actual.z, expected.z))
                return false;
        }
        if (group.status == FactStatus::Available &&
            (!bone.contains(2) || !bone.contains(3) || !std::get<Object>(bone.at(3).data).contains(5) ||
             !bone.contains(4)))
            return false;
    }
    if (group.status == FactStatus::Available && source_count &&
        !std::all_of(seen.begin(), seen.end(), [](bool present) { return present; }))
        return false;
    return true;
}
} // namespace monitor::synchronization
