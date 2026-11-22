#include "frame.hpp"
#include <algorithm>

namespace mousebox::games::cs2::hitboxes
{
namespace
{
const ModelAsset *bodyModel(const Snapshot &frame, const Body &body)
{
    if (body.nativeHitboxSetAvailable != true || !body.skeletonModel || !body.skeletonProfile ||
        (body.skeletonStatus != SkeletonStatus::VerifiedComplete &&
         body.skeletonStatus != SkeletonStatus::VerifiedPartial))
        return {};
    return findModel(frame.build, *body.skeletonProfile, *body.skeletonModel);
}
BoneTransform boneTransform(const Bone &bone)
{
    return {bone.index, bone.position, bone.worldRotation, bone.worldUniformScale};
}
} // namespace
Pose fromBody(const Snapshot &frame, const Body &body)
{
    const auto *model = bodyModel(frame, body);
    if (!model)
        return {};
    std::vector<BoneTransform> bones;
    bones.reserve(body.bones.size());
    for (const auto &bone : body.bones)
        bones.push_back(boneTransform(bone));
    return prepare(model, bones);
}
std::optional<PosedHitbox> fromBody(const Snapshot &frame, const Body &body, unsigned id)
{
    const auto *model = bodyModel(frame, body);
    if (!model)
        return {};
    const auto definition = std::find_if(model->definitions.begin(), model->definitions.end(),
                                         [id](const Definition &box) { return box.id == id; });
    if (definition == model->definitions.end())
        return {};
    const auto matches = [&](const Bone &bone) { return bone.index == definition->bone; };
    const auto bone = std::find_if(body.bones.begin(), body.bones.end(), matches);
    if (bone == body.bones.end() || std::find_if(bone + 1, body.bones.end(), matches) != body.bones.end())
        return {};
    return poseHitbox(*definition, boneTransform(*bone));
}
bool retainsEligibility(const Snapshot &analyzed, const Snapshot &current)
{
    for (const auto &previous : analyzed.players)
        if (previous.body)
        {
            const auto pose = fromBody(analyzed, *previous.body);
            if (pose.hitboxes.empty())
                continue;
            const auto found = std::find_if(current.players.begin(), current.players.end(), [&](const Player &p) {
                return p.controller == previous.controller && p.body && p.body->entity.id == previous.body->entity.id;
            });
            if (found == current.players.end() || found->body->nativeHitboxSetAvailable != true ||
                found->body->skeletonModel != previous.body->skeletonModel ||
                found->body->skeletonProfile != previous.body->skeletonProfile)
                return false;
            const auto fresh = fromBody(current, *found->body);
            for (const auto &box : pose.hitboxes)
                if (std::none_of(fresh.hitboxes.begin(), fresh.hitboxes.end(),
                                 [&](const PosedHitbox &p) { return p.definition->id == box.definition->id; }))
                    return false;
        }
    return true;
}
} // namespace mousebox::games::cs2::hitboxes
