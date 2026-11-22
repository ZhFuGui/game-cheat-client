#pragma once
#include "adapters/sources/pubg_reader/session.hpp"
#include <filesystem>
namespace mousebox::reader
{
class Evidence
{
  public:
    Evidence(const std::string &root, const char *category, unsigned connection);
    void received(std::string_view body, unsigned payload);
    void sentAck(std::string_view packet);
    void snapshot(const Session &);

  private:
    void write(const char *name, std::string_view value);
    std::filesystem::path folder_;
    std::string latestUpdate_;
};
} // namespace mousebox::reader
