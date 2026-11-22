#include "viewpoint_state.hpp"
#include "games/cs2/model/snapshot.hpp"

namespace mousebox::games::cs2
{
application::Json describeViewpoint(const Snapshot &frame)
{
    using application::Json;
    const Body *body = nullptr;
    if (frame.directoryVerified && frame.local.controller && frame.local.pawn)
        for (const auto &player : frame.players)
            if (player.controller == *frame.local.controller && player.body &&
                player.body->entity.id == *frame.local.pawn)
            {
                body = &*player.body;
                break;
            }
    const auto vector = [](const std::optional<Vec3> &value) -> Json {
        return value ? Json::array({value->x, value->y, value->z}) : Json{};
    };
    return {{"readOnly", true},
            {"capture", std::to_string(frame.sequence)},
            {"pawn", frame.local.pawn ? Json(std::to_string(*frame.local.pawn)) : Json{}},
            {"bodyMatched", body != nullptr},
            {"inputAngles", vector(frame.local.angles)},
            {"pawnInputAngles", body ? vector(body->inputEyeAngles) : Json{}},
            {"readerEyePosition", vector(frame.local.eye)},
            {"bodyPosition", body ? vector(body->entity.position) : Json{}},
            {"rawViewOffset", body ? vector(body->viewOffset) : Json{}},
            {"bodyRotation", body ? vector(body->entity.absoluteRotation) : Json{}},
            {"bodyScale", body && body->entity.absoluteScale ? Json(*body->entity.absoluteScale) : Json{}}};
}
} // namespace mousebox::games::cs2
