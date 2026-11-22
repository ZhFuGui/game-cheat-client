#pragma once
#define NAPI_VERSION 8
#include <node_api.h>
#include <Windows.h>
#include <bit>
#include <stdexcept>
#include <string>

namespace mousebox::desktop::addon
{
// Stable Node-API entry points are exported by both node.exe and Electron's
// main executable. Resolve them in-process, avoiding a dependency on an
// executable filename or Electron's version-specific node.lib import library.
template <typename T> T symbol(const char *name)
{
    const auto address = GetProcAddress(GetModuleHandleW(nullptr), name);
    if (!address)
        throw std::runtime_error(std::string("Node-API export missing: ") + name);
    return std::bit_cast<T>(address);
}
struct Api
{
    decltype(&napi_get_cb_info) arguments = symbol<decltype(arguments)>("napi_get_cb_info");
    decltype(&napi_get_value_string_utf8) readString = symbol<decltype(readString)>("napi_get_value_string_utf8");
    decltype(&napi_create_string_utf8) string = symbol<decltype(string)>("napi_create_string_utf8");
    decltype(&napi_throw_error) error = symbol<decltype(error)>("napi_throw_error");
    decltype(&napi_create_function) function = symbol<decltype(function)>("napi_create_function");
    decltype(&napi_set_named_property) property = symbol<decltype(property)>("napi_set_named_property");
    decltype(&napi_set_instance_data) setData = symbol<decltype(setData)>("napi_set_instance_data");
    decltype(&napi_get_instance_data) data = symbol<decltype(data)>("napi_get_instance_data");
};
inline const Api &api()
{
    static const Api value;
    return value;
}
inline void check(napi_status status)
{
    if (status != napi_ok)
        throw std::runtime_error("Node-API call failed");
}
inline std::string readString(napi_env env, napi_value value)
{
    std::size_t size = 0;
    check(api().readString(env, value, nullptr, 0, &size));
    if (size > 32768)
        throw std::runtime_error("Native input argument is too long");
    std::string text(size + 1, '\0');
    check(api().readString(env, value, text.data(), text.size(), &size));
    text.resize(size);
    return text;
}
} // namespace mousebox::desktop::addon
