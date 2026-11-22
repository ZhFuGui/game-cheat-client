#include "module.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <unordered_map>
#include <limits>

namespace mousebox::games::pubg
{
namespace
{
using domain::Vec3;
constexpr std::pair<std::string_view, std::string_view> BodyPairs[]{
    {"pelvis", "spine_01"},   {"spine_01", "spine_02"},   {"spine_02", "spine_03"},     {"spine_03", "neck_01"},
    {"neck_01", "head"},      {"spine_03", "clavicle_l"}, {"clavicle_l", "upperarm_l"}, {"upperarm_l", "lowerarm_l"},
    {"lowerarm_l", "hand_l"}, {"spine_03", "clavicle_r"}, {"clavicle_r", "upperarm_r"}, {"upperarm_r", "lowerarm_r"},
    {"lowerarm_r", "hand_r"}, {"pelvis", "thigh_l"},      {"thigh_l", "calf_l"},        {"calf_l", "foot_l"},
    {"pelvis", "thigh_r"},    {"thigh_r", "calf_r"},      {"calf_r", "foot_r"}};
bool officialMap(std::string_view value)
{
    constexpr std::string_view Names[]{"Range_Main",     "Baltic_Main",     "Erangel_Main", "Desert_Main",
                                       "DihorOtok_Main", "Tiger_Main",      "Kiki_Main",    "Savage_Main",
                                       "Chimera_Main",   "Summerland_Main", "Heaven_Main",  "Neon_Main"};
    return std::ranges::find(Names, value) != std::end(Names);
}
Vec3 add(Vec3 value, Vec3 origin)
{
    return {value.x + origin.x, value.y + origin.y, value.z + origin.z};
}
scene::SourceStamp stamp(const control::Header &header)
{
    return {"pubg",
            header.reader,
            header.clockDomain,
            header.session,
            header.epoch,
            header.capture,
            header.mappingRevision,
            header.sourceStartNs,
            header.sourceFinishNs,
            header.sourceClock,
            header.current ? scene::FactStatus::Current : scene::FactStatus::Unavailable};
}
std::string key(const control::Identity &identity)
{
    return std::to_string(identity.id) + ":" + control::lifecycleKey(identity);
}
} // namespace
std::shared_ptr<const scene::Snapshot> observeScene(const control::Frame &frame)
{
    auto result = std::make_shared<scene::Snapshot>();
    result->source = stamp(frame.header);
    auto &space = result->coordinates;
    space.id = "pubg/" + frame.header.reader + "/" + std::to_string(frame.header.epoch) + "/" +
               std::to_string(frame.presentationWorld.value_or(frame.header.world.value_or(0)));
    space.originEpoch = frame.header.epoch;
    space.convention = frame.header.convention;
    if (frame.verifiedMetersPerUnit && std::isfinite(*frame.verifiedMetersPerUnit) && *frame.verifiedMetersPerUnit > 0)
    {
        space.metersPerUnit = frame.verifiedMetersPerUnit;
        space.scaleSource = scene::ScaleSource::Source;
    }
    else if (officialMap(frame.verifiedMapName))
    {
        space.metersPerUnit = .01;
        space.scaleSource = scene::ScaleSource::GameProfile;
    }
    const auto point = [&](Vec3 value, Vec3 origin) {
        return scene::Position{add(value, origin), space.id, space.originEpoch};
    };
    if (frame.camera && frame.cameraOrigin && frame.rotation && frame.fov && domain::finite(*frame.camera) &&
        domain::finite(*frame.cameraOrigin) && domain::finite(*frame.rotation) && std::isfinite(*frame.fov) &&
        *frame.fov > 0 && *frame.fov < 180 && domain::finite(add(*frame.camera, *frame.cameraOrigin)))
    {
        constexpr double radians = std::numbers::pi / 180;
        const auto rotation = *frame.rotation;
        const double sp = std::sin(rotation.x * radians), cp = std::cos(rotation.x * radians),
                     sy = std::sin(rotation.y * radians), cy = std::cos(rotation.y * radians),
                     sr = std::sin(rotation.z * radians), cr = std::cos(rotation.z * radians);
        auto positionStamp = result->source, orientationStamp = result->source;
        positionStamp.startedNs = frame.cameraSourceStartNs;
        positionStamp.finishedNs = frame.cameraSourceFinishNs;
        orientationStamp.startedNs = frame.rotationSourceStartNs;
        orientationStamp.finishedNs = frame.rotationSourceFinishNs;
        result->camera = scene::CameraPose{point(*frame.camera, *frame.cameraOrigin),
                                           {cp * cy, cp * sy, sp},
                                           {sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp},
                                           {-cr * sp * cy - sr * sy, -cr * sp * sy + sr * cy, cr * cp},
                                           *frame.fov,
                                           std::move(positionStamp),
                                           std::move(orientationStamp)};
    }
    result->entities.reserve(frame.characters.size());
    for (const auto &character : frame.characters)
    {
        scene::Entity entity;
        entity.key =
            space.id + "/session/" + std::to_string(frame.header.session) + "/entity/" + key(character.identity);
        if (character.position && character.positionOrigin && domain::finite(*character.position) &&
            domain::finite(*character.positionOrigin) &&
            domain::finite(add(*character.position, *character.positionOrigin)))
            entity.actorPosition = point(*character.position, *character.positionOrigin);
        std::unordered_map<unsigned, std::size_t> indexed;
        std::unordered_map<std::string_view, std::size_t> named;
        constexpr auto ambiguous = std::numeric_limits<std::size_t>::max();
        for (std::size_t i = 0; i < character.bones.size(); ++i)
        {
            const auto &bone = character.bones[i];
            const auto jointId = bone.role ? static_cast<unsigned>(*bone.role) : bone.index;
            entity.joints.push_back({jointId, point(bone.position, bone.origin), bone.sourceStartNs,
                                     bone.sourceFinishNs, bone.name == "head"});
            auto [ix, unique] = indexed.emplace(jointId, i);
            if (!unique)
                ix->second = ambiguous;
            auto [name, one] = named.emplace(bone.name, i);
            if (!one)
                name->second = ambiguous;
        }
        for (const auto &[parentName, childName] : BodyPairs)
        {
            const auto parent = named.find(parentName), child = named.find(childName);
            if (parent == named.end() || child == named.end() || parent->second == ambiguous ||
                child->second == ambiguous)
                continue;
            const auto &a = character.bones[parent->second], &b = character.bones[child->second];
            if (a.role || b.role)
            {
                const auto &binding = character.skeletonBinding;
                if (frame.header.contract != control::FrameContract::ConsumerV3 || !binding ||
                    binding->owner.kind != control::IdentityKind::EntityToken || binding->owner != character.identity ||
                    !control::hasLifecycle(binding->owner) || !binding->layoutToken || !binding->poseToken || !a.role ||
                    !b.role || b.parentRole != a.role || control::boneRoleName(*a.role) != a.name ||
                    control::boneRoleName(*b.role) != b.name ||
                    indexed[static_cast<unsigned>(*a.role)] != parent->second ||
                    indexed[static_cast<unsigned>(*b.role)] != child->second)
                    continue;
            }
            else if (!b.parent || *b.parent < 0 || static_cast<unsigned>(*b.parent) >= b.index ||
                     b.poseIndex.value_or(-1) < 0 || a.poseIndex.value_or(-1) < 0 ||
                     indexed[a.index] != parent->second || indexed[b.index] != child->second ||
                     static_cast<unsigned>(*b.parent) != a.index)
                continue;
            entity.edges.emplace_back(parent->second, child->second);
        }
        result->entities.push_back(std::move(entity));
    }
    return result;
}
} // namespace mousebox::games::pubg
