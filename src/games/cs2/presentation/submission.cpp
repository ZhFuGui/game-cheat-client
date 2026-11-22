#include "submission.hpp"
#include "cover_geometry.hpp"
#include "remaining_damage_label.hpp"
#include "head_damage_label.hpp"
#include "best_damage.hpp"
#include "application/storage/json_store.hpp"
#include "render/esp_scale.hpp"
#include "render/triangle_mesh.hpp"
#include "games/cs2/hitboxes/frame.hpp"
#include "games/cs2/hitboxes/geometry.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
namespace mousebox::games::cs2
{
std::optional<render::Point> projectPoint(const std::array<double, 16> &m, Vec3 p, float w, float h)
{
    auto x = m[0] * p.x + m[1] * p.y + m[2] * p.z + m[3], y = m[4] * p.x + m[5] * p.y + m[6] * p.z + m[7],
         z = m[12] * p.x + m[13] * p.y + m[14] * p.z + m[15];
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || z <= 0.001 || w <= 0 || h <= 0)
        return {};
    x = (x / z + 1) * w / 2;
    y = (1 - y / z) * h / 2;
    if (!std::isfinite(x) || !std::isfinite(y) || std::abs(x) > 10000000 || std::abs(y) > 10000000)
        return {};
    return render::Point{static_cast<float>(x), static_cast<float>(y)};
}
namespace
{
domain::Vec3 sceneVector(Vec3 p)
{
    return {p.x, p.y, p.z};
}
struct State final : render::ProjectionState
{
    std::weak_ptr<const Snapshot> meshFrame;
    std::shared_ptr<const render::TriangleMesh> mesh;
    std::shared_ptr<const extensions::CoverResult> cover;
    render::EspOptions options;
    render::MeshViewport viewport;
    hitboxes::Display mode = hitboxes::Display::Off;
};
class Projection final : public render::SceneProjection
{
    std::shared_ptr<const Snapshot> frame_;
    application::ports::PresentationOptions options_;
    double scale_;
    std::shared_ptr<const extensions::CoverResult> cover_;
    hitboxes::Display hitboxDisplay_;
    std::shared_ptr<const extensions::RemainingDamageResult> damage_;
    std::shared_ptr<const extensions::HeadDamageResult> headDamage_;
    std::shared_ptr<const extensions::BestDamageResult> bestDamage_;

