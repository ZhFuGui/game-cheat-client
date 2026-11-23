#include "submission.hpp"
#include "identity.hpp"
#include "render/camera_projection.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numbers>
#include <set>
#include <sstream>

namespace mousebox::games::valorant
{
namespace
{
using namespace sources::valorant_reader;
domain::Vec3 vector(Vec3 p)
{
    return {p.x, p.y, p.z};
}
render::CameraBasis basis(const Camera &camera)
{
    const auto r = *camera.rotation;
    constexpr auto rad = std::numbers::pi / 180;
    const auto sp = std::sin(r.x * rad), cp = std::cos(r.x * rad), sy = std::sin(r.y * rad), cy = std::cos(r.y * rad),
               sr = std::sin(r.z * rad), cr = std::cos(r.z * rad);
    return {vector(*camera.position),
            {cp * cy, cp * sy, sp},
            {sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp},
            {-cr * sp * cy - sr * sy, -cr * sp * sy + sr * cy, cr * cp}};
}
render::EspRect viewport(float width, float height, render::EspAspect selection, const Camera &camera)
{
    double aspect = selection == render::EspAspect::Wide           ? 16. / 9
                    : selection == render::EspAspect::StandardWide ? 16. / 10
                    : selection == render::EspAspect::UltraWide    ? 21. / 9
                    : selection == render::EspAspect::Classic      ? 4. / 3
                                                                   : 0;
    if (!aspect && camera.constrainAspect.value_or(false))
        aspect = camera.aspect.value_or(0);
    if (!std::isfinite(aspect) || aspect <= 0)
        return {0, 0, width, height};
    const auto w = static_cast<float>(std::min<double>(width, height * aspect)), h = static_cast<float>(w / aspect);
    return {(width - w) / 2, (height - h) / 2, w, h};
}
struct State final : render::ProjectionState
{
};
class Projection final : public render::SceneProjection
{
    Observation source_;
    application::ports::PresentationOptions options_;
    VisualSettings settings_;
    bool matching(const Channel &channel) const
    {
        const auto &view = source_.channels[2];
        return channel.data && view.data && channel.instance == view.instance &&
               channel.data->capture.epoch == view.data->capture.epoch;
    }

