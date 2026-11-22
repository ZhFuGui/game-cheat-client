#pragma once
#include "application/json.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace mousebox::application
{
// Each available game has one configuration. The public workspace vocabulary is
// retained for persisted IDs, paths and session contracts. Index publication,
// application values and game documents still have one native writer.
class WorkspaceStore
{
  public:
    explicit WorkspaceStore(std::filesystem::path root);
    void open(const std::optional<Json> &imported = {});
    const Json &index() const noexcept;
    Json applicationSettings() const;
    void setApplicationSetting(const std::string &key, const Json &value);
    std::string activeId() const;
    Json settings(const std::string &id) const;
    Json motionBinding(const std::string &id) const;
    std::filesystem::path resolveAssetPath(const std::filesystem::path &) const;
    std::vector<std::string> modelReferences(const std::filesystem::path &, const std::string &excludingId) const;
    void save(const std::string &id, const Json &settings, const Json &binding);
    void activate(const std::string &id);
    Json summary(const std::string &id) const;

  private:
    std::filesystem::path root_;
    Json index_, application_;
    std::filesystem::path configurationPath(const std::string &id) const;
    Json document(const std::string &id) const;
    void commitIndex(Json next);
    void migrateLegacyConfiguration(const std::optional<Json> &imported);
    void ensureGameConfigurations();
};
} // namespace mousebox::application
