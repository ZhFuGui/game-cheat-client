#pragma once
#include "model/view.hpp"
#include "model/entities/muzzle.hpp"

namespace monitor {
// 原生剩余目标量；采样区间涵盖两个实际字段块，不能当作原子三轴旋转或已应用角。
struct WeaponRecoilTargetsSample {
    OwnedObservation observation;
    std::optional<float> target_pitch, target_yaw, recovery_target;
};
// 原生轨迹建立时的 uint32 计数，不是扳机状态、射速、命中或服务器确认。
struct WeaponTrajectoryGenerationSample {
    OwnedObservation observation;
    std::optional<u32> counter;
};
// 只读取当前槽位；不将人物的备用装备表带入本地客户端状态。
struct LocalWeaponSample {
    OwnedObservation observation;
    std::optional<u64> object_address;
    std::optional<ObjectName> class_name;
    std::optional<InstanceIdentity> instance;
    std::shared_ptr<const WeaponMuzzleSample> muzzle;
    std::shared_ptr<const WeaponRecoilTargetsSample> recoil_targets;
    std::shared_ptr<const WeaponTrajectoryGenerationSample> trajectory_generation;
};
struct CurrentWeaponSample {
    OwnedObservation observation;
    SampleInterval verified;
    std::optional<InstanceIdentity> character, processor;
    std::optional<i32> active_slot;
    std::optional<u32> declared_slots;
    std::optional<u64> raw_weapon_value;
    std::optional<bool> has_current_item; // 原生当前槽有有效对象；false仅确认无当前对象，失败保持缺失。
    std::optional<LocalWeaponSample> weapon;
};
struct LocalSpectatorsSample {
    OwnedObservation observation;
    SampleInterval verified;
    std::optional<InstanceIdentity> character;
    std::optional<u32> count;
};
// 原生控制器旋转；未证明它等于无散布射击方向，也不把它解释成后坐角或角增量。
struct ControllerRotationSample {
    OwnedObservation observation;
    SampleInterval controller_sample, rotation_sample, verified;
    std::optional<InstanceIdentity> controller;
    std::optional<ViewRotation> rotation;
};
// 原生接受的屏息状态及过渡值；属于本地身体的动作组件，不代表按键意图或纯呼吸角。
struct NativeHoldingSample {
    OwnedObservation observation;
    SampleInterval verified, holding_sample, blend_sample;
    std::optional<InstanceIdentity> character, component;
    std::optional<bool> accepted_holding;
    std::optional<float> holding_blend;
};
// 按本批输入计算的原生 sway 三轴量；不解释为纯呼吸、已施加后坐或最终射击方向。
struct NativeSwaySample {
    OwnedObservation observation;
    SampleInterval verified;
    std::optional<InstanceIdentity> character, weapon;
    std::optional<ViewRotation> rotation;
};
enum class AimBasisBranch : unsigned { Barrel = 1, ViewNoTrace = 2 };
enum class AimBasisStage : unsigned { PostBranchPreSpread = 1 };
// 已核定原生分支的散布前基方向；分支决定是否使用枪口和归零，不以相机替代未完成的物理查询。
struct LocalAimBasisSample {
    OwnedObservation observation;
    SampleInterval verified;
    std::optional<InstanceIdentity> character, weapon, mesh, gun_data, trajectory_config;
    std::optional<Vec3> direction;
    std::optional<float> zeroing_distance_m, zeroing_correction_degrees;
    std::optional<u32> integrated_steps;
    std::optional<bool> zeroing_reached_distance;
    std::optional<float> integrated_distance_cm;
    std::optional<AimBasisBranch> branch;
    std::optional<AimBasisStage> stage;
    std::optional<bool> native_zeroing_applied;
    std::optional<std::string> native_profile;
};
// 同一订阅批次提供本地专属数据；各组仍保留自己的真实读取与关闭时间。
struct LocalSample {
    FactStatus control_status = FactStatus::NotAttempted;
    ViewSample camera;
    std::shared_ptr<const CurrentWeaponSample> current_weapon;
    LocalSpectatorsSample spectators;
    std::shared_ptr<const ControllerRotationSample> controller_rotation;
    std::shared_ptr<const NativeHoldingSample> native_holding;
    std::shared_ptr<const NativeSwaySample> native_sway;
    std::shared_ptr<const LocalAimBasisSample> aim_basis;
};
} // namespace monitor
