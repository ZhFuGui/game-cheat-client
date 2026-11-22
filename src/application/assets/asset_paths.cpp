#include "asset_paths.hpp"
#include <stdexcept>

namespace mousebox::application::assets
{
std::filesystem::path directory(const std::filesystem::path &root, Kind kind)
{
    switch (kind)
    {
    case Kind::MouseCapture:
        return root / L"assets" / L"captures";
    case Kind::MouseModel:
        return root / L"assets" / L"mouse-models";
    case Kind::MouseReports:
        return root / L"assets" / L"mouse-reports";
    }
    throw std::invalid_argument("未知资产类型");
}
} // namespace mousebox::application::assets
