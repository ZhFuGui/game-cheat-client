#pragma once
#include "model/entities/skeleton.hpp"
#include "model/entities/movement.hpp"
#include "model/raw_field.hpp"
#include <memory>
namespace monitor {
// 原始来源与解释结论独立保留；缺失结论表示未知，而非真人。
struct BotClassificationSample {
    OwnedObservation observation;
    std::optional<bool> is_bot;
    std::optional<u8> player_state_flags_raw;
    std::optional<u64> player_state_vtable;
    std::string interpretation_revision;
};
// ETslCharacterType 是独立的角色类型，不能与 Bot 标记或虚函数返回的另一套枚举混用。
struct CharacterTypeSample {
    u64 player_state = 0; // 实际读取对象；不能把其他人物的同帧标记贴到本人物。
    OwnedObservation observation;
    std::optional<u8> character_type_raw;
    std::optional<u32> character_type; // 仅解释当前映像定义的 1～9；0/MAX/其他值为未知。
    std::optional<u64> player_state_vtable;
    std::string interpretation_revision;
    bool native_identity_verified = false;
    std::vector<OwnedRawField> raw_fields;
};

struct Player {
    i32 index = -1;
    u64 state = 0, pawn = 0, root = 0;
    // 本批主 Level 人物的原生类型及 Pawn 生命周期均通过最终关闭核验。
    // 身体仍可存在而 PlayerState 已清空；不能用在线名单代替身体身份。
    bool actor_identity_verified = false;
    std::optional<i32> team;
    std::optional<u8> killed_raw, dbno_raw;
    std::optional<Vec3> position;
    std::optional<std::string> display_name; // 成功读取的空名称仍 present；缺失表示本次未获得。
    // Preserve bytes read before a coordinate/link check fails. These are raw
    // evidence only and never replace a successfully checked spatial sample.
    std::optional<Vec3> rejected_position_raw;
    u64 rejected_position_root = 0;
    bool self = false;
    std::string availability; // Diagnostic state, NOT an alive/dead classification.
    std::string mapping_source;
    Issue acquisition_issue;
    std::vector<OwnedRawField> raw_fields;
    // Immutable, current-capture owned samples; absent means not acquired.
    std::shared_ptr<const BotClassificationSample> bot_classification;
    std::shared_ptr<const CharacterTypeSample> character_type;
    std::shared_ptr<const CharacterSkeletonSample> skeleton;
    std::shared_ptr<const CharacterMovementSample> movement;
};
} // namespace monitor
