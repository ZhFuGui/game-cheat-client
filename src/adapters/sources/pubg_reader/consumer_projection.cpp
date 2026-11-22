#include "control_projection.hpp"
#include "contract.hpp"
#include "detail/consumer_facts.hpp"
#include <cmath>

namespace mousebox::reader
{
namespace
{
using namespace consumer_detail;
void projectCharacter(const c::Entity &entity, const c::State &state, control::Frame &frame)
{
    if (!entity.id || entity.kind != monitor::EntityKind::Character)
        return;
    control::Character out;
    out.identity = control::Identity::entity(entity.id);
    out.nativeMovementExpected = state.context.native_world_velocity_supported;
    if (const auto spatial = value<c::Spatial>(property(entity, monitor::PropertyGroup::Spatial), state))
    {
        if (spatial->position)
            out.position = vector(*spatial->position);
        if (spatial->origin)
            out.positionOrigin = vector(*spatial->origin);
    }
    if (const auto flags = value<c::LivingState>(property(entity, monitor::PropertyGroup::CharacterState), state))
    {
        out.team = flags->team;
        out.isBot = flags->is_bot;
    }
    if (const auto name = value<c::Name>(property(entity, monitor::PropertyGroup::CharacterName), state))
        out.name = name->value.value_or("");
    const auto movementGroup = property(entity, monitor::PropertyGroup::CharacterMovement);
    if (const auto movement = value<c::Movement>(movementGroup, state); movement && movement->world)
    {
        const auto &world = *movement->world;
        const auto timing = source(*movementGroup, {4}, state);
        const auto owner = character(world.owner);
        if (timing && owner == std::optional(out.identity) && world.velocity && world.source && *world.source >= 1 &&
            *world.source <= 5)
            out.bodyWorldVelocity = control::BodyWorldVelocity{out.identity,
                                                               vector(*world.velocity),
                                                               ns(timing->source.started),
                                                               ns(timing->source.finished),
                                                               control::BodyVelocitySource::ReaderTotalWorld,
                                                               *world.source};
    }
    const auto skeletonGroup = property(entity, monitor::PropertyGroup::CharacterSkeleton);
    if (const auto skeleton = value<c::Skeleton>(skeletonGroup, state))
    {
        const auto owner = character(skeleton->owner);
        if (owner == std::optional(out.identity) && skeleton->origin && skeleton->layout_token &&
            *skeleton->layout_token && skeleton->pose_token && *skeleton->pose_token)
        {
            out.skeletonBinding =
                control::Character::SkeletonBinding{*owner, *skeleton->layout_token, *skeleton->pose_token};
            for (const auto &bone : skeleton->bones)
            {
                const auto role = control::boneRole(static_cast<unsigned>(bone.role));
                if (!role || !bone.world_position)
                    continue;
                control::Bone next;
                next.role = role;
                if (bone.parent_role)
                    next.parentRole = control::boneRole(static_cast<unsigned>(*bone.parent_role));
                next.name = control::boneRoleName(*role);
                next.position = vector(*bone.world_position);
                next.origin = vector(*skeleton->origin);
                next.sourceStartNs = ns(skeletonGroup->source.started);
                next.sourceFinishNs = ns(skeletonGroup->source.finished);
                if (*role == control::BoneRole::Head)
                {
                    out.head = next.position;
                    out.headOrigin = next.origin;
                }
                out.bones.push_back(std::move(next));
            }
        }
    }
    frame.characters.push_back(std::move(out));
}
} // namespace
control::Frame projectControlFrame(const monitor::synchronization::consumer::State &state,
                                   const monitor::synchronization::codec::Binding &binding)
{
    using namespace consumer_detail;
    control::Frame frame;
    auto &header = frame.header;
    header.contract = control::FrameContract::ConsumerV3;
    header.descriptorSha256 = ReaderContractDescriptorSha256;
    header.category =
        state.category == monitor::DataCategory::Local ? control::Category::Local : control::Category::Living;
    header.current = state.accepted && (state.category == monitor::DataCategory::Local ||
                                        state.category == monitor::DataCategory::Living);
    header.reader = binding.session.stream_id;
    header.session = binding.session.session_id;
    header.epoch = state.epoch;
    header.revision = state.revision;
    header.capture = state.capture_sequence;
    header.clockDomain = binding.clock.domain_id;
    header.mappingRevision = binding.clock.mapping_revision;
    header.sourceStartNs = ns(state.capture.started);
    header.sourceFinishNs = ns(state.capture.finished);
    if (!header.current)
        return frame;
    if (contextField(state, 1))
        header.world = state.context.world_token;
    if (contextField(state, 2))
        header.level = state.context.level_token;
    if (contextField(state, 4))
        header.convention = state.context.coordinate_convention;
    frame.presentationWorld = state.context.world_token;
    frame.presentationGameState = state.context.game_state_token;
    frame.verifiedMapName = state.context.map_name.value_or("");
    if (state.context.verified_meters_per_unit && std::isfinite(*state.context.verified_meters_per_unit) &&
        *state.context.verified_meters_per_unit > 0)
        frame.verifiedMetersPerUnit = state.context.verified_meters_per_unit;
    if (state.category == monitor::DataCategory::Local)
        projectLocal(state, frame);
    else
        for (const auto &[id, entity] : state.entities)
        {
            (void)id;
            projectCharacter(entity, state, frame);
        }
    return frame;
}
} // namespace mousebox::reader
