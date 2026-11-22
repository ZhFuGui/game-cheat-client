#include "esp_scene.hpp"
#include "games/pubg/module.hpp"
#include "render/esp_scale.hpp"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <string_view>
#include <unordered_map>

namespace mousebox::render
{
namespace
{
using control::Vec3;
Vec3 add(Vec3 a, Vec3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
Vec3 delta(Vec3 a, Vec3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
double dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
bool inside(Point p, EspRect r)
{
    return p.x >= r.x && p.x <= r.x + r.width && p.y >= r.y && p.y <= r.y + r.height;
}
bool intersects(EspRect a, EspRect b)
{
    return a.x + a.width >= b.x && a.x <= b.x + b.width && a.y + a.height >= b.y && a.y <= b.y + b.height;
}
EspRect viewport(float w, float h, EspAspect a)
{
    if (!std::isfinite(w) || !std::isfinite(h) || w <= 0 || h <= 0)
        return {};
    const double aspect = a == EspAspect::Wide           ? 16. / 9
                          : a == EspAspect::StandardWide ? 16. / 10
                          : a == EspAspect::UltraWide    ? 21. / 9
                          : a == EspAspect::Classic      ? 4. / 3
                                                         : 0;
    if (!aspect)
        return {0, 0, w, h};
    const float width = static_cast<float>(std::min<double>(w, h * aspect)),
                height = static_cast<float>(width / aspect);
    return {(w - width) / 2, (h - height) / 2, width, height};
}
struct CameraPoint
{
    double horizontal, vertical, depth;
};
struct Projection
{
    Vec3 camera, forward, right, up;
    double focal;
    EspRect rect;
    CameraPoint cameraSpace(Vec3 p) const
    {
        const auto d = delta(p, camera);
        return {dot(d, right), dot(d, up), dot(d, forward)};
    }
    std::optional<Point> point(CameraPoint p) const
    {
        if (!std::isfinite(p.depth) || p.depth <= 0)
            return {};
        const double x = rect.x + rect.width / 2. + focal * p.horizontal / p.depth,
                     y = rect.y + rect.height / 2. - focal * p.vertical / p.depth;
        if (!std::isfinite(x) || !std::isfinite(y) || std::abs(x) > std::numeric_limits<float>::max() ||
            std::abs(y) > std::numeric_limits<float>::max())
            return {};
        return Point{static_cast<float>(x), static_cast<float>(y)};
    }
    std::optional<Circle> headCircle(Vec3 head, Vec3 neck, double referenceSpan) const
    {
        const auto p = cameraSpace(head);
        const auto center = point(p);
        const auto span = delta(head, neck);
        // Estimate the head's diameter from the proven neck-head edge. This is
        // a camera-facing decoration, not a hitbox; both size and depth share
        // the same world units, so no guessed meter scale is needed.
        const double radius = focal * (std::hypot(span.x, span.y, span.z) / 2 / p.depth);
        if (!center || !std::isfinite(radius) || radius <= 0 || radius > std::numeric_limits<float>::max())
            return {};
        const auto projectedRadius = static_cast<float>(radius);
        if (projectedRadius <= 0)
            return {};
        return Circle{*center, projectedRadius, espScale(focal / p.depth, referenceSpan).boneWidth};
    }
    std::optional<ScaledLine> segment(Vec3 a, Vec3 b, double referenceSpan) const
    {
        const auto first = cameraSpace(a), last = cameraSpace(b);
        if (!std::isfinite(first.horizontal) || !std::isfinite(first.vertical) || !std::isfinite(first.depth) ||
            !std::isfinite(last.horizontal) || !std::isfinite(last.vertical) || !std::isfinite(last.depth))
            return {};
        // Numerical near-plane margin, never a meter-distance visibility limit.
        const double depthMargin =
            std::max(std::numeric_limits<double>::denorm_min(),
                     std::numeric_limits<double>::epsilon() * std::max(std::abs(first.depth), std::abs(last.depth)));
        const auto planes = [&](CameraPoint p) {
            return std::array{p.depth - depthMargin, rect.width / 2. * p.depth + focal * p.horizontal,
                              rect.width / 2. * p.depth - focal * p.horizontal,
                              rect.height / 2. * p.depth + focal * p.vertical,
                              rect.height / 2. * p.depth - focal * p.vertical};
        };
        const auto from = planes(first), to = planes(last);
        double enter = 0, leave = 1;
        for (unsigned i = 0; i < from.size(); ++i)
        {
            if (!std::isfinite(from[i]) || !std::isfinite(to[i]) || (from[i] < 0 && to[i] < 0))
                return {};
            if (from[i] < 0 || to[i] < 0)
            {
                const auto at = from[i] / (from[i] - to[i]);
                if (from[i] < 0)
                    enter = std::max(enter, at);
                else
                    leave = std::min(leave, at);
                if (enter > leave)
                    return {};
            }
        }
        const auto at = [&](double t) -> std::optional<Point> {
            auto p = point({(1 - t) * first.horizontal + t * last.horizontal,
                            (1 - t) * first.vertical + t * last.vertical, (1 - t) * first.depth + t * last.depth});
            if (p)
            {
                p->x = std::clamp(p->x, rect.x, rect.x + rect.width);
                p->y = std::clamp(p->y, rect.y, rect.y + rect.height);
            }
            return p;
        };
        const auto p = at(enter), q = at(leave);
        const auto widthAt = [&](double t) {
            return espScale(focal / ((1 - t) * first.depth + t * last.depth), referenceSpan).boneWidth;
        };
        return p && q ? std::optional(ScaledLine{*p, *q, widthAt(enter), widthAt(leave)}) : std::nullopt;
    }
};
std::wstring safeName(const control::Character &c)
{
    const auto n =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, c.name.data(), static_cast<int>(c.name.size()), nullptr, 0);
    std::wstring name(n > 0 ? n : 0, L' ');
    if (n > 0)
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, c.name.data(), static_cast<int>(c.name.size()), name.data(),
                            n);
    std::erase_if(name, [](wchar_t ch) {
        return ch < 0x20 || (ch >= 0x7f && ch <= 0x9f) || (ch >= 0x202a && ch <= 0x202e) ||
               (ch >= 0x2066 && ch <= 0x2069);
    });
    const auto first = name.find_first_not_of(L" \t\r\n"), last = name.find_last_not_of(L" \t\r\n");
    return first == std::wstring::npos ? std::to_wstring(c.identity.actor ? c.identity.actor : c.identity.id)
                                       : name.substr(first, last - first + 1);
}
} // namespace
void EspProjector::connect(const std::shared_ptr<const control::Frame> &frame)
{
    if (previous_ == frame)
    {
        // Legacy/offline callers may replace people or topology in place.
        // The scene owns its matching joints and edges as one current value.
        if (frame && !frame->scene)
            geometry_ = games::pubg::observeScene(*frame);
        return;
    }
    previous_ = frame;
    if (!frame)
        return;
    geometry_ = frame->scene ? frame->scene : games::pubg::observeScene(*frame);
}
std::shared_ptr<const EspPrimitives> EspProjector::project(const Scene &scene, float width, float height)
{
    auto result = std::make_shared<EspPrimitives>();
    auto &r = *result;
    r.options = scene.esp.value_or(EspOptions{});
    r.viewport = viewport(width, height, r.options.aspect);
    r.bearing = bearingLayout(width, height);
    r.spectators = scene.spectators;
    auto &summary = r.summary;
    const auto &v = scene.view;
    const auto &l = scene.living;
    if (l)
        summary.livingCapture = l->header.capture;
    if (v)
        summary.viewCapture = v->header.capture;
    if (!v)
        return result;
    if (!v->header.current || !v->cameraSourceAvailable || !v->camera || !v->cameraOrigin || !v->rotation || !v->fov ||
        !control::finite(*v->camera) || !control::finite(*v->cameraOrigin) || !control::finite(*v->rotation) ||
        !std::isfinite(*v->fov) || *v->fov <= 0 || *v->fov >= 180 ||
        !control::finite(add(*v->camera, *v->cameraOrigin)))
    {
        summary.state = EspState::InvalidCamera;
        return result;
    }
    summary.fov = *v->fov;
    summary.state = EspState::WaitingLiving;
    constexpr double rad = std::numbers::pi / 180;
    const double focal = (r.options.fovAxis == EspFovAxis::Horizontal ? r.viewport.width : r.viewport.height) / 2. /
                         std::tan(*v->fov * rad / 2);
    if (r.options.aimRange && std::isfinite(scene.aimConeDegrees) && scene.aimConeDegrees > 0 &&
        scene.aimConeDegrees < 90)
    {
        const double radius = focal * std::tan(scene.aimConeDegrees * rad);
        if (std::isfinite(radius) && radius > 0 && radius <= std::numeric_limits<float>::max())
            r.aimRangeRadius = static_cast<float>(radius);
    }
    if (!l || !l->header.current)
        return result;
    summary.state = EspState::WorldMismatch;
    const auto livingWorld = l->presentationWorld.value_or(l->header.world.value_or(0)),
               viewWorld = v->presentationWorld.value_or(v->header.world.value_or(0));
    if (l->header.reader != v->header.reader || l->header.epoch != v->header.epoch ||
        (livingWorld && viewWorld && livingWorld != viewWorld))
        return result;
    summary.state = EspState::Ready;
    if (r.viewport.width <= 0)
        return result;
    connect(l);
    if (previousView_ != v || !v->scene)
    {
        previousView_ = v;
        cameraScene_ = v->scene ? v->scene : games::pubg::observeScene(*v);
    }
    if (!cameraScene_ || !cameraScene_->camera)
        return result;
    const auto &camera = *cameraScene_->camera;
    const auto meters = geometry_->coordinates.metersPerUnit;
    if (geometry_->coordinates.scaleSource == scene::ScaleSource::Source)
        summary.units = EspUnits::Reader;
    else if (geometry_->coordinates.scaleSource == scene::ScaleSource::GameProfile)
        summary.units = EspUnits::RadarProfile;
    const auto rot = *v->rotation;
    const Projection projection{camera.position.value, camera.forward, camera.right, camera.up, focal, r.viewport};
    if (!std::isfinite(projection.focal) || projection.focal <= 0)
        return result;
    r.bearing.ready = true;
    for (std::size_t ci = 0; ci < l->characters.size(); ++ci)
    {
        const auto &c = l->characters[ci];
        const auto &geometry = geometry_->entities[ci];
        auto &counts = summary.counts;
        ++counts.input;
        const auto selfIdentity = l->self ? l->self : scene.espTeam ? scene.espTeam->self : std::nullopt;
        const auto selfTeam = l->selfTeam && *l->selfTeam >= 0 ? l->selfTeam
                              : scene.espTeam                  ? scene.espTeam->team
                                                               : std::nullopt;
        auto team = c.team && *c.team >= 0 ? c.team : std::nullopt;
        if (!team && scene.espTeam &&
            std::ranges::find(scene.espTeam->members, c.identity) != scene.espTeam->members.end())
            team = scene.espTeam->team;
        const bool self = selfIdentity && c.identity == *selfIdentity,
                   teammate = !self && selfTeam && team && team == selfTeam, bot = c.isBot.value_or(false);
        if (self       ? !r.options.self
            : teammate ? (!r.options.teammates || (bot && !r.options.bots))
            : bot      ? !r.options.bots
                       : !r.options.opponents)
        {
            ++counts.filtered;
            continue;
        }
        const auto worldPosition = geometry.actorPosition ? std::optional(geometry.actorPosition->value) : std::nullopt;
        std::optional<double> distance;
        if (worldPosition && meters)
        {
            const auto d = delta(*worldPosition, projection.camera);
            const double n = std::hypot(d.x, d.y, d.z) * *meters;
            if (std::isfinite(n))
                distance = n;
        }
        // Cull the entire character before generating bones, labels or target highlights.
        const auto limit = r.options.maximumDistanceMeters;
        if (limit != 0 && (!std::isfinite(limit) || limit < 0 || !distance || *distance > limit))
        {
            ++counts.filtered;
            continue;
        }
        const bool selected = r.options.targetHighlight && scene.target && scene.target->identity == c.identity;
        const bool locked = selected && scene.heldTarget && *scene.heldTarget == c.identity;
        const unsigned group = selected   ? (locked ? EspLockedGroup : EspPreselectedGroup)
                               : self     ? 0
                               : teammate ? 1
                               : bot      ? 3
                                          : 2;
        if (!self && worldPosition)
            appendBearing(r.bearing, projection.camera, *worldPosition, rot.y, group, c.identity.id);
        SkeletonReference reference;
        for (const auto &joint : geometry.joints)
            reference.point(joint.position.value);
        for (const auto &[a, b] : geometry.edges)
            reference.edge(geometry.joints[a].position.value, geometry.joints[b].position.value,
                           geometry.joints[b].head);
        const double referenceSpan = reference.span();
        const auto scaleAt = [&](Vec3 position) {
            const auto depth = projection.cameraSpace(position).depth;
            return espScale(depth > 0 ? focal / depth : 0, referenceSpan);
        };
        bool bonesVisible = false;
        if (r.options.bones)
        {
            for (const auto &[a, b] : geometry.edges)
            {
                if (const auto line = projection.segment(geometry.joints[a].position.value,
                                                         geometry.joints[b].position.value, referenceSpan))
                {
                    r.skeletons[group].push_back(*line);
                    bonesVisible = true;
                    if (geometry.joints[b].head)
                        if (const auto head = projection.headCircle(geometry.joints[b].position.value,
                                                                    geometry.joints[a].position.value, referenceSpan);
                            head && inside(head->center, r.viewport))
                            r.heads[group].push_back(*head);
                }
            }
            // Only real semantic body joints participating in proven edges
            // receive the existing small joint dots.
            if (bonesVisible)
            {
                std::vector<bool> used(c.bones.size());
                for (const auto &[a, b] : geometry.edges)
                {
                    used[a] = true;
                    used[b] = true;
                }
                for (std::size_t i = 0; i < used.size(); ++i)
                    if (used[i])
                        if (const auto p = projection.point(projection.cameraSpace(geometry.joints[i].position.value));
                            p && inside(*p, r.viewport))
                        {
                            r.dots[group].push_back({*p, scaleAt(geometry.joints[i].position.value).jointRadius});
                        }
            }
        }
        if (selected && scene.target->position && scene.target->origin)
            if (const auto p =
                    projection.point(projection.cameraSpace(add(*scene.target->position, *scene.target->origin)));
                p && inside(*p, r.viewport))
            {
                r.aim = *p;
                r.targetLocked = locked;
                r.aimAnnotationScale = scaleAt(add(*scene.target->position, *scene.target->origin)).annotation;
            }
        const auto fail = [&](unsigned &count) {
            if (bonesVisible)
                ++counts.visible;
            else
                ++count;
        };
        if (!worldPosition)
        {
            fail(counts.invalid);
            continue;
        }
        const auto position = *worldPosition;
        const auto point = projection.point(projection.cameraSpace(position));
        if (!point)
        {
            fail(counts.behind);
            continue;
        }
        const auto annotation = scaleAt(position).annotation;
        EspRect bounds{point->x - 8 * annotation, point->y - 8 * annotation, 16 * annotation, 16 * annotation};
        bool estimated = false;
        if (r.options.marker == EspMarker::Estimated && meters && std::isfinite(r.options.estimatedHeightMeters) &&
            r.options.estimatedHeightMeters > 0)
        {
            const double hh = r.options.estimatedHeightMeters / *meters / 2, hw = hh / 3;
            double left = std::numeric_limits<double>::max(), right = -left, top = left, bottom = right;
            bool valid = true;
            for (int side : {-1, 1})
                for (int up : {-1, 1})
                {
                    const auto p = projection.point(projection.cameraSpace(
                        {position.x + side * hw * projection.right.x, position.y + side * hw * projection.right.y,
                         position.z + side * hw * projection.right.z + up * hh}));
                    if (!p)
                    {
                        valid = false;
                        continue;
                    }
                    left = std::min<double>(left, p->x);
                    right = std::max<double>(right, p->x);
                    top = std::min<double>(top, p->y);
                    bottom = std::max<double>(bottom, p->y);
                }
            if (valid && std::isfinite(right - left) && std::isfinite(bottom - top) &&
                right - left <= std::numeric_limits<float>::max() && bottom - top <= std::numeric_limits<float>::max())
            {
                bounds = {static_cast<float>(left), static_cast<float>(top), static_cast<float>(right - left),
                          static_cast<float>(bottom - top)};
                estimated = true;
            }
        }
        if (!intersects(bounds, r.viewport))
        {
            fail(counts.outside);
            continue;
        }
        r.markers.push_back({group, *point, bounds, estimated, safeName(c), distance, c.identity.id, annotation});
        ++counts.visible;
    }
    return result;
}
} // namespace mousebox::render