  public:
    Projection(Observation source, application::ports::PresentationOptions options, VisualSettings settings)
        : source_(std::move(source)), options_(options), settings_(settings)
    {
    }
    bool accepts(const render::ProjectionState &s) const noexcept override
    {
        return dynamic_cast<const State *>(&s) != nullptr;
    }
    std::unique_ptr<render::ProjectionState> createState() const override
    {
        return std::make_unique<State>();
    }
    render::Primitives project(render::ProjectionState &, float width, float height) const override
    {
        render::Primitives result;
        auto out = std::make_shared<render::EspPrimitives>();
        result.esp = out;
        out->options = options_.esp;
        out->options.aimRange = out->options.targetHighlight = out->options.health = false;
        out->viewport = {0, 0, width, height};
        if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0)
            return result;
        const auto &view = source_.channels[2].data;
        if (!view || !view->camera)
            return result;
        const auto &camera = *view->camera;
        out->summary.viewCapture = view->capture.sequence;
        out->summary.state = render::EspState::InvalidCamera;
        if (!camera.position || !camera.rotation || !camera.constrainAspect || !camera.projection ||
            (camera.projection == 1 && !camera.fov) || (camera.projection == 2 && !camera.orthoWidth) ||
            (*camera.constrainAspect && (!camera.aspect || *camera.aspect <= 0)))
            return result;
        out->viewport = viewport(width, height, options_.esp.aspect, camera);
        const auto projection =
            render::CameraProjection::create(basis(camera), out->viewport, camera.fov.value_or(0), options_.esp.fovAxis,
                                             camera.projection == 2 ? camera.orthoWidth : std::nullopt);
        if (!projection)
            return result;
        out->summary.fov = camera.fov.value_or(0);
        out->summary.units = settings_.metersPerUnit > 0 ? render::EspUnits::RadarProfile : render::EspUnits::Unknown;
        out->summary.state = render::EspState::WaitingLiving;
        const auto &positions = source_.channels[0], &bones = source_.channels[1];
        const bool hasPositions = matching(positions), hasBones = matching(bones);
        if (!hasPositions && !hasBones)
            return result;
        out->summary.state = render::EspState::Ready;
        out->summary.livingCapture = hasPositions ? positions.data->capture.sequence : bones.data->capture.sequence;
        const PositionContext empty;
        const auto &context = hasPositions ? positions.data->context : empty;
        std::set<std::uint64_t> ids;
        if (hasPositions)
            for (const auto &[id, p] : positions.data->positions)
                ids.insert(id);
        if (hasBones)
            for (const auto &[id, p] : bones.data->skeletons)
                ids.insert(id);
        out->summary.counts.input = static_cast<unsigned>(ids.size());
        const auto inside = [&](render::Point p) {
            const auto v = out->viewport;
            return p.x >= v.x && p.x <= v.x + v.width && p.y >= v.y && p.y <= v.y + v.height;
        };
        constexpr auto &parents = JointParents;
        for (const auto id : ids)
        {
            const Position *position = nullptr;
            if (hasPositions)
            {
                const auto found = positions.data->positions.find(id);
                if (found != positions.data->positions.end())
                    position = &found->second;
            }
            const auto relation = relationship(id, context, position);
            if (!included(relation, settings_))
            {
                ++out->summary.counts.filtered;
                continue;
            }
            const unsigned group = relation == 1 || relation == 2 ? 1 : relation == 3 ? 2 : 0;
            const Skeleton *skeleton = nullptr;
            if (hasBones)
            {
                const auto found = bones.data->skeletons.find(id);
                if (found != bones.data->skeletons.end() && found->second)
                    skeleton = &*found->second;
            }
            std::optional<Vec3> origin = position ? position->world : std::nullopt;
            if (!origin && skeleton)
                for (const auto &point : *skeleton)
                    if (point)
                    {
                        origin = point;
                        break;
                    }
            if (!origin)
            {
                ++out->summary.counts.invalid;
                continue;
            }
            const auto c = *camera.position;
            const auto distance = std::hypot(origin->x - c.x, origin->y - c.y, origin->z - c.z);
            if (!std::isfinite(distance))
            {
                ++out->summary.counts.invalid;
                continue;
            }
            if (options_.esp.maximumDistanceMeters > 0 &&
                (settings_.metersPerUnit <= 0 ||
                 distance * settings_.metersPerUnit > options_.esp.maximumDistanceMeters))
            {
                ++out->summary.counts.filtered;
                continue;
            }
            std::vector<render::Point> visible;
            const auto collect = [&](std::optional<render::Point> p) {
                if (p && inside(*p))
                    visible.push_back(*p);
            };
            collect(projection->point(vector(*origin)));
            if (skeleton)
                for (unsigned joint = 0; joint < skeleton->size(); ++joint)
                {
                    const auto &point = (*skeleton)[joint];
                    if (!point)
                        continue;
                    const auto projected = projection->point(vector(*point));
                    collect(projected);
                    if (options_.esp.bones && projected && inside(*projected))
                        out->dots[group].push_back({*projected, 1.8f, 1});
                    const auto parent = parents[joint];
                    if (parent < 0 || !(*skeleton)[parent])
                        continue;
                    const auto edge = projection->segment(vector(*(*skeleton)[parent]), vector(*point));
                    if (edge)
                    {
                        visible.push_back(edge->a);
                        visible.push_back(edge->b);
                        if (options_.esp.bones)
                            out->skeletons[group].push_back({edge->a, edge->b, 1.3f, 1.3f});
                    }
                }
            if (visible.empty())
            {
                ++out->summary.counts.outside;
                continue;
            }
            float left = visible.front().x, right = left, top = visible.front().y, bottom = top;
            for (auto p : visible)
            {
                left = std::min(left, p.x);
                right = std::max(right, p.x);
                top = std::min(top, p.y);
                bottom = std::max(bottom, p.y);
            }
            render::EspMarkerPrimitive marker;
            marker.id = id;
            marker.group = group;
            marker.point = visible.front();
            marker.bounds = {left, top, std::max(1.f, right - left), std::max(1.f, bottom - top)};
            marker.name = L"#" + std::to_wstring(id);
            if (settings_.metersPerUnit > 0)
            {
                const auto meters = distance * settings_.metersPerUnit;
                if (!std::isfinite(meters))
                {
                    ++out->summary.counts.invalid;
                    continue;
                }
                marker.distanceMeters = meters;
            }
            else if (options_.esp.distance)
            {
                std::wostringstream text;
                text << std::fixed << std::setprecision(0) << distance << L" u";
                marker.extraLabel = text.str();
            }
            out->markers.push_back(std::move(marker));
            ++out->summary.counts.visible;
        }
        return result;
    }
};
} // namespace
application::ports::PresentationFrame makeSubmission(Observation source,
                                                     const application::ports::PresentationOptions &options,
                                                     const VisualSettings &settings, std::uint64_t revision)
{
    application::ports::PresentationFrame result;
    result.scene.revision = revision;
    result.scene.palette = options.palette;
    result.scene.english = options.english;
    const auto &view = source.channels[2];
    if (view.data)
        for (const auto &channel : source.channels)
            if (channel.data && channel.instance == view.instance &&
                channel.data->capture.epoch == view.data->capture.epoch)
            {
                result.samples.push_back(channel.sample);
                if (channel.receipt &&
                    (!result.scene.receipt || channel.receipt->receivedAtMs > result.scene.receipt->receivedAtMs))
                    result.scene.receipt = channel.receipt;
            }
    result.scene.projection = std::make_shared<const Projection>(std::move(source), options, settings);
    return result;
}
} // namespace mousebox::games::valorant
