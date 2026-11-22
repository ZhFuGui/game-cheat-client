#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <optional>

namespace mousebox::adapters::traceline
{
struct FileIdentity
{
    std::string file, sha256;
    std::uintmax_t bytes = 0;
};
struct SceneEntry : FileIdentity
{
    std::string variant;
    std::uint64_t fingerprint = 0;
    std::uint32_t triangles = 0, solids = 0, closedSolids = 0, nodes = 0, flags = 0;
};
struct MapEntry
{
    std::string name, resource;
    std::vector<SceneEntry> scenes;
};
class Catalog
{
    std::filesystem::path root_;

  public:
    unsigned abi = 0;
    FileIdentity library;
    std::optional<FileIdentity> ballistics;
    unsigned ballisticsGameBuild = 0;
    std::vector<MapEntry> maps;
    explicit Catalog(const std::filesystem::path &root);
    const SceneEntry &scene(std::string_view map, std::string_view resource) const;
    std::filesystem::path verify(const FileIdentity &) const;
};
} // namespace mousebox::adapters::traceline
