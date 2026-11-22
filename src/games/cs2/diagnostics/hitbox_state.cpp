#include "hitbox_state.hpp"
#include "games/cs2/hitboxes/frame.hpp"

namespace mousebox::games::cs2
{
application::Json describeHitboxes(const Snapshot *frame, hitboxes::Display display)
{
    unsigned supported = 0, ready = 0, missing = 0, invalid = 0;
    if (display != hitboxes::Display::Off && frame)
        for (const auto &player : frame->players)
            if (player.body && player.body->entity.dormant != true)
            {
                const auto &body = *player.body;
                const auto *asset = body.skeletonModel && body.skeletonProfile
                                        ? hitboxes::findModel(frame->build, *body.skeletonProfile, *body.skeletonModel)
                                        : nullptr;
                if (asset)
                {
                    ++supported;
                    const auto pose = hitboxes::fromBody(*frame, body);
                    ready += static_cast<unsigned>(pose.hitboxes.size());
                    missing += pose.missing;
                    invalid += pose.invalid;
                    if (body.nativeHitboxSetAvailable != true)
                        missing += static_cast<unsigned>(asset->definitions.size());
                }
            }
    const auto phase = display == hitboxes::Display::Off ? "disabled"
                       : !frame                          ? "waiting-data"
                       : !supported                      ? "unsupported"
                       : !ready                          ? "waiting-pose"
                       : (missing || invalid)            ? "partial"
                                                         : "ready";
    return {{"phase", phase}, {"models", supported}, {"hitboxes", ready}, {"missing", missing}, {"invalid", invalid}};
}
} // namespace mousebox::games::cs2
