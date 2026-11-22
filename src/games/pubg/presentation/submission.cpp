#include "submission.hpp"
#include "games/pubg/model/source_sample.hpp"

namespace mousebox::games::pubg
{
namespace
{
struct State final : render::ProjectionState
{
    render::SceneProjector projector;
};
class Projection final : public render::SceneProjection
{
    const render::Scene scene_;

  public:
    explicit Projection(render::Scene scene) : scene_(std::move(scene)) {}
    bool accepts(const render::ProjectionState &state) const noexcept override
    {
        return dynamic_cast<const State *>(&state) != nullptr;
    }
    std::unique_ptr<render::ProjectionState> createState() const override
    {
        return std::make_unique<State>();
    }
    render::Primitives project(render::ProjectionState &state, float width, float height) const override
    {
        auto &owned = dynamic_cast<State &>(state);
        if (!scene_.living && !scene_.view)
            owned.projector = render::SceneProjector{};
        return owned.projector.project(scene_, width, height);
    }
};
} // namespace
application::ports::PresentationFrame makeSubmission(render::Scene scene)
{
    application::ports::PresentationFrame result;
    result.scene.revision = scene.revision;
    result.scene.palette = scene.palette;
    result.scene.english = scene.english;
    if (scene.living && scene.living->header.current && scene.living->header.consumerReceivedAtMs)
    {
        const auto &h = scene.living->header;
        result.scene.receipt = domain::SourceReceipt{
            1, h.reader, h.session, h.epoch, h.capture, h.consumerConnection, *h.consumerReceivedAtMs};
    }
    for (const auto &frame : {scene.living, scene.view})
        if (frame && frame->header.current && frame->header.consumerConnection)
            result.samples.push_back(sourceSample(frame->header));
    result.scene.projection = std::make_shared<const Projection>(std::move(scene));
    return result;
}
} // namespace mousebox::games::pubg
