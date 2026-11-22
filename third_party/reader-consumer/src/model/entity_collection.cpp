#include "model/entity_admission.hpp"
#include "model/actor_directory.hpp"
#include "memory/address.hpp"
#include <algorithm>
#include <bit>
#include <deque>
#include <limits>
#include <unordered_map>
#include <unordered_set>

namespace monitor::model {
bool CurrentSceneSlot(const Snapshot& f, const SceneActor& row) noexcept {
    return f.scene_list_valid && row.index >= 0 && row.source_level == f.level &&
           row.source_container == f.scene_header.data && row.index < f.scene_header.num &&
           std::size_t(row.index) < f.scene_actor_slots.size() && f.scene_actor_slots[row.index] == row.raw;
}
namespace {
struct SceneSlotKey {
    u64 level = 0, container = 0;
    i32 index = -1;
    bool operator==(const SceneSlotKey&) const = default;
};
struct SceneSlotHash {
    std::size_t operator()(const SceneSlotKey& key) const noexcept {
        const auto combined = key.level ^ (key.container + 0x9e3779b97f4a7c15ULL + (key.level << 6) + (key.level >> 2));
        return static_cast<std::size_t>(combined ^ static_cast<u32>(key.index));
    }
};
using SceneSlots = std::unordered_map<SceneSlotKey, u64, SceneSlotHash>;
u64 Ns(Clock::time_point value) {
    return static_cast<u64>(std::max<std::int64_t>(
        0, std::chrono::duration_cast<std::chrono::nanoseconds>(value.time_since_epoch()).count()));
}
u64 CoreEnd(const Snapshot& frame) {
    return std::max(Ns(frame.started),
                    Ns(frame.core_finished == Clock::time_point{} ? frame.finished : frame.core_finished));
}
std::string Bytes(u64 value, unsigned count) {
    std::string bytes;
    bytes.reserve(count);
    for (unsigned i = 0; i < count; ++i)
        bytes.push_back(static_cast<char>((value >> (8 * i)) & 255));
    return bytes;
}
std::string PositionBytes(const Vec3& position) {
    return Bytes(std::bit_cast<u32>(position.x), 4) + Bytes(std::bit_cast<u32>(position.y), 4) +
           Bytes(std::bit_cast<u32>(position.z), 4);
}
void RecordIssue(const Snapshot& frame, EntityCollection& out, const Issue& issue, u64 sequence, u64 begin, u64 end) {
    if (DetailedSubscription(frame.subscription_profile) && issue)
        out.issues.push_back({issue, sequence, begin, std::max(begin, end)});
}
EntitySource RosterSource(const Snapshot& frame, u64 state, std::optional<i32> index) {
    return {SourceKind::PlayerRoster,
            frame.players_header.data,
            0,
            index,
            state,
            frame.sequence,
            Ns(frame.started),
            CoreEnd(frame)};
}
EntitySource SceneSource(const Snapshot& frame, const SceneActor& actor) {
    const u64 begin = Ns(frame.scene_list_started);
    return {SourceKind::SceneArray,
            frame.scene_header.data,
            frame.level,
            actor.index,
            actor.raw,
            frame.directory_generation,
            begin,
            std::max(begin, Ns(frame.scene_list_finished))};
}
bool CurrentRow(const Snapshot& frame, const SceneActor& row) {
    if (row.source_level != frame.level || row.source_container != frame.scene_header.data ||
        !UserAddress(row.source_level) || !UserAddress(row.source_container))
        return false;
    if (!CurrentSceneSlot(frame, row))
        return false;
    return ValidDirectoryObservation(frame);
}
template <class Sample> bool CurrentSource(const SceneSlots& slots, const Sample& sample) {
    if (sample.source_index < 0 || !UserAddress(sample.source_level) || !UserAddress(sample.source_container))
        return false;
    const auto found = slots.find({sample.source_level, sample.source_container, sample.source_index});
    return found != slots.end() && found->second == sample.actor;
}
RawField Field(std::string name, u64 owner, std::optional<u64> offset, std::string bytes, u64 sequence, u64 begin,
               u64 end, bool partial = false) {
    return {std::move(name), owner, offset, std::move(bytes), sequence, begin, std::max(begin, end), partial};
}
Character MakeCharacter(const Snapshot& frame, const Player& player) {
    Character result;
    result.actor_id = player.pawn;
    result.player_state_id = player.state;
    result.actor_identity_verified = player.actor_identity_verified;
    result.root_component_id = UserAddress(player.root) ? player.root : 0;
    if (player.index >= 0)
        result.roster_index = player.index;
    result.team_id = player.team;
    result.bot_classification = player.bot_classification;
    result.character_type = player.character_type;
    result.skeleton = player.skeleton;
    result.movement = player.movement;
    result.dbno_raw = player.dbno_raw;
    result.killed_raw = player.killed_raw;
    result.display_name = player.display_name;
    result.sample_capture_sequence = frame.sequence;
    result.observed_started_monotonic_ns = Ns(frame.started);
    result.observed_finished_monotonic_ns = CoreEnd(frame);
    result.acquisition_issue = player.acquisition_issue;
    if (DetailedSubscription(frame.subscription_profile)) {
        result.rejected_position_raw = player.rejected_position_raw;
        result.rejected_position_root = player.rejected_position_root;
    }
    if (player.position && ValidPosition(*player.position)) {
        result.position = player.position;
        result.origin = frame.origin;
    } else if (player.position) {
        if (DetailedSubscription(frame.subscription_profile) && !result.rejected_position_raw) {
            result.rejected_position_raw = player.position;
            result.rejected_position_root = player.root;
        }
        if (!result.acquisition_issue)
            result.acquisition_issue = {Code::InvalidLayout, "ENTITY_SPATIAL_VALIDATION",
                                        "Nonfinite/out-of-range character coordinates", player.root, sizeof(Vec3)};
    }

    if (DetailedSubscription(frame.subscription_profile))
        for (const auto& raw : player.raw_fields)
            result.raw_fields.push_back(Field(raw.field_name, raw.owner, raw.offset, raw.bytes,
                                              raw.observation.capture_sequence, Ns(raw.observation.started),
                                              Ns(raw.observation.finished),
                                              raw.observation.latest_status != OwnedReadStatus::Available));
    return result;
}
} // namespace

bool CurrentVehicleClassification(const Snapshot& frame, const VehicleSample& sample) {
    const auto& o = sample.classification;
    return frame.complete && frame.sequence && UserAddress(sample.actor) && UserAddress(sample.vtable) &&
           !sample.classification_rule_id.empty() &&
           static_cast<unsigned>(sample.kind) <= static_cast<unsigned>(VehicleKind::Special) &&
           o.capture_sequence == frame.sequence && o.started != Clock::time_point{} && o.started >= frame.started &&
           o.finished >= o.started && o.finished <= frame.finished && o.latest_status == OwnedReadStatus::Available &&
           o.value_status == OwnedReadStatus::Available && !o.issue;
}

bool CurrentPickupClassification(const Snapshot& frame, const PickupSample& sample) {
    const auto& o = sample.classification;
    return frame.complete && frame.sequence && UserAddress(sample.actor) && UserAddress(sample.vtable) &&
           !sample.classification_rule_id.empty() && o.capture_sequence == frame.sequence &&
           o.started != Clock::time_point{} && o.started >= frame.started && o.finished >= o.started &&
           o.finished <= frame.finished && o.latest_status == OwnedReadStatus::Available &&
           o.value_status == OwnedReadStatus::Available && !o.issue;
}

bool CurrentCreatureClassification(const Snapshot& frame, const CreatureSample& sample) {
    const auto& o = sample.classification;
    return frame.complete && frame.sequence && UserAddress(frame.session.base) && UserAddress(sample.actor) &&
           UserAddress(sample.player_state) && sample.actor != sample.player_state && UserAddress(sample.vtable) &&
           UserAddress(sample.player_state_vtable) && sample.vtable >= frame.session.base &&
           sample.player_state_vtable >= frame.session.base && sample.object_internal_index &&
           *sample.object_internal_index <= u32(std::numeric_limits<i32>::max()) && sample.object_serial_number &&
           sample.character_type_raw && !sample.interpretation_revision.empty() &&
           o.capture_sequence == frame.sequence && o.started != Clock::time_point{} && o.started >= frame.started &&
           o.finished >= o.started && o.finished <= frame.finished && o.latest_status == OwnedReadStatus::Available &&
           o.value_status == OwnedReadStatus::Available && !o.issue;
}

bool CurrentContainerClassification(const Snapshot& frame, const ContainerSample& sample) {
    const auto& o = sample.classification;
    return frame.complete && frame.sequence && UserAddress(sample.actor) && UserAddress(sample.vtable) &&
           sample.kind == ContainerKind::LootGroup && !sample.classification_rule_id.empty() &&
           sample.object_internal_index && *sample.object_internal_index >= 0 && sample.object_serial_number &&
           o.capture_sequence == frame.sequence && o.started != Clock::time_point{} && o.started >= frame.started &&
           o.finished >= o.started && o.finished <= frame.finished && o.latest_status == OwnedReadStatus::Available &&
           o.value_status == OwnedReadStatus::Available && !o.issue;
}

EntityCollection CollectEntities(const Snapshot& frame) {
    EntityCollection out;
    if (!frame.complete)
        return out;
    out.characters.reserve(frame.players.size());
    auto& stats = out.classification;

    // The bipartite graph is built exclusively from checked collector rows.
    // A conflict poisons its entire connected component, not merely the first
    // edge found; neither a scene alias nor a duplicate source repairs it.
    std::unordered_map<u64, std::unordered_set<u64>> pawn_states, state_pawns;
    std::unordered_map<u64, const Player*> candidates, invalid_candidates, unmatched, scene_characters;
    for (const auto& player : frame.players) {
        if (!UserAddress(player.pawn) ||
            (player.state ? !UserAddress(player.state) : !player.actor_identity_verified)) {
            if (UserAddress(player.state))
                invalid_candidates.try_emplace(player.state, &player);
            continue;
        }
        pawn_states[player.pawn].insert(player.state);
        if (player.state) {
            state_pawns[player.state].insert(player.pawn);
            candidates.try_emplace(player.state, &player);
        }
        if (player.actor_identity_verified)
            scene_characters.try_emplace(player.pawn, &player);
    }
    for (const auto& player : frame.unmatched)
        if (UserAddress(player.state))
            unmatched.try_emplace(player.state, &player);
    std::unordered_set<u64> conflicting_pawns, conflicting_states;
    SceneSlots current_slots;
    current_slots.reserve(frame.scene_actors.size());
    for (const auto& row : frame.scene_actors)
        if (CurrentRow(frame, row) && UserAddress(row.raw) && (!row.actor || row.actor == row.raw))
            current_slots.try_emplace(SceneSlotKey{row.source_level, row.source_container, row.index}, row.raw);
    std::unordered_map<u64, std::shared_ptr<const VehicleSample>> vehicle_candidates;
    std::unordered_set<u64> conflicting_vehicles;
    for (const auto& sample : frame.vehicles) {
        if (!sample || !CurrentVehicleClassification(frame, *sample) || !CurrentSource(current_slots, *sample))
            continue;
        const auto [found, inserted] = vehicle_candidates.try_emplace(sample->actor, sample);
        if (!inserted && (found->second->vtable != sample->vtable || found->second->raw_name != sample->raw_name ||
                          found->second->kind != sample->kind ||
                          found->second->classification_rule_id != sample->classification_rule_id))
            conflicting_vehicles.insert(sample->actor);
        if (pawn_states.count(sample->actor)) {
            conflicting_vehicles.insert(sample->actor);
            conflicting_pawns.insert(sample->actor);
        }
    }
    std::unordered_map<u64, std::shared_ptr<const PickupSample>> pickup_candidates;
    std::unordered_set<u64> conflicting_pickups;
    for (const auto& sample : frame.pickups) {
        if (!sample || !CurrentPickupClassification(frame, *sample) || !CurrentSource(current_slots, *sample))
            continue;
        const auto [found, inserted] = pickup_candidates.try_emplace(sample->actor, sample);
        if (!inserted && (found->second->vtable != sample->vtable || found->second->raw_name != sample->raw_name ||
                          found->second->object_internal_index != sample->object_internal_index ||
                          found->second->object_serial_number != sample->object_serial_number ||
                          found->second->classification_rule_id != sample->classification_rule_id))
            conflicting_pickups.insert(sample->actor);
        if (pawn_states.count(sample->actor)) {
            conflicting_pickups.insert(sample->actor);
            conflicting_pawns.insert(sample->actor);
        }
        if (vehicle_candidates.count(sample->actor)) {
            conflicting_pickups.insert(sample->actor);
            conflicting_vehicles.insert(sample->actor);
        }
    }
    std::unordered_map<u64, std::shared_ptr<const CreatureSample>> creature_candidates;
    std::unordered_set<u64> conflicting_creatures;
    for (const auto& sample : frame.creatures) {
        if (!sample || !CurrentCreatureClassification(frame, *sample) || !CurrentSource(current_slots, *sample))
            continue;
        const auto [found, inserted] = creature_candidates.try_emplace(sample->actor, sample);
        if (!inserted && (found->second->vtable != sample->vtable || found->second->raw_name != sample->raw_name ||
                          found->second->player_state != sample->player_state ||
                          found->second->player_state_vtable != sample->player_state_vtable ||
                          found->second->object_internal_index != sample->object_internal_index ||
                          found->second->object_serial_number != sample->object_serial_number ||
                          found->second->character_type_raw != sample->character_type_raw ||
                          found->second->species != sample->species ||
                          found->second->interpretation_revision != sample->interpretation_revision))
            conflicting_creatures.insert(sample->actor);
        if (pawn_states.count(sample->actor)) {
            conflicting_creatures.insert(sample->actor);
            conflicting_pawns.insert(sample->actor);
        }
        if (vehicle_candidates.count(sample->actor)) {
            conflicting_creatures.insert(sample->actor);
            conflicting_vehicles.insert(sample->actor);
        }
        if (pickup_candidates.count(sample->actor)) {
            conflicting_creatures.insert(sample->actor);
            conflicting_pickups.insert(sample->actor);
        }
    }
    std::unordered_map<u64, std::shared_ptr<const ContainerSample>> container_candidates;
    std::unordered_set<u64> conflicting_containers;
    for (const auto& sample : frame.containers) {
        if (!sample || !CurrentContainerClassification(frame, *sample) || !CurrentSource(current_slots, *sample))
            continue;
        const auto [found, inserted] = container_candidates.try_emplace(sample->actor, sample);
        if (!inserted && (found->second->vtable != sample->vtable || found->second->raw_name != sample->raw_name ||
                          found->second->object_internal_index != sample->object_internal_index ||
                          found->second->object_serial_number != sample->object_serial_number ||
                          found->second->kind != sample->kind ||
                          found->second->classification_rule_id != sample->classification_rule_id))
            conflicting_containers.insert(sample->actor);
        if (pawn_states.count(sample->actor)) {
            conflicting_containers.insert(sample->actor);
            conflicting_pawns.insert(sample->actor);
        }
        if (vehicle_candidates.count(sample->actor)) {
            conflicting_containers.insert(sample->actor);
            conflicting_vehicles.insert(sample->actor);
        }
        if (pickup_candidates.count(sample->actor)) {
            conflicting_containers.insert(sample->actor);
            conflicting_pickups.insert(sample->actor);
        }
        if (creature_candidates.count(sample->actor)) {
            conflicting_containers.insert(sample->actor);
            conflicting_creatures.insert(sample->actor);
        }
    }
    std::deque<std::pair<bool, u64>> pending;
    for (const auto pawn : conflicting_pawns)
        pending.emplace_back(true, pawn);
    for (const auto& [pawn, states] : pawn_states)
        if (states.size() > 1 && conflicting_pawns.insert(pawn).second)
            pending.emplace_back(true, pawn);
    for (const auto& [state, pawns] : state_pawns)
        if (pawns.size() > 1 && conflicting_states.insert(state).second)
            pending.emplace_back(false, state);
    while (!pending.empty()) {
        const auto [is_pawn, address] = pending.front();
        pending.pop_front();
        if (is_pawn) {
            for (const u64 state : pawn_states.at(address))
                if (state && conflicting_states.insert(state).second)
                    pending.emplace_back(false, state);
        } else {
            for (const u64 pawn : state_pawns.at(address))
                if (conflicting_pawns.insert(pawn).second)
                    pending.emplace_back(true, pawn);
        }
    }

    std::unordered_map<u64, std::size_t> admitted_states, admitted_pawns;
    const auto roster_slot = [&](u64 state, std::optional<i32> index, const Player* source_row) {
        ++stats.examined_source_slots;
        if (!UserAddress(state)) {
            ++stats.skipped_invalid_slots;
            if (DetailedSubscription(frame.subscription_profile))
                RecordIssue(
                    frame, out,
                    {Code::InvalidAddress, "ENTITY_PLAYERSTATE_ADDRESS", "Invalid PlayerArray source pointer", state},
                    frame.sequence, Ns(frame.started), CoreEnd(frame));
            return;
        }
        if (conflicting_states.count(state)) {
            ++stats.skipped_conflicting_slots;
            if (DetailedSubscription(frame.subscription_profile))
                RecordIssue(frame, out,
                            {Code::SnapshotChanged, "ENTITY_CLASSIFICATION_CONFLICT",
                             "PlayerState/Pawn relation is not one-to-one", state},
                            frame.sequence, Ns(frame.started), CoreEnd(frame));
            return;
        }
        const auto found = candidates.find(state);
        if (found == candidates.end()) {
            if (invalid_candidates.count(state)) {
                ++stats.skipped_invalid_slots;
                const auto& player = *invalid_candidates.at(state);
                if (DetailedSubscription(frame.subscription_profile))
                    RecordIssue(frame, out,
                                player.acquisition_issue ? player.acquisition_issue
                                                         : Issue{Code::InvalidAddress, "ENTITY_PAWN_ADDRESS",
                                                                 "Invalid verified Pawn address", player.pawn},
                                frame.sequence, Ns(frame.started), CoreEnd(frame));
            } else {
                ++stats.skipped_unclassified_slots;
                if (!source_row) {
                    const auto known = unmatched.find(state);
                    if (known != unmatched.end())
                        source_row = known->second;
                }
                if (source_row)
                    if (DetailedSubscription(frame.subscription_profile))
                        RecordIssue(frame, out, source_row->acquisition_issue, frame.sequence, Ns(frame.started),
                                    CoreEnd(frame));
            }
            return;
        }
        ++stats.admitted_source_slots;
        const auto existing = admitted_states.find(state);
        if (existing != admitted_states.end()) {
            ++stats.duplicate_source_slots;
            out.characters[existing->second].sources.push_back(RosterSource(frame, state, index));
            return;
        }
        const Player& player = *found->second;
        const auto slot = out.characters.size();
        admitted_states.emplace(state, slot);
        admitted_pawns.emplace(player.pawn, slot);
        auto character = MakeCharacter(frame, player);
        if (index)
            character.roster_index = index;
        character.sources.push_back(RosterSource(frame, state, index));
        out.characters.push_back(std::move(character));
    };
    if (!frame.player_roster_slots.empty()) {
        for (std::size_t i = 0; i < frame.player_roster_slots.size(); ++i)
            roster_slot(frame.player_roster_slots[i], static_cast<i32>(i), nullptr);
    } else {
        for (const auto& player : frame.players)
            roster_slot(player.state, player.index >= 0 ? std::optional<i32>(player.index) : std::nullopt, &player);
        for (const auto& player : frame.unmatched)
            roster_slot(player.state, player.index >= 0 ? std::optional<i32>(player.index) : std::nullopt, &player);
    }

    // Cached scene rows are not a current list until the collector accepts that
    // list. A checked current Pawn can supply classification to a scene source,
    // but cannot prove the cached source's optional fields still belong to it.
    if (!frame.scene_list_valid)
        return out;
    std::unordered_map<u64, std::size_t> admitted_vehicles;
    std::unordered_map<u64, std::size_t> admitted_pickups;
    std::unordered_map<u64, std::size_t> admitted_creatures;
    std::unordered_map<u64, std::size_t> admitted_containers;
    for (const auto& actor : frame.scene_actors) {
        ++stats.examined_source_slots;
        if (!CurrentRow(frame, actor)) {
            ++stats.skipped_invalid_slots;
            if (DetailedSubscription(frame.subscription_profile))
                RecordIssue(frame, out,
                            {Code::SnapshotChanged, "ENTITY_SCENE_SOURCE_REJECTED",
                             "Actor source Level/container/slot is not a current closed copy", actor.raw},
                            frame.sequence, Ns(frame.started), Ns(frame.finished));
            continue;
        }
        const auto source = SceneSource(frame, actor);
        if (!UserAddress(actor.raw) || (actor.actor && actor.actor != actor.raw)) {
            ++stats.skipped_invalid_slots;
            if (DetailedSubscription(frame.subscription_profile))
                RecordIssue(frame, out,
                            {Code::InvalidAddress, "ENTITY_SCENE_ADDRESS",
                             "Invalid or inconsistent Actor source pointer", actor.raw},
                            source.capture_sequence, source.started_ns, source.finished_ns);
            continue;
        }
        if (conflicting_pawns.count(actor.raw) || conflicting_vehicles.count(actor.raw) ||
            conflicting_pickups.count(actor.raw) || conflicting_creatures.count(actor.raw) ||
            conflicting_containers.count(actor.raw)) {
            ++stats.skipped_conflicting_slots;
            if (DetailedSubscription(frame.subscription_profile))
                RecordIssue(frame, out,
                            {Code::SnapshotChanged, "ENTITY_CLASSIFICATION_CONFLICT",
                             "Actor has conflicting type or ownership evidence", actor.raw},
                            source.capture_sequence, source.started_ns, source.finished_ns);
            continue;
        }
        const auto found = admitted_pawns.find(actor.raw);
        if (found == admitted_pawns.end()) {
            // 在线名单不是身体的存在性证明。已核验的主 Level 身体可没有 PlayerState，
            // 或持有已退出在线名单的有效 PlayerState；只挂实际 SceneArray 来源。
            const auto body = scene_characters.find(actor.raw);
            if (body != scene_characters.end()) {
                ++stats.admitted_source_slots;
                admitted_pawns.emplace(actor.raw, out.characters.size());
                auto character = MakeCharacter(frame, *body->second);
                character.sources.push_back(source);
                out.characters.push_back(std::move(character));
                continue;
            }
            const auto vehicle = vehicle_candidates.find(actor.raw);
            if (vehicle != vehicle_candidates.end()) {
                ++stats.admitted_source_slots;
                const auto old = admitted_vehicles.find(actor.raw);
                if (old != admitted_vehicles.end()) {
                    ++stats.duplicate_source_slots;
                    out.vehicles[old->second].sources.push_back(source);
                } else {
                    admitted_vehicles.emplace(actor.raw, out.vehicles.size());
                    out.vehicles.push_back({0, vehicle->second, {source}});
                }
                continue;
            }
            const auto pickup = pickup_candidates.find(actor.raw);
            if (pickup != pickup_candidates.end()) {
                ++stats.admitted_source_slots;
                const auto old = admitted_pickups.find(actor.raw);
                if (old != admitted_pickups.end()) {
                    ++stats.duplicate_source_slots;
                    out.pickups[old->second].sources.push_back(source);
                } else {
                    admitted_pickups.emplace(actor.raw, out.pickups.size());
                    out.pickups.push_back({0, pickup->second, {source}});
                }
                continue;
            }
            const auto creature = creature_candidates.find(actor.raw);
            if (creature != creature_candidates.end()) {
                ++stats.admitted_source_slots;
                const auto old = admitted_creatures.find(actor.raw);
                if (old != admitted_creatures.end()) {
                    ++stats.duplicate_source_slots;
                    out.creatures[old->second].sources.push_back(source);
                } else {
                    admitted_creatures.emplace(actor.raw, out.creatures.size());
                    out.creatures.push_back({0, creature->second, {source}});
                }
                continue;
            }
            const auto container = container_candidates.find(actor.raw);
            if (container != container_candidates.end()) {
                ++stats.admitted_source_slots;
                const auto old = admitted_containers.find(actor.raw);
                if (old != admitted_containers.end()) {
                    ++stats.duplicate_source_slots;
                    out.containers[old->second].sources.push_back(source);
                } else {
                    admitted_containers.emplace(actor.raw, out.containers.size());
                    out.containers.push_back({0, container->second, {source}});
                }
                continue;
            }
            ++stats.skipped_unclassified_slots;
            if (DetailedSubscription(frame.subscription_profile))
                RecordIssue(frame, out, actor.acquisition_issue, actor.sample_capture_sequence, Ns(actor.observed),
                            Ns(actor.observed_finished));
            continue;
        }
        ++stats.admitted_source_slots;
        ++stats.duplicate_source_slots;
        auto& character = out.characters[found->second];
        character.sources.push_back(source);
        if (!frame.sequence || actor.sample_capture_sequence != frame.sequence || actor.actor != actor.raw)
            continue;
        const u64 begin = Ns(actor.observed), end = std::max(begin, Ns(actor.observed_finished));
        // Optional current scene read failures remain diagnostics; they do not
        // overwrite independently checked character spatial/state observations.
        if (DetailedSubscription(frame.subscription_profile))
            RecordIssue(frame, out, actor.acquisition_issue, actor.sample_capture_sequence, begin, end);
    }
    return out;
}

} // namespace monitor::model
