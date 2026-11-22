#include "adapters/sources/pubg_reader/control_projection.hpp"
#include "contract.hpp"
#include "detail/local_projection.hpp"
#include <algorithm>
#include <cctype>
#include <climits>
#include <chrono>
#include <cmath>
#include <limits>

namespace mousebox::reader
{
// Protocol field access and Local-only projection stay inside the Reader adapter.
using namespace detail;
namespace s = monitor::synchronization;
using monitor::u64;
control::Frame projectControlFrame(const s::State &state, const s::codec::Binding &binding)
{
    control::Frame f;
    auto &h = f.header;
    h.descriptorSha256 = ReaderContractDescriptorSha256;
    h.category =
        state.batch.category == monitor::DataCategory::Local ? control::Category::Local : control::Category::Living;
    h.current = state.accepted && (state.batch.category == monitor::DataCategory::Living ||
                                   state.batch.category == monitor::DataCategory::Local);
    h.reader = binding.session.stream_id;
    h.session = binding.session.session_id;
    h.epoch = state.epoch;
    h.revision = state.revision;
    h.capture = state.capture_sequence;
    h.clockDomain = binding.clock.domain_id;
    h.mappingRevision = binding.clock.mapping_revision;
    h.sourceStartNs =
        std::chrono::duration_cast<std::chrono::nanoseconds>(state.capture.started.time_since_epoch()).count();
    h.sourceFinishNs =
        std::chrono::duration_cast<std::chrono::nanoseconds>(state.capture.finished.time_since_epoch()).count();
    const auto world = context(state, s::ContextGroup::World), coords = context(state, s::ContextGroup::Coordinates);
    h.world = integer(fresh(state, world, 2));
    h.level = integer(fresh(state, world, 6));
    h.convention = text(fresh(state, coords, 1));
    if (coords)
    {
        const auto scale = number(field(coords->fields, 2));
        if (scale && std::isfinite(*scale) && *scale > 0)
            f.verifiedMetersPerUnit = scale;
    }
    // A validated Map may carry an explicitly reused independent sample. Bind
    // it only to this World, never to a remembered map from another epoch.
    if (world)
    {
        f.presentationWorld = integer(field(world->fields, 2));
        f.presentationGameState = integer(field(world->fields, 4));
    }
    if (const auto map = context(state, s::ContextGroup::Map);
        map && f.presentationWorld && *f.presentationWorld && integer(field(map->fields, 1)) == f.presentationWorld)
        if (const auto name = object(field(map->fields, 2)))
            f.verifiedMapName = text(field(*name, 5));
    if (world)
    {
        const auto flag = [&](unsigned id) {
            const auto v = field(world->fields, id);
            const auto b = v ? std::get_if<bool>(&v->data) : nullptr;
            return b && *b;
        };
        h.worldRootUnverified = flag(9);
        h.worldCacheUsed = flag(10);
    }
    if (h.category == control::Category::Local)
    {
        // This independently sampled controller state stays diagnostic. Do not
        // replace the final camera or infer a recoil/breath angle by subtraction.
        if (const auto raw = context(state, s::ContextGroup::LocalControllerRotation))
        {
            const auto controller = instance(fresh(state, raw, 1));
            const auto rotation = vector(fresh(state, raw, 2));
            const auto stamp = raw->field_sources.find(2);
            if (controller && rotation && stamp != raw->field_sources.end())
                f.controllerRotation = control::ControllerRotationSample{
                    *controller, *rotation, ns(stamp->second.sample.started), ns(stamp->second.sample.finished)};
        }
        const auto source = context(state, s::ContextGroup::ViewSource),
                   pose = context(state, s::ContextGroup::ViewPose);
        f.cameraSourceAvailable = source != nullptr;
        f.controller = integer(fresh(state, source, 1));
        if (!fresh(state, source, 2))
            f.controller.reset();
        f.camera = vector(fresh(state, pose, 1));
        f.rotation = vector(fresh(state, pose, 2));
        f.fov = number(fresh(state, pose, 3));
        f.cameraOrigin = vector(fresh(state, pose, 4));
        if (f.camera && pose)
        {
            const auto stamp = pose->field_sources.find(1);
            if (stamp != pose->field_sources.end())
            {
                f.cameraSourceStartNs = ns(stamp->second.sample.started);
                f.cameraSourceFinishNs = ns(stamp->second.sample.finished);
            }
        }
        if (f.rotation && pose)
        {
            const auto stamp = pose->field_sources.find(2);
            if (stamp != pose->field_sources.end())
            {
                f.rotationSourceStartNs = ns(stamp->second.sample.started);
                f.rotationSourceFinishNs = ns(stamp->second.sample.finished);
            }
        }
        const auto local = context(state, s::ContextGroup::LocalControl);
        const auto controller = integer(fresh(state, local, 4));
        const auto reference = object(fresh(state, local, 11));
        if (f.controller != controller)
            f.controller.reset();
        if (controller && reference && text(field(*reference, 1)) == h.reader &&
            integer(field(*reference, 2)) == h.epoch && integer(field(*reference, 4)) == 1)
        {
            const auto id = integer(field(*reference, 3));
            auto key = instance(field(*reference, 5));
            if (id && *id && key && integer(fresh(state, local, 7)) == key->actor &&
                integer(fresh(state, local, 9)) == id)
            {
                key->id = *id;
                f.self = *key;
            }
        }
        if (f.self)
        {
            f.activeWeapon = activeWeapon(state, *f.self);
            if (const auto spectators = context(state, s::ContextGroup::LocalSpectators))
            {
                const auto owner = instance(fresh(state, spectators, 1));
                const auto count = integer(fresh(state, spectators, 2));
                if (owner && sameInstance(*owner, *f.self) && count && *count <= INT32_MAX)
                    f.spectatedCount = static_cast<unsigned>(*count);
            }
        }
        projectNativeObservations(state, f);
        return f;
    }
    for (const auto &[id, e] : state.entities)
    {
        (void)id;
        if (e.key.kind != monitor::EntityKind::Character)
            continue;
        const auto verifiedIdentity = property(e, monitor::PropertyGroup::Identity, h.capture);
        if (!verifiedIdentity)
            continue;
        control::Character c;
        c.identity = identity(e);
        c.team = team(e, h.capture);
        if (const auto spatial = property(e, monitor::PropertyGroup::Spatial, h.capture))
        {
            c.position = vector(field(spatial->fields, 1));
            c.positionOrigin = vector(field(spatial->fields, 2));
        }
        if (const auto characterState = property(e, monitor::PropertyGroup::CharacterState, h.capture))
            if (const auto value = field(characterState->fields, 2))
                if (const auto bot = std::get_if<bool>(&value->data))
                    c.isBot = *bot;
        if (const auto contract = binding.contract.find(monitor::EntityKind::Character);
            contract != binding.contract.end())
            c.nativeMovementExpected = contract->second.enabled.contains(monitor::PropertyGroup::CharacterMovement);
        if (const auto movement = property(e, monitor::PropertyGroup::CharacterMovement, h.capture))
        {
            const auto velocity = vector(field(movement->fields, 1));
            const auto component = object(field(movement->fields, 2));
            const auto actor = component ? integer(field(*component, 1)) : std::nullopt;
            const auto index = component ? integer(field(*component, 2)) : std::nullopt;
            const auto serial = component ? integer(field(*component, 3)) : std::nullopt;
            if (velocity && actor && *actor && index && *index <= INT32_MAX && serial && *serial <= UINT32_MAX)
            {
                control::NativeMovement value;
                value.componentVelocity = *velocity;
                value.component = {0, *actor, static_cast<std::int32_t>(*index), static_cast<std::uint32_t>(*serial)};
                value.rawComponent = integer(field(movement->fields, 3));
                value.sourceStartNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                          movement->source_sample.started.time_since_epoch())
                                          .count();
                value.sourceFinishNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                           movement->source_sample.finished.time_since_epoch())
                                           .count();
                c.nativeMovement = value;
            }
            const auto sample = nested(*movement, {4}, h.capture);
            const auto worldValue = object(field(movement->fields, 4));
            if (sample && worldValue)
            {
                const auto total = vector(field(*worldValue, 1));
                const auto source = integer(field(*worldValue, 2));
                const auto owner = instance(field(*worldValue, 3));
                if (total && source && *source >= 1 && *source <= 5 && owner && owner->actor == c.identity.actor &&
                    owner->index == c.identity.index && owner->serial == c.identity.serial)
                    c.bodyWorldVelocity = control::BodyWorldVelocity{c.identity,
                                                                     *total,
                                                                     ns(sample->source.started),
                                                                     ns(sample->source.finished),
                                                                     control::BodyVelocitySource::ReaderTotalWorld,
                                                                     static_cast<unsigned>(*source)};
            }
        }
        if (const auto name = property(e, monitor::PropertyGroup::CharacterName, h.capture))
            c.name = text(field(name->fields, 1));
        if (const auto skeleton = property(e, monitor::PropertyGroup::CharacterSkeleton, h.capture))
        {
            const auto source = object(field(skeleton->fields, 1));
            const auto bonesValue = field(skeleton->fields, 2);
            const auto bones = bonesValue ? std::get_if<s::Value::List>(&bonesValue->data) : nullptr;
            if (source && bones)
                for (const auto &bone : *bones)
                    if (const auto b = object(&bone))
                    {
                        const auto n = object(field(*b, 3));
                        auto name = n ? text(field(*n, 5)) : "";
                        std::transform(name.begin(), name.end(), name.begin(),
                                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                        const auto position = vector(field(*b, 5)), origin = vector(field(*source, 8));
                        const auto index = integer(field(*b, 1));
                        if (position && origin && index && *index <= std::numeric_limits<unsigned>::max())
                        {
                            control::Bone point;
                            point.index = static_cast<unsigned>(*index);
                            point.position = *position;
                            point.origin = *origin;
                            point.name = name;
                            point.sourceStartNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                      skeleton->source_sample.started.time_since_epoch())
                                                      .count();
                            point.sourceFinishNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                       skeleton->source_sample.finished.time_since_epoch())
                                                       .count();
                            if (const auto parent = number(field(*b, 2)))
                                point.parent = static_cast<int>(*parent);
                            if (const auto pose = number(field(*b, 6)))
                                point.poseIndex = static_cast<int>(*pose);
                            c.bones.push_back(std::move(point));
                        }
                        if (name == "head")
                        {
                            c.head = position;
                            c.headOrigin = origin;
                        }
                    }
        }
        f.characters.push_back(std::move(c));
    }
    return f;
}
void bindLocalControl(control::Frame &living, const control::Frame *local)
{
    living.self.reset();
    living.selfTeam.reset();
    living.controller.reset();
    living.activeWeapon.reset();
    living.spectatedCount.reset();
    if (!local || !living.header.current || !local->header.current ||
        living.header.category != control::Category::Living || local->header.category != control::Category::Local ||
        !local->self || !local->controller)
        return;
    const auto &a = living.header;
    const auto &b = local->header;
    if (a.contract != b.contract || a.reader.empty() || a.reader != b.reader || !a.epoch || a.epoch != b.epoch ||
        !a.world || a.world != b.world || !a.level || a.level != b.level || a.convention.empty() ||
        a.convention != b.convention || a.clockDomain.empty() || a.clockDomain != b.clockDomain || !a.mappingRevision ||
        a.mappingRevision != b.mappingRevision)
        return;
    const auto found = std::find_if(living.characters.begin(), living.characters.end(),
                                    [&](const auto &c) { return c.identity == *local->self; });
    if (found == living.characters.end())
        return;
    living.self = found->identity;
    living.selfTeam = found->team;
    living.controller = local->controller;
    if (local->activeWeapon && local->activeWeapon->owner == *living.self)
        living.activeWeapon = local->activeWeapon;
    living.spectatedCount = local->spectatedCount;
}
} // namespace mousebox::reader
