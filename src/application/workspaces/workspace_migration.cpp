#include "workspace_migration.hpp"
#include "workspace_storage.hpp"
#include "application/settings/configuration_documents.hpp"
#include "application/settings/game_configurations.hpp"
#include "application/storage/json_store.hpp"
#include <algorithm>
#include <map>
#include <stdexcept>

namespace mousebox::application::workspace_migration
{
namespace
{
using workspace_storage::MaximumDocument;
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
} // namespace

Json prepareLegacyConfiguration(const std::filesystem::path &root, const std::optional<Json> &imported,
                                Json &application)
{
    const auto legacy = root / L"settings.json";
    Json source = Json::object();
    std::string bytes = "{}";
    if (std::filesystem::exists(legacy))
    {
        bytes = storage::readBytes(legacy, MaximumDocument);
        source = Json::parse(bytes);
    }
    else if (imported)
    {
        source = *imported;
        bytes = source.dump();
    }
    require(source.is_object(), "旧配置不是对象，迁移未开始");
    const auto digest = storage::sha256(bytes);
    const auto backup = root / L"migration-backup" / storage::wide("multigame-v1-" + digest.substr(0, 16));
    std::filesystem::create_directories(backup);
    if (std::filesystem::exists(legacy))
    {
        const auto original = backup / L"settings.json";
        if (!std::filesystem::exists(original))
            std::filesystem::copy_file(legacy, original);
        require(storage::sha256(storage::readBytes(original, MaximumDocument)) == digest,
                "旧配置备份校验失败，迁移未提交");
    }
    else
        workspace_storage::save(backup / L"imported-settings.json", source);
    const auto normalized = gameConfiguration("pubg").normalize(source);
    application = splitApplicationDocument(normalized);
    const std::string id = "pubg-default";
    const auto workspace = splitGameDocument(normalized, "pubg", source.value("mouseMotion", Json::object()));
    validateGameDocument(workspace);
    // Every provisional document is complete before the index is published.
    // Interrupted migrations can repeat this step from the unchanged backup.
    workspace_storage::save(
        backup / L"migration.json",
        {{"schemaVersion", 1}, {"sourceSha256", digest}, {"workspaceId", id}, {"assets", "preserved-in-place"}});
    workspace_storage::save(workspace_storage::settingsPath(root, id), workspace);
    workspace_storage::save(root / L"application.json", application);
    return {{"schemaVersion", 1},
            {"activeWorkspaceId", id},
            {"workspaces", Json::array({{{"id", id}, {"gameId", "pubg"}, {"name", "默认配置"}}})}};
}
Json prepareGameConfigurationIndex(const std::filesystem::path &root, const Json &index)
{
    const bool legacy = index.at("schemaVersion") == 1;
    const auto active = index.at("activeWorkspaceId").get<std::string>();
    std::map<std::string, Json> chosen;
    for (const auto &entry : index.at("workspaces"))
    {
        const auto gameId = entry.at("gameId").get<std::string>();
        require(legacy || !chosen.contains(gameId), "同一游戏存在重复配置，原文件保留");
        if (!chosen.contains(gameId) || entry.at("id") == active)
            chosen[gameId] = entry;
    }
    auto next = index;
    next["schemaVersion"] = 2;
    next["policy"] = "one-configuration-per-game";
    next["workspaces"] = Json::array();
    if (legacy)
    {
        const auto indexPath = root / L"workspace-index.json";
        const auto bytes = storage::readBytes(indexPath, MaximumDocument);
        const auto backup =
            root / L"migration-backup" / storage::wide("single-game-v2-" + storage::sha256(bytes).substr(0, 16));
        const auto copy = [&](const std::filesystem::path &source, const std::filesystem::path &relative) {
            const auto target = backup / relative;
            std::filesystem::create_directories(target.parent_path());
            if (!std::filesystem::exists(target))
                std::filesystem::copy_file(source, target);
            require(storage::sha256(storage::readBytes(source, MaximumDocument)) ==
                        storage::sha256(storage::readBytes(target, MaximumDocument)),
                    "游戏配置迁移备份校验失败");
        };
        copy(indexPath, L"workspace-index.json");
        for (const auto &entry : index.at("workspaces"))
        {
            const auto id = entry.at("id").get<std::string>();
            copy(workspace_storage::settingsPath(root, id),
                 std::filesystem::path(L"workspaces") / storage::wide(id) / L"settings.json");
        }
    }
    for (const auto &game : availableGameConfigurations())
    {
        const std::string gameId(game.id);
        if (auto entry = chosen.find(gameId); entry != chosen.end())
        {
            auto summary = entry->second;
            summary["name"] = game.name;
            next["workspaces"].push_back(std::move(summary));
        }
        else
        {
            const auto id = gameId + "-default";
            require(std::none_of(index.at("workspaces").begin(), index.at("workspaces").end(),
                                 [&](const Json &entry) { return entry.at("id") == id; }),
                    "游戏配置标识冲突");
            const auto path = workspace_storage::settingsPath(root, id);
            if (std::filesystem::exists(path))
            {
                // An interrupted index commit may leave a complete document.
                // Reuse it after validation; never overwrite it with defaults.
                const auto existing = storage::load(path, MaximumDocument);
                validateGameDocument(existing);
                require(existing.at("gameId") == gameId, "游戏配置文件归属不匹配");
            }
            else
                workspace_storage::save(path, splitGameDocument(gameConfiguration(gameId).defaults(), gameId));
            next["workspaces"].push_back({{"id", id}, {"gameId", gameId}, {"name", game.name}});
        }
    }
    require(!next.at("workspaces").empty(), "没有已接入的游戏");
    require(std::any_of(next.at("workspaces").begin(), next.at("workspaces").end(),
                        [&](const Json &entry) { return entry.at("id") == active; }),
            "当前游戏不可用，原配置保留");
    return next;
}
} // namespace mousebox::application::workspace_migration
