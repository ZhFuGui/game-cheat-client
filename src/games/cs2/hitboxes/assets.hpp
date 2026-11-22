#pragma once
#include "games/cs2/model/geometry.hpp"
#include <span>
#include <string_view>

namespace mousebox::games::cs2::hitboxes
{
enum class Shape
{
    Box,
    Sphere,
    Capsule
};
enum class Display
{
    Off,
    Solid,
    Wireframe
};
struct Definition
{
    unsigned id = 0, bone = 0, group = 0;
    Shape shape = Shape::Capsule;
    Vec3 a, b;
    double radius = 0;
    bool translationOnly = false;
};
struct ModelAsset
{
    std::string_view name, resourceSha256, set;
    std::span<const Definition> definitions;
};
// Only explicitly reviewed model/build/profile combinations can use an asset.
const ModelAsset *findModel(unsigned build, std::string_view profile, std::string_view name) noexcept;
std::span<const ModelAsset> reviewedModels() noexcept;
} // namespace mousebox::games::cs2::hitboxes
