#pragma once
// 由 protocol/generate_consumer_model.py 生成，与纯业务proto字段同源。
namespace monitor::synchronization::codec::detail {
template<> auto ConsumerMembers<consumer::Vector>() {
    return std::tuple{ConsumerField{1u, &consumer::Vector::x}, ConsumerField{2u, &consumer::Vector::y}, ConsumerField{3u, &consumer::Vector::z}};
}
template<> auto ConsumerMembers<consumer::Origin>() {
    return std::tuple{ConsumerField{1u, &consumer::Origin::x}, ConsumerField{2u, &consumer::Origin::y}, ConsumerField{3u, &consumer::Origin::z}};
}
template<> auto ConsumerMembers<consumer::Rotation>() {
    return std::tuple{ConsumerField{1u, &consumer::Rotation::pitch}, ConsumerField{2u, &consumer::Rotation::yaw}, ConsumerField{3u, &consumer::Rotation::roll}};
}
template<> auto ConsumerMembers<consumer::Quaternion>() {
    return std::tuple{ConsumerField{1u, &consumer::Quaternion::x}, ConsumerField{2u, &consumer::Quaternion::y}, ConsumerField{3u, &consumer::Quaternion::z}, ConsumerField{4u, &consumer::Quaternion::w}};
}
template<> auto ConsumerMembers<consumer::Transform>() {
    return std::tuple{ConsumerField{1u, &consumer::Transform::rotation}, ConsumerField{2u, &consumer::Transform::translation}, ConsumerField{3u, &consumer::Transform::scale}};
}
template<> auto ConsumerMembers<consumer::Reference>() {
    return std::tuple{ConsumerField{1u, &consumer::Reference::id}, ConsumerField{2u, &consumer::Reference::kind}};
}
template<> auto ConsumerMembers<consumer::Spatial>() {
    return std::tuple{ConsumerField{1u, &consumer::Spatial::position}, ConsumerField{2u, &consumer::Spatial::origin}};
}
template<> auto ConsumerMembers<consumer::LivingState>() {
    return std::tuple{ConsumerField{1u, &consumer::LivingState::team}, ConsumerField{2u, &consumer::LivingState::is_bot}, ConsumerField{6u, &consumer::LivingState::character_type}};
}
template<> auto ConsumerMembers<consumer::Name>() {
    return std::tuple{ConsumerField{1u, &consumer::Name::value}};
}
template<> auto ConsumerMembers<consumer::WorldVelocity>() {
    return std::tuple{ConsumerField{1u, &consumer::WorldVelocity::velocity}, ConsumerField{2u, &consumer::WorldVelocity::source}, ConsumerField{3u, &consumer::WorldVelocity::owner}};
}
template<> auto ConsumerMembers<consumer::Movement>() {
    return std::tuple{ConsumerField{1u, &consumer::Movement::component_velocity}, ConsumerField{4u, &consumer::Movement::world}};
}
template<> auto ConsumerMembers<consumer::Bone>() {
    return std::tuple{ConsumerField{1u, &consumer::Bone::role}, ConsumerField{5u, &consumer::Bone::world_position}, ConsumerField{2u, &consumer::Bone::parent_role}};
}
template<> auto ConsumerMembers<consumer::Skeleton>() {
    return std::tuple{ConsumerField{1u, &consumer::Skeleton::origin}, ConsumerField{2u, &consumer::Skeleton::bones}, ConsumerField{3u, &consumer::Skeleton::owner}, ConsumerField{4u, &consumer::Skeleton::layout_token}, ConsumerField{5u, &consumer::Skeleton::pose_token}};
}
template<> auto ConsumerMembers<consumer::VehicleState>() {
    return std::tuple{ConsumerField{1u, &consumer::VehicleState::health}, ConsumerField{2u, &consumer::VehicleState::max_health}, ConsumerField{3u, &consumer::VehicleState::fuel}, ConsumerField{4u, &consumer::VehicleState::max_fuel}, ConsumerField{5u, &consumer::VehicleState::engine_running}, ConsumerField{6u, &consumer::VehicleState::destroyed}, ConsumerField{7u, &consumer::VehicleState::is_dying}, ConsumerField{8u, &consumer::VehicleState::is_broken}};
}
template<> auto ConsumerMembers<consumer::Seat>() {
    return std::tuple{ConsumerField{1u, &consumer::Seat::index}, ConsumerField{2u, &consumer::Seat::occupancy}, ConsumerField{3u, &consumer::Seat::coverage}, ConsumerField{4u, &consumer::Seat::riders}};
}
template<> auto ConsumerMembers<consumer::Seats>() {
    return std::tuple{ConsumerField{1u, &consumer::Seats::coverage}, ConsumerField{2u, &consumer::Seats::declared_seats}, ConsumerField{3u, &consumer::Seats::occupied_seats}, ConsumerField{4u, &consumer::Seats::seats}};
}
template<> auto ConsumerMembers<consumer::VehicleModel>() {
    return std::tuple{ConsumerField{1u, &consumer::VehicleModel::model_key}, ConsumerField{2u, &consumer::VehicleModel::class_name}};
}
template<> auto ConsumerMembers<consumer::VehicleType>() {
    return std::tuple{ConsumerField{1u, &consumer::VehicleType::kind}};
}
template<> auto ConsumerMembers<consumer::CreatureType>() {
    return std::tuple{ConsumerField{1u, &consumer::CreatureType::character_type}, ConsumerField{2u, &consumer::CreatureType::species}};
}
template<> auto ConsumerMembers<consumer::ContainerType>() {
    return std::tuple{ConsumerField{1u, &consumer::ContainerType::kind}};
}
template<> auto ConsumerMembers<consumer::ItemDefinition>() {
    return std::tuple{ConsumerField{1u, &consumer::ItemDefinition::item_id}, ConsumerField{2u, &consumer::ItemDefinition::identifier}, ConsumerField{3u, &consumer::ItemDefinition::display_name}};
}
template<> auto ConsumerMembers<consumer::ItemClassification>() {
    return std::tuple{ConsumerField{1u, &consumer::ItemClassification::category}, ConsumerField{2u, &consumer::ItemClassification::status}};
}
template<> auto ConsumerMembers<consumer::Stack>() {
    return std::tuple{ConsumerField{1u, &consumer::Stack::quantity}, ConsumerField{2u, &consumer::Stack::durability}, ConsumerField{3u, &consumer::Stack::max_durability}};
}
template<> auto ConsumerMembers<consumer::GroundMember>() {
    return std::tuple{ConsumerField{1u, &consumer::GroundMember::id}, ConsumerField{2u, &consumer::GroundMember::spatial}, ConsumerField{3u, &consumer::GroundMember::definition}, ConsumerField{4u, &consumer::GroundMember::stack}, ConsumerField{5u, &consumer::GroundMember::classification}};
}
template<> auto ConsumerMembers<consumer::GroundContents>() {
    return std::tuple{ConsumerField{1u, &consumer::GroundContents::coverage}, ConsumerField{2u, &consumer::GroundContents::members}};
}
template<> auto ConsumerMembers<consumer::LocalControl>() {
    return std::tuple{ConsumerField{1u, &consumer::LocalControl::controller_token}, ConsumerField{2u, &consumer::LocalControl::character}, ConsumerField{3u, &consumer::LocalControl::controlled}};
}
template<> auto ConsumerMembers<consumer::Camera>() {
    return std::tuple{ConsumerField{1u, &consumer::Camera::position}, ConsumerField{2u, &consumer::Camera::rotation}, ConsumerField{3u, &consumer::Camera::fov_degrees}, ConsumerField{4u, &consumer::Camera::origin}, ConsumerField{5u, &consumer::Camera::cached_fov_degrees}, ConsumerField{6u, &consumer::Camera::locked_fov_degrees}, ConsumerField{7u, &consumer::Camera::cache_time_seconds}, ConsumerField{8u, &consumer::Camera::controller_token}, ConsumerField{9u, &consumer::Camera::source_token}};
}
template<> auto ConsumerMembers<consumer::CameraTarget>() {
    return std::tuple{ConsumerField{1u, &consumer::CameraTarget::has_target}, ConsumerField{2u, &consumer::CameraTarget::pending_active}, ConsumerField{3u, &consumer::CameraTarget::target}};
}
template<> auto ConsumerMembers<consumer::Muzzle>() {
    return std::tuple{ConsumerField{1u, &consumer::Muzzle::origin}, ConsumerField{2u, &consumer::Muzzle::attach_name}, ConsumerField{3u, &consumer::Muzzle::bone_name}, ConsumerField{6u, &consumer::Muzzle::binding}, ConsumerField{10u, &consumer::Muzzle::world_transform}};
}
template<> auto ConsumerMembers<consumer::Recoil>() {
    return std::tuple{ConsumerField{1u, &consumer::Recoil::target_pitch}, ConsumerField{2u, &consumer::Recoil::target_yaw}, ConsumerField{3u, &consumer::Recoil::recovery_target}};
}
template<> auto ConsumerMembers<consumer::Weapon>() {
    return std::tuple{ConsumerField{1u, &consumer::Weapon::weapon_token}, ConsumerField{2u, &consumer::Weapon::name}, ConsumerField{3u, &consumer::Weapon::active_slot}, ConsumerField{4u, &consumer::Weapon::muzzle}, ConsumerField{5u, &consumer::Weapon::recoil}, ConsumerField{6u, &consumer::Weapon::trajectory_generation}, ConsumerField{7u, &consumer::Weapon::has_current_item}, ConsumerField{8u, &consumer::Weapon::owner}};
}
template<> auto ConsumerMembers<consumer::Spectators>() {
    return std::tuple{ConsumerField{1u, &consumer::Spectators::count}, ConsumerField{2u, &consumer::Spectators::owner}};
}
template<> auto ConsumerMembers<consumer::ControllerRotation>() {
    return std::tuple{ConsumerField{1u, &consumer::ControllerRotation::controller_token}, ConsumerField{2u, &consumer::ControllerRotation::rotation}};
}
template<> auto ConsumerMembers<consumer::Holding>() {
    return std::tuple{ConsumerField{3u, &consumer::Holding::accepted_holding}, ConsumerField{4u, &consumer::Holding::blend}};
}
template<> auto ConsumerMembers<consumer::Sway>() {
    return std::tuple{ConsumerField{3u, &consumer::Sway::rotation}};
}
template<> auto ConsumerMembers<consumer::AimBasis>() {
    return std::tuple{ConsumerField{1u, &consumer::AimBasis::owner}, ConsumerField{2u, &consumer::AimBasis::weapon_token}, ConsumerField{3u, &consumer::AimBasis::native_context_token}, ConsumerField{6u, &consumer::AimBasis::direction}, ConsumerField{7u, &consumer::AimBasis::zeroing_distance_m}, ConsumerField{8u, &consumer::AimBasis::zeroing_correction_degrees}, ConsumerField{9u, &consumer::AimBasis::integrated_steps}, ConsumerField{10u, &consumer::AimBasis::zeroing_reached_distance}, ConsumerField{11u, &consumer::AimBasis::integrated_distance_cm}, ConsumerField{12u, &consumer::AimBasis::branch}, ConsumerField{13u, &consumer::AimBasis::stage}, ConsumerField{14u, &consumer::AimBasis::native_zeroing_applied}, ConsumerField{15u, &consumer::AimBasis::native_profile}, ConsumerField{16u, &consumer::AimBasis::controller_token}};
}
} // namespace monitor::synchronization::codec::detail
