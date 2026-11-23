#include "observation.hpp"
#include "games/valorant/presentation/identity.hpp"
#include <cmath>

namespace mousebox::games::valorant::aim
{
std::shared_ptr<const Frame> controlFrame(const sources::valorant_reader::Observation &source)
{
    for (unsigned i = 0; i < source.channels.size(); ++i)
    {
        const auto &channel = source.channels[i];
        if (!channel.connected || !channel.data || channel.data->stream != i + 1 ||
            channel.instance != source.channels[0].instance ||
            channel.data->capture.epoch != source.channels[0].data->capture.epoch)
            return {};
    }
    const auto &positions = *source.channels[0].data, &bones = *source.channels[1].data,
               &view = *source.channels[2].data;
    if (!view.camera || !view.camera->position || !view.camera->rotation || view.camera->projection != 1 ||
        std::abs(view.camera->rotation->z) > .01 || positions.positions.size() > 4096)
        return {};
    auto out = std::make_shared<Frame>();
    out->identity = {source.channels[0].instance, source.channels[0].sample.clockDomain, positions.capture.epoch,
                     positions.context.localEntity.value_or(0)};
    for (unsigned i = 0; i < 3; ++i)
    {
        out->samples[i] = source.channels[i].sample;
        out->identity.connections[i] = source.channels[i].connection;
    }
    out->localResolved = positions.context.localState == 3 && positions.context.localEntity &&
                         positions.positions.contains(*positions.context.localEntity);
    out->revision = source.revision;
    out->cameraCapture = view.capture.sequence;
    const auto origin = *view.camera->position, rotation = *view.camera->rotation;
    out->origin = {origin.x, origin.y, origin.z};
    out->viewDegrees = {rotation.y, rotation.x};
    constexpr unsigned Parts[]{5, 4, 3, 1};
    out->targets.reserve(positions.positions.size());
    for (const auto &[id, position] : positions.positions)
    {
        Target target;
        target.id = id;
        target.relationship = relationship(id, positions.context, &position);
        // No life state is supplied by this Reader revision. It remains unknown.
        if (const auto found = bones.skeletons.find(id); found != bones.skeletons.end() && found->second)
            for (unsigned part = 0; part < std::size(Parts); ++part)
                if (const auto &point = (*found->second)[Parts[part]])
                    target.points[part] = domain::Vec3{point->x, point->y, point->z};
        out->targets.push_back(std::move(target));
    }
    return out;
}
} // namespace mousebox::games::valorant::aim
