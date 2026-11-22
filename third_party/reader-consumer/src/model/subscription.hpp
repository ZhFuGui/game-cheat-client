#pragma once
#include "model/primitives.hpp"

namespace monitor {
// 模式只控制诊断工作；两种模式都必须执行发布业务数据所需的来源核验。
enum class SubscriptionProfile : unsigned { Consumer = 1, Debug = 2 };
using SubscriptionFeatures = u64;
// 稳定业务能力位；0 表示所订类别当前全部已实现业务能力，不表示不采集。
enum class SubscriptionFeature : unsigned {
    LivingState = 0, LivingName, LivingWorldVelocity, LivingSkeleton,
    VehicleState, VehicleSeats, VehicleModel, GroundLoot,
    LocalControl, LocalCamera, LocalWeapon, LocalMuzzle, LocalRecoil,
    LocalHolding, LocalSway, LocalAimBasis, LocalSpectators, LocalControllerRotation,
    Count
};
inline constexpr SubscriptionFeatures FeatureBit(SubscriptionFeature feature) noexcept {
    return unsigned(feature) < unsigned(SubscriptionFeature::Count) ? u64(1) << unsigned(feature) : 0;
}
inline constexpr SubscriptionFeatures AllSubscriptionFeatures =
    (u64(1) << unsigned(SubscriptionFeature::Count)) - 1;
inline constexpr bool ValidSubscriptionFeatures(SubscriptionFeatures features) noexcept {
    return !(features & ~AllSubscriptionFeatures);
}
inline constexpr bool ValidSubscriptionProfile(SubscriptionProfile profile) noexcept {
    return profile == SubscriptionProfile::Consumer || profile == SubscriptionProfile::Debug;
}
inline constexpr bool DetailedSubscription(SubscriptionProfile profile) noexcept {
    return profile == SubscriptionProfile::Debug;
}
inline constexpr const char* SubscriptionProfileName(SubscriptionProfile profile) noexcept {
    return profile == SubscriptionProfile::Debug ? "Debug" : "Consumer";
}
} // namespace monitor
