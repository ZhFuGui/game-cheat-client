#pragma once
#include "domain/math.hpp"
#include <memory>
#include <string_view>

namespace mousebox::application::ports
{
enum class Visibility
{
    Unknown,
    Visible,
    Occluded
};
// Geometry-only capability. It exposes no process memory, input or device port.
class VisibilityScene
{
  public:
    virtual ~VisibilityScene() = default;
    virtual Visibility query(domain::Vec3 origin, domain::Vec3 target) const = 0;
};
class VisibilityProvider
{
  public:
    virtual ~VisibilityProvider() = default;
    virtual bool available() const noexcept = 0;
    virtual std::shared_ptr<const VisibilityScene> load(std::string_view map, std::string_view resource) = 0;
};
} // namespace mousebox::application::ports
