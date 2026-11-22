#pragma once
#include <Windows.h>
#include <stdexcept>
#include <string>
namespace mousebox::render::detail
{
inline void check(HRESULT result, const char *operation)
{
    if (FAILED(result))
        throw std::runtime_error(std::string(operation) + " (HRESULT " +
                                 std::to_string(static_cast<unsigned long>(result)) + ")");
}
} // namespace mousebox::render::detail
