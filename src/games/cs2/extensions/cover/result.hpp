#pragma once
#include "application/extensions/contracts.hpp"
#include "application/ports/scene_visibility.hpp"
#include "games/cs2/model/geometry.hpp"
#include <array>
#include <map>

namespace mousebox::games::cs2::extensions
{
using application::ports::Visibility;
struct Span
{
    double start = 0, end = 1;
    Visibility visibility = Visibility::Unknown;
};
struct ActorVisibility
{
    std::uint32_t controller = 0, pawn = 0;
    std::map<unsigned, Visibility> bones;
    std::map<std::pair<unsigned, unsigned>, std::vector<Span>> links;
    struct Surface
    {
        std::array<Vec3, 3> triangle;
        Visibility visibility = Visibility::Unknown;
    };
    std::map<unsigned, std::vector<Surface>> hitboxes;
};
struct CoverResult final : application::extensions::Attachment
{
    application::extensions::Scope scope;
    std::uint64_t capture = 0;
    std::vector<ActorVisibility> actors;
    const ActorVisibility *find(std::uint32_t controller, std::uint32_t pawn) const noexcept
    {
        for (const auto &a : actors)
            if (a.controller == controller && a.pawn == pawn)
                return &a;
        return nullptr;
    }
};
} // namespace mousebox::games::cs2::extensions
