#pragma once
#include "primitives.hpp"
#include <functional>
#include "domain/source_receipt.hpp"

namespace mousebox::render
{
class ProjectionState
{
  public:
    virtual ~ProjectionState() = default;
};
// Immutable game-owned projection request. The renderer owns the only mutable
// state and invokes this port exclusively on its existing render thread.
class SceneProjection
{
  public:
    virtual ~SceneProjection() = default;
    virtual bool accepts(const ProjectionState &) const noexcept = 0;
    virtual std::unique_ptr<ProjectionState> createState() const = 0;
    virtual Primitives project(ProjectionState &, float width, float height) const = 0;
};
struct SceneSubmission
{
    std::shared_ptr<const SceneProjection> projection;
    std::uint64_t revision = 0;
    Palette palette;
    bool english = false;
    std::function<void(double localQpcMs)> completed;
    std::optional<domain::SourceReceipt> receipt;
};
} // namespace mousebox::render
