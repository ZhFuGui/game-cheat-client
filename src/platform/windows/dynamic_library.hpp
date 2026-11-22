#pragma once
#include <filesystem>
#include <string>

namespace mousebox::platform
{
std::filesystem::path executableDirectory();
std::string fileSha256(const std::filesystem::path &);

// Explicit absolute file; dependency search never includes the working directory.
class DynamicLibrary final
{
    void *handle_ = nullptr;

  public:
    explicit DynamicLibrary(const std::filesystem::path &);
    ~DynamicLibrary();
    DynamicLibrary(const DynamicLibrary &) = delete;
    DynamicLibrary &operator=(const DynamicLibrary &) = delete;
    void *symbol(const char *name) const;
};
} // namespace mousebox::platform