  public:
    Projection(std::shared_ptr<const Snapshot> f, application::ports::PresentationOptions o, double scale,
               std::shared_ptr<const extensions::CoverResult> cover, hitboxes::Display mode,
               std::shared_ptr<const extensions::RemainingDamageResult> damage,
               std::shared_ptr<const extensions::HeadDamageResult> headDamage,
               std::shared_ptr<const extensions::BestDamageResult> bestDamage)
        : frame_(std::move(f)), options_(o), scale_(scale), cover_(std::move(cover)), hitboxDisplay_(mode),
          damage_(std::move(damage)), headDamage_(std::move(headDamage)), bestDamage_(std::move(bestDamage))
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
    render::Primitives project(render::ProjectionState &projectionState, float width, float height) const override
    {
        render::Primitives result;
        auto out = std::make_shared<render::EspPrimitives>();
        result.esp = out;
        out->options = options_.esp;
        out->options.aimRange = false;
        out->options.targetHighlight = false;
        out->options.bots = true;
        out->viewport = {0, 0, width, height};
        double aspect = 0;
        switch (options_.esp.aspect)
        {
        case render::EspAspect::Wide:
            aspect = 16.0 / 9;
            break;
        case render::EspAspect::StandardWide:
            aspect = 16.0 / 10;
            break;
        case render::EspAspect::UltraWide:
            aspect = 21.0 / 9;
            break;
        case render::EspAspect::Classic:
            aspect = 4.0 / 3;
            break;
        default:
            break;
        }
        if (aspect > 0 && height > 0)
        {
            if (width / height > aspect)
            {
                out->viewport.width = static_cast<float>(height * aspect);
                out->viewport.x = (width - out->viewport.width) / 2;
            }
            else
            {
                out->viewport.height = static_cast<float>(width / aspect);
                out->viewport.y = (height - out->viewport.height) / 2;
            }
        }
        out->summary.units = scale_ > 0 ? render::EspUnits::RadarProfile : render::EspUnits::Unknown;
        if (!frame_ || !frame_->local.matrix)
        {
            result.message = options_.english ? L"Waiting for CS2 Reader matrix" : L"等待 CS2 Reader 当前矩阵";
            return result;
        }
        out->summary.state = render::EspState::Ready;
        out->summary.viewCapture = out->summary.livingCapture = frame_->sequence;
        const render::MatrixProjectionScale projectionScale(*frame_->local.matrix, out->viewport.width,
                                                            out->viewport.height);
        const auto convert = [&](Vec3 point) {
            auto p = projectPoint(*frame_->local.matrix, point, out->viewport.width, out->viewport.height);
            if (p)
            {
                p->x += out->viewport.x;
                p->y += out->viewport.y;
            }
            return p;
        };
        const auto inside = [&](render::Point p) {
            return p.x >= out->viewport.x && p.y >= out->viewport.y && p.x <= out->viewport.x + out->viewport.width &&
                   p.y <= out->viewport.y + out->viewport.height;
        };
        std::optional<unsigned> selfTeam;
        for (const auto &p : frame_->players)
            if (frame_->local.controller && p.controller == *frame_->local.controller)
                selfTeam = p.team ? p.team : (p.body ? p.body->entity.team : std::nullopt);
        out->summary.counts.input = static_cast<unsigned>(frame_->players.size());
        auto &cached = static_cast<State &>(projectionState);
        const render::MeshViewport meshViewport{out->viewport.x, out->viewport.y, out->viewport.width,
                                                out->viewport.height};
        std::optional<render::MeshBuilder> mesh;
        if (hitboxDisplay_ == hitboxes::Display::Off)
        {
            cached.mesh.reset();
            cached.meshFrame.reset();
            cached.cover.reset();
        }
        else if (cached.meshFrame.lock() == frame_ && cached.mesh && cached.cover == cover_ &&
                 cached.options == options_.esp && cached.mode == hitboxDisplay_ && cached.viewport == meshViewport)
            out->mesh = cached.mesh;
        else
            mesh.emplace(*frame_->local.matrix, meshViewport, hitboxDisplay_ == hitboxes::Display::Wireframe);
        for (const auto &p : frame_->players)
        {
            if (!p.body)
            {
                ++out->summary.counts.invalid;
                continue;
            }
            const auto &body = *p.body;
            const bool self = frame_->local.controller && p.controller == *frame_->local.controller;
            auto team = p.team ? p.team : body.entity.team;
            const bool knownTeam = selfTeam && team;
            const bool teammate = knownTeam && *selfTeam == *team;
            if ((self && !options_.esp.self) || (!self && knownTeam && teammate && !options_.esp.teammates) ||
                (!self && knownTeam && !teammate && !options_.esp.opponents) ||
                (body.entity.dormant && *body.entity.dormant))
            {
                ++out->summary.counts.filtered;
                continue;
            }
            std::optional<double> distance;
            if (scale_ > 0 && frame_->local.eye && body.entity.position)
            {
                const auto &a = *frame_->local.eye;
                const auto &b = *body.entity.position;
                distance = std::hypot(a.x - b.x, a.y - b.y, a.z - b.z) * scale_;
            }
            if (options_.esp.maximumDistanceMeters > 0 && (!distance || *distance > options_.esp.maximumDistanceMeters))
            {
                ++out->summary.counts.filtered;
                continue;
            }
            const unsigned group = self ? 0 : knownTeam ? (teammate ? 1 : 2) : 0;
            const auto *visibility = !self && knownTeam && !teammate ? coverActor(cover_.get(), *frame_, p) : nullptr;
            if (mesh)
            {
                const auto pose = hitboxes::fromBody(*frame_, body);
                for (const auto &hitbox : pose.hitboxes)
                {
                    const auto fragments =
                        visibility
                            ? visibility->hitboxes.find(hitbox.definition->id)
                            : std::map<unsigned, std::vector<extensions::ActorVisibility::Surface>>::const_iterator{};
                    if (visibility && fragments != visibility->hitboxes.end())
                        for (const auto &fragment : fragments->second)
                            mesh->triangle({sceneVector(fragment.triangle[0]), sceneVector(fragment.triangle[1]),
                                            sceneVector(fragment.triangle[2])},
                                           fragment.visibility == extensions::Visibility::Visible
                                               ? 0xffd400
                                               : render::EspColors[group]);
                    else
                        for (const auto &triangle : hitboxes::worldSurface(hitbox))
                            mesh->triangle(
                                {sceneVector(triangle[0]), sceneVector(triangle[1]), sceneVector(triangle[2])},
                                render::EspColors[group]);
                }
            }
            std::vector<render::Point> projected;
            std::optional<Vec3> firstProjectedWorld;
            const bool verifiedSkeleton = body.skeletonStatus == SkeletonStatus::VerifiedPartial ||
                                          body.skeletonStatus == SkeletonStatus::VerifiedComplete;
            render::SkeletonReference reference;
            std::unordered_map<unsigned, const Bone *> bonesByNativeIndex;
            if (verifiedSkeleton)
            {
                bonesByNativeIndex.reserve(body.bones.size());
                for (const auto &bone : body.bones)
                {
                    reference.point(sceneVector(bone.position));
                    bonesByNativeIndex.emplace(bone.index, &bone);
                }
                for (const auto &link : body.boneLinks)
                {
                    const auto a = bonesByNativeIndex.find(link.parent), b = bonesByNativeIndex.find(link.child);
                    if (a != bonesByNativeIndex.end() && b != bonesByNativeIndex.end())
                        reference.edge(sceneVector(a->second->position), sceneVector(b->second->position),
                                       a->second->role == BoneRole::Neck && b->second->role == BoneRole::Head);
                }
            }
            const auto referenceSpan = reference.span();
            const auto scaleAt = [&](Vec3 position) {
                return render::espScale(projectionScale.at(sceneVector(position)).value_or(0), referenceSpan);
            };
            const auto *best = bestDamageActor(bestDamage_.get(), *frame_, p);
            if (best)
                if (const auto point = bestDamagePoint(*best, *frame_, p))
                    appendBestDamage(*out, *best, *point, convert, scaleAt);
            if (verifiedSkeleton)
            {
                std::unordered_map<unsigned, render::Point> pointsByNativeIndex;
                pointsByNativeIndex.reserve(body.bones.size());
                for (const auto &bone : body.bones)
                    if (auto point = convert(bone.position); point && inside(*point))
                    {
                        projected.push_back(*point);
                        if (!firstProjectedWorld)
                            firstProjectedWorld = bone.position;
                        pointsByNativeIndex.emplace(bone.index, *point);
                        if (options_.esp.bones)
                        {
                            appendBonePoint(*out, group, {*point, scaleAt(bone.position).jointRadius}, visibility,
                                            bone.index);
                            ++result.visiblePoints;
                        }
                    }
                // Bone.role carries explicit semantics; it is never a native index or a topology source.
                // Only this batch's declared parent/child endpoints can form a line. HEAD remains a point.
                if (options_.esp.bones)
                    for (const auto &link : body.boneLinks)
                    {
                        const auto parent = pointsByNativeIndex.find(link.parent);
                        const auto child = pointsByNativeIndex.find(link.child);
                        if (parent != pointsByNativeIndex.end() && child != pointsByNativeIndex.end())
                            appendBoneLine(*out, group, *bonesByNativeIndex.at(link.parent),
                                           *bonesByNativeIndex.at(link.child), parent->second, child->second,
                                           visibility, convert, scaleAt);
                    }
            }
            auto anchorWorld = body.entity.position;
            auto anchor = anchorWorld ? convert(*anchorWorld) : std::nullopt;
            if (!anchor && !projected.empty())
            {
                anchor = projected.front();
                anchorWorld = firstProjectedWorld;
            }
            if (!anchor || !inside(*anchor))
            {
                if (projected.empty())
                {
                    ++out->summary.counts.outside;
                    continue;
                }
                anchor = projected.front();
                anchorWorld = firstProjectedWorld;
            }
            ++out->summary.counts.visible;
            render::EspMarkerPrimitive marker;
            marker.point = *anchor;
            marker.group = group;
            marker.id = p.controller;
            marker.annotationScale = anchorWorld ? scaleAt(*anchorWorld).annotation : 1;
            const auto annotation = marker.annotationScale;
            marker.bounds = {anchor->x - 2 * annotation, anchor->y - 2 * annotation, 4 * annotation, 4 * annotation};
            if (projected.size() > 1)
            {
                float left = projected[0].x, right = left, top = projected[0].y, bottom = top;
                for (auto point : projected)
                {
                    left = std::min(left, point.x);
                    right = std::max(right, point.x);
                    top = std::min(top, point.y);
                    bottom = std::max(bottom, point.y);
                }
                marker.bounds = {left, top, std::max(1.0f, right - left), std::max(1.0f, bottom - top)};
            }
            if (options_.esp.names && p.name)
            {
                try
                {
                    marker.name = application::storage::wide(*p.name);
                }
                catch (const std::exception &)
                {
                }
            }
            if (options_.esp.distance)
                marker.distanceMeters = distance;
            if (options_.esp.health)
            {
                if (body.entity.health && *body.entity.health >= 0)
                    marker.health = *body.entity.health;
                if (body.entity.maxHealth && *body.entity.maxHealth >= 0)
                    marker.maxHealth = *body.entity.maxHealth;
            }
            marker.extraLabel = remainingDamageLabel(damage_.get(), *frame_, p, options_.english);
            if (best)
                marker.emphasis =
                    render::EspMarkerPrimitive::Emphasis{bestDamageLabel(*best, options_.english), bestDamageColor};
            if (const auto label = headDamageLabel(headDamage_.get(), *frame_, p, options_.english); !label.empty())
            {
                if (!marker.extraLabel.empty())
                    marker.extraLabel += L"\n";
                marker.extraLabel += label;
            }
            out->markers.push_back(std::move(marker));
        }
        if (mesh)
        {
            cached.meshFrame = frame_;
            cached.cover = cover_;
            cached.options = options_.esp;
            cached.viewport = meshViewport;
            cached.mode = hitboxDisplay_;
            cached.mesh = std::make_shared<const render::TriangleMesh>(mesh->finish());
            out->mesh = cached.mesh;
        }
        if (!out->summary.counts.visible)
            result.message = options_.english ? L"No CS2 entities in viewport" : L"当前视野没有可见 CS2 人物";
        return result;
    }
};
} // namespace
application::ports::PresentationFrame makeSubmission(std::shared_ptr<const Snapshot> frame,
                                       const application::ports::PresentationOptions &options, double scale,
                                       std::uint64_t revision, std::optional<domain::SourceSample> sample,
                                       std::shared_ptr<const extensions::CoverResult> cover, hitboxes::Display mode,
                                       std::shared_ptr<const extensions::RemainingDamageResult> damage,
                                       std::shared_ptr<const extensions::HeadDamageResult> headDamage,
                                       std::shared_ptr<const extensions::BestDamageResult> bestDamage,
                                       std::optional<domain::SourceReceipt> receipt)
{
    application::ports::PresentationFrame result;
    result.scene.palette = options.palette;
    result.scene.english = options.english;
    result.scene.revision = revision;
    if (frame && receipt && receipt->reader == frame->instance && receipt->epoch == frame->epoch &&
        receipt->capture == frame->sequence)
        result.scene.receipt = std::move(receipt);
    result.scene.projection =
        std::make_shared<const Projection>(std::move(frame), options, scale, std::move(cover), mode, std::move(damage),
                                           std::move(headDamage), std::move(bestDamage));
    if (sample)
        result.samples.push_back(*sample);
    return result;
}
} // namespace mousebox::games::cs2
