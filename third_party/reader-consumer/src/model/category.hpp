#pragma once
#include "model/primitives.hpp"
#include "model/subscription.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <limits>

namespace monitor {
// 业务采集类别；GroundLoot 同时包含独立拾取 Actor 和地面组的组件成员。
enum class DataCategory : unsigned {
    None = 0,
    Living = 1,
    Vehicles = 2,
    GroundLoot = 3,
    Containers = 4,
    Projectiles = 5,
    Local = 6
};
inline constexpr std::size_t CategoryCount = 7;
inline constexpr std::size_t CategoryStreamCount = CategoryCount * 2;
inline constexpr std::size_t CategoryStreamIndex(DataCategory category, SubscriptionProfile profile) noexcept {
    return static_cast<std::size_t>(category) + (profile == SubscriptionProfile::Debug ? CategoryCount : 0);
}
inline constexpr double MaximumCategoryHz = 144.0;
inline constexpr std::size_t CategoryIndex(DataCategory category) noexcept {
    return static_cast<std::size_t>(category);
}
inline constexpr bool ImplementedCategory(DataCategory category) noexcept {
    return category == DataCategory::Living || category == DataCategory::Vehicles ||
           category == DataCategory::GroundLoot || category == DataCategory::Local;
}
inline constexpr double DefaultCategoryHz(DataCategory category) noexcept {
    return category == DataCategory::GroundLoot
               ? 2.0
               : (category == DataCategory::Living || category == DataCategory::Vehicles ||
                          category == DataCategory::Local
                      ? 30.0
                      : 0.0);
}
// 客户端视点只依赖本地控制器链；调高相机频率不能提高 Actor 名单及分类需求。
inline constexpr bool RequiresActorDirectory(DataCategory category) noexcept {
    return category != DataCategory::Local;
}
inline const char* CategoryName(DataCategory category) noexcept {
    switch (category) {
        case DataCategory::Living:
            return "Living";
        case DataCategory::Vehicles:
            return "Vehicles";
        case DataCategory::GroundLoot:
            return "GroundLoot";
        case DataCategory::Containers:
            return "Containers";
        case DataCategory::Projectiles:
            return "Projectiles";
        case DataCategory::Local:
            return "Local";
        default:
            return "None";
    }
}
inline std::uint64_t PeriodForHz(double hz) noexcept {
    if (!std::isfinite(hz) || hz <= 0)
        return 0;
    const long double period = std::ceil(1000000000.0L / static_cast<long double>(hz));
    // 留出时间线加法的安全余量；无法表达的周期明确拒绝。
    if (period < 1 || period > static_cast<long double>((std::numeric_limits<std::int64_t>::max)() / 4))
        return 0;
    return static_cast<std::uint64_t>(period);
}
inline bool ValidCategoryHz(double hz) noexcept {
    return std::isfinite(hz) && hz > 0 && hz <= MaximumCategoryHz && PeriodForHz(hz) != 0;
}
struct CategoryDemandSnapshot {
    u64 revision = 0;
    std::array<double, CategoryCount> target_hz{};
    std::array<double, CategoryCount> consumer_hz{}, debug_hz{};
    std::array<SubscriptionFeatures, CategoryCount> consumer_features{}, debug_features{};
    double Hz(DataCategory category, SubscriptionProfile profile) const noexcept {
        const auto index = CategoryIndex(category);
        return profile == SubscriptionProfile::Debug ? debug_hz[index] : consumer_hz[index];
    }
    SubscriptionFeatures Features(DataCategory category, SubscriptionProfile profile) const noexcept {
        const auto index = CategoryIndex(category);
        return profile == SubscriptionProfile::Debug ? debug_features[index] : consumer_features[index];
    }
    double Highest() const noexcept {
        double highest = 0;
        for (std::size_t i = CategoryIndex(DataCategory::Living); i <= CategoryIndex(DataCategory::Projectiles); ++i)
            highest = std::max(highest, target_hz[i]);
        return highest;
    }
    double ClassificationHz() const noexcept { return Highest() + 1.0; }
    double DirectoryHz() const noexcept { return Highest() + 2.0; }
};
} // namespace monitor
