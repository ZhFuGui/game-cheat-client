#pragma once
#include "application/json.hpp"
#include <filesystem>
#include <string>
namespace mousebox::application::storage {
std::string readBytes(const std::filesystem::path&,std::uintmax_t maximum);
Json load(const std::filesystem::path&,std::uintmax_t maximum);
bool save(const std::filesystem::path&,const Json&);
std::string sha256(std::string_view);
std::string utf8(const std::wstring&);
std::wstring wide(const std::string&);
}
