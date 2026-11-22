#pragma once
#include <array>
#include <optional>
#include <string_view>

namespace monitor {
// 人体主干的语义集合；数组位置不是游戏骨骼索引，实际索引由本批参考骨架解析。
inline constexpr std::array<std::string_view, 20> BodyBoneNames{
    "pelvis",     "spine_01",   "spine_02", "spine_03",   "neck_01",    "head",       "clavicle_l",
    "upperarm_l", "lowerarm_l", "hand_l",   "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r",
    "thigh_l",    "calf_l",     "foot_l",   "thigh_r",    "calf_r",     "foot_r"};
// 对应语义父节点；骨盆的原生父节点可以是未采集的 root，不能捏造或重新编号。
inline constexpr std::array<int, 20> BodyBoneParents{-1, 0,  1,  2,  3, 4,  3,  6, 7,  8,
                                                     3,  10, 11, 12, 0, 14, 15, 0, 17, 18};
inline std::optional<std::size_t> BodyBoneRole(std::string_view name) noexcept {
    for (std::size_t role = 0; role < BodyBoneNames.size(); ++role) {
        const auto expected = BodyBoneNames[role];
        if (name.size() != expected.size())
            continue;
        bool equal = true;
        for (std::size_t i = 0; i < name.size(); ++i) {
            const auto c = name[i];
            if ((c >= 'A' && c <= 'Z' ? char(c - 'A' + 'a') : c) != expected[i]) {
                equal = false;
                break;
            }
        }
        if (equal)
            return role;
    }
    return {};
}
} // namespace monitor
