#pragma once
#include <stdexcept>
#include <string_view>
namespace mousebox::control::movement
{
// Historical is a saved selection, resolved by the game's preset adapter.
enum class Algorithm
{
    Historical,
    Smooth,
    RecoilTracking,
    Direct
};
constexpr std::string_view algorithmId(Algorithm value)
{
    switch (value)
    {
    case Algorithm::Historical:
        return "historical";
    case Algorithm::Smooth:
        return "smooth";
    case Algorithm::RecoilTracking:
        return "recoil-tracking";
    case Algorithm::Direct:
        return "direct";
    }
    throw std::invalid_argument("未知鼠标移动算法");
}
inline Algorithm parseAlgorithm(std::string_view id)
{
    for (const auto value : {Algorithm::Historical, Algorithm::Smooth, Algorithm::RecoilTracking, Algorithm::Direct})
        if (algorithmId(value) == id)
            return value;
    throw std::invalid_argument("未知鼠标移动算法");
}
} // namespace mousebox::control::movement
