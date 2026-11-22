#include "workspace_storage.hpp"
#include "application/settings/configuration_documents.hpp"
#include "application/storage/json_store.hpp"
#include <algorithm>
#include <stdexcept>

namespace mousebox::application::workspace_storage
{
namespace
{
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
} // namespace

bool validId(const std::string &id)
{
    return !id.empty() && id.size() <= 80 && std::all_of(id.begin(), id.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
    });
}
void save(const std::filesystem::path &path, const Json &value)
{
    require(storage::save(path, value), "工作区配置保存失败，原有配置保持");
}
std::filesystem::path settingsPath(const std::filesystem::path &root, const std::string &id)
{
    require(validId(id), "工作区标识无效");
    return root / L"workspaces" / storage::wide(id) / L"settings.json";
}
void recoverTransaction(const std::filesystem::path &root)
{
    const auto path = root / L"workspace-transaction.json";
    if (!std::filesystem::exists(path))
        return;
    const auto journal = storage::load(path, 3 * MaximumDocument);
    require(journal.value("schemaVersion", 0) == 1 &&
                (journal.at("state") == "prepared" || journal.at("state") == "committed"),
            "配置事务日志无效，原数据保留");
    if (journal.at("state") == "prepared")
    {
        const auto restore = [&](const std::filesystem::path &target, const Json &previous) {
            bool unchanged = false;
            try
            {
                unchanged = storage::load(target, MaximumDocument) == previous;
            }
            catch (const std::exception &)
            {
            }
            if (!unchanged)
                save(target, previous);
        };
        const auto id = journal.at("workspaceId").get<std::string>();
        const auto workspacePath = settingsPath(root, id);
        validateGameDocument(journal.at("previousWorkspace"));
        validateApplicationDocument(journal.at("previousGlobal"));
        restore(workspacePath, journal.at("previousWorkspace"));
        restore(root / L"application.json", journal.at("previousGlobal"));
    }
    std::error_code error;
    std::filesystem::remove(path, error);
    require(!error, "配置事务恢复后无法清理日志，请检查目录权限");
}
void commitConfiguration(const std::filesystem::path &root, const std::string &id, Json &application,
                         const Json &previousDocument, const Json &nextApplication, const Json &nextDocument)
{
    const auto journalPath = root / L"workspace-transaction.json";
    Json journal = {{"schemaVersion", 1},
                    {"state", "prepared"},
                    {"workspaceId", id},
                    {"previousGlobal", application},
                    {"previousWorkspace", previousDocument}};
    save(journalPath, journal);
    try
    {
        save(settingsPath(root, id), nextDocument);
        if (nextApplication != application)
            save(root / L"application.json", nextApplication);
        journal["state"] = "committed";
        save(journalPath, journal);
        application = nextApplication;
        std::error_code error;
        std::filesystem::remove(journalPath, error);
    }
    catch (...)
    {
        recoverTransaction(root);
        throw;
    }
}
} // namespace mousebox::application::workspace_storage
