#include "dynamic_library.hpp"
#include <Windows.h>
#include <bcrypt.h>
#include <array>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace mousebox::platform
{
std::filesystem::path executableDirectory()
{
    std::wstring path(512, L'\0');
    for (;;)
    {
        const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (!length)
            throw std::runtime_error("Cannot locate executable directory");
        if (length < path.size() - 1)
        {
            path.resize(length);
            return std::filesystem::path(path).parent_path();
        }
        if (path.size() >= 32768)
            throw std::runtime_error("Executable path is too long");
        path.resize(path.size() * 2);
    }
}
std::string fileSha256(const std::filesystem::path &path)
{
    struct Digest
    {
        BCRYPT_ALG_HANDLE algorithm = nullptr;
        BCRYPT_HASH_HANDLE hash = nullptr;
        std::vector<unsigned char> object;
        ~Digest()
        {
            if (hash)
                BCryptDestroyHash(hash);
            if (algorithm)
                BCryptCloseAlgorithmProvider(algorithm, 0);
        }
    } digest;
    const auto check = [](NTSTATUS status) {
        if (status < 0)
            throw std::runtime_error("File SHA256 failed");
    };
    check(BCryptOpenAlgorithmProvider(&digest.algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0));
    DWORD bytes = 0, objectSize = 0;
    check(BCryptGetProperty(digest.algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize),
                            sizeof(objectSize), &bytes, 0));
    digest.object.resize(objectSize);
    check(BCryptCreateHash(digest.algorithm, &digest.hash, digest.object.data(), objectSize, nullptr, 0, 0));
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("Cannot open file for SHA256");
    std::array<unsigned char, 64 * 1024> buffer{};
    while (stream)
    {
        stream.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
        const auto count = stream.gcount();
        if (count)
            check(BCryptHashData(digest.hash, buffer.data(), static_cast<ULONG>(count), 0));
    }
    if (!stream.eof())
        throw std::runtime_error("Cannot complete file SHA256");
    std::array<unsigned char, 32> result{};
    check(BCryptFinishHash(digest.hash, result.data(), static_cast<ULONG>(result.size()), 0));
    constexpr char hex[] = "0123456789abcdef";
    std::string output;
    output.reserve(64);
    for (const auto byte : result)
    {
        output.push_back(hex[byte >> 4]);
        output.push_back(hex[byte & 15]);
    }
    return output;
}
DynamicLibrary::DynamicLibrary(const std::filesystem::path &file)
{
    if (!file.is_absolute())
        throw std::invalid_argument("Library path must be absolute");
    handle_ = LoadLibraryExW(file.c_str(), nullptr,
                             LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                                 LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!handle_)
        throw std::runtime_error("无法加载掩体检测运行库或其运行依赖");
}
DynamicLibrary::~DynamicLibrary()
{
    if (handle_)
        FreeLibrary(static_cast<HMODULE>(handle_));
}
void *DynamicLibrary::symbol(const char *name) const
{
    const auto address = GetProcAddress(static_cast<HMODULE>(handle_), name);
    if (!address)
        throw std::runtime_error("掩体检测运行库接口不完整");
    return reinterpret_cast<void *>(address);
}
} // namespace mousebox::platform
