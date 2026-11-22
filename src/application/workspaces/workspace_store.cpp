#include "workspace_store.hpp"
#include "workspace_migration.hpp"
#include "workspace_storage.hpp"
#include "application/settings/configuration_documents.hpp"
#include "application/settings/game_configurations.hpp"
#include "application/settings/application_values.hpp"
#include "application/storage/json_store.hpp"
#include <algorithm>
#include <cwctype>
#include <set>
#include <stdexcept>

namespace mousebox::application
{
namespace
{
using workspace_storage::MaximumDocument;
using workspace_storage::validId;
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}

void validateStoredConfigurationName(const std::string &name)
{
    const auto wide = storage::wide(name);
    require(!wide.empty() && wide.size() <= 80 &&
                std::any_of(name.begin(), name.end(), [](unsigned char c) { return c > 32; }) &&
                std::none_of(name.begin(), name.end(), [](unsigned char c) { return c < 32; }),
            "工作区名称不能为空、含控制字符或超过80个字符");
}

} // namespace

WorkspaceStore::WorkspaceStore(std::filesystem::path root) : root_(std::move(root)) {}
const Json &WorkspaceStore::index() const noexcept
{
    return index_;
}
Json WorkspaceStore::applicationSettings() const
{
    return application_.at("values");
}
void WorkspaceStore::setApplicationSetting(const std::string &key, const Json &value)
{
    validateApplicationValue(key, value);
    auto next = application_;
    next["values"][key] = value;
    workspace_storage::save(root_ / L"application.json", next);
    application_ = std::move(next);
}
std::string WorkspaceStore::activeId() const
{
    return index_.at("activeWorkspaceId").get<std::string>();
}

std::filesystem::path WorkspaceStore::configurationPath(const std::string &id) const
{
    return workspace_storage::settingsPath(root_, id);
}
Json WorkspaceStore::summary(const std::string &id) const
{
    require(validId(id), "工作区标识无效");
    for (const auto &entry : index_.at("workspaces"))
        if (entry.at("id") == id)
            return entry;
    throw std::runtime_error("工作区不存在");
}
Json WorkspaceStore::document(const std::string &id) const
{
    const auto entry = summary(id);
    auto result = storage::load(configurationPath(id), MaximumDocument);
    validateGameDocument(result);
    require(result.at("gameId") == entry.at("gameId"), "工作区游戏归属不一致");
    return result;
}
Json WorkspaceStore::settings(const std::string &id) const
{
    auto doc = document(id);
    // Unsupported games never receive PUBG defaults or normalization.
    const auto &configuration = gameConfiguration(doc.at("gameId").get<std::string>());
    for (auto i = application_.at("values").begin(); i != application_.at("values").end(); ++i)
        doc["values"][i.key()] = i.value();
    return configuration.normalize(doc);
}
Json WorkspaceStore::motionBinding(const std::string &id) const
{
    const auto doc = document(id);
    return doc.value("mouseMotion", Json::object());
}
std::filesystem::path WorkspaceStore::resolveAssetPath(const std::filesystem::path &path) const
{
    const auto absolute = std::filesystem::absolute(path.is_relative() ? root_ / path : path).lexically_normal();
    std::error_code error;
    const auto canonical = std::filesystem::weakly_canonical(absolute, error);
    return error ? absolute : canonical;
}
std::vector<std::string> WorkspaceStore::modelReferences(const std::filesystem::path &model,
                                                         const std::string &excludingId) const
{
    const auto target = resolveAssetPath(model);
    const auto folded = [](const std::filesystem::path &path) {
        auto value = path.wstring();
        std::transform(value.begin(), value.end(), value.begin(),
                       [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
        return value;
    };
    std::vector<std::string> result;
    for (const auto &entry : index_.at("workspaces"))
    {
        const auto id = entry.at("id").get<std::string>();
        if (id == excludingId)
            continue;
        const auto reference = motionBinding(id).value("modelPath", std::string{});
        if (reference.empty())
            continue;
        const auto candidate = resolveAssetPath(storage::wide(reference));
        std::error_code error;
        // Existing file aliases, casing and relative paths are resolved by the
        // filesystem itself. Missing references still compare normalized names.
        const bool same = std::filesystem::equivalent(target, candidate, error);
        if ((!error && same) || (error && folded(target) == folded(candidate)))
            result.push_back(entry.at("name").get<std::string>());
    }
    return result;
}
void WorkspaceStore::commitIndex(Json next)
{
    workspace_storage::save(root_ / L"workspace-index.json", next);
    index_ = std::move(next);
}
void WorkspaceStore::open(const std::optional<Json> &imported)
{
    std::filesystem::create_directories(root_);
    workspace_storage::recoverTransaction(root_);
    if (!std::filesystem::exists(root_ / L"workspace-index.json"))
    {
        migrateLegacyConfiguration(imported);
        ensureGameConfigurations();
        return;
    }
    index_ = storage::load(root_ / L"workspace-index.json", MaximumDocument);
    const auto version = index_.value("schemaVersion", 0);
    require((version == 1 || version == 2) && index_.at("workspaces").is_array() && !index_.at("workspaces").empty() &&
                index_.at("workspaces").size() <= 256,
            "工作区索引结构或版本无效");
    std::set<std::string> ids;
    for (const auto &entry : index_.at("workspaces"))
    {
        const auto id = entry.at("id").get<std::string>();
        require(validId(id) && ids.insert(id).second, "工作区索引包含无效或重复标识");
        validateStoredConfigurationName(entry.at("name").get<std::string>());
        document(id);
    }
    summary(activeId());
    application_ = storage::load(root_ / L"application.json", MaximumDocument);
    validateApplicationDocument(application_);
    ensureGameConfigurations();
}

void WorkspaceStore::migrateLegacyConfiguration(const std::optional<Json> &imported)
{
    commitIndex(workspace_migration::prepareLegacyConfiguration(root_, imported, application_));
}
void WorkspaceStore::ensureGameConfigurations()
{
    auto next = workspace_migration::prepareGameConfigurationIndex(root_, index_);
    if (next != index_)
        commitIndex(std::move(next));
}
void WorkspaceStore::save(const std::string &id, const Json &settings, const Json &binding)
{
    const auto entry = summary(id);
    const auto gameId = entry.at("gameId").get<std::string>();
    const auto normalized = gameConfiguration(gameId).normalize(settings);
    const auto nextApplication = splitApplicationDocument(normalized);
    // This API keeps unknown extension metadata in the existing document.
    auto next = document(id);
    const auto known = splitGameDocument(normalized, gameId, binding);
    for (auto i = known.begin(); i != known.end(); ++i)
        next[i.key()] = i.value();
    const auto previous = document(id);
    if (next == previous && nextApplication == application_)
        return;
    workspace_storage::commitConfiguration(root_, id, application_, previous, nextApplication, next);
}

void WorkspaceStore::activate(const std::string &id)
{
    const auto entry = summary(id);
    require(isAvailableGameConfiguration(entry.at("gameId").get<std::string>()), "此游戏尚未接入");
    settings(id); // Validate persisted data before publishing the new active index.
    auto next = index_;
    next["activeWorkspaceId"] = id;
    commitIndex(std::move(next));
}
} // namespace mousebox::application
