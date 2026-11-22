#include "model/entity_admission.hpp"
#include "model/actor_directory.hpp"
#include "memory/address.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
namespace monitor::model {
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
u64 Ns(Clock::time_point t) {
    return static_cast<u64>(
        std::max<std::int64_t>(0, std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count()));
}
u64 CoreEnd(const Snapshot& f) {
    return std::max(Ns(f.started), Ns(f.core_finished == Clock::time_point{} ? f.finished : f.core_finished));
}
u64 Start(const Character& c) {
    return c.observed_started_monotonic_ns;
}
u64 End(const Character& c) {
    return std::max(Start(c), c.observed_finished_monotonic_ns);
}
bool OwnedObservationCurrent(const Snapshot& f, const OwnedObservation& o) {
    const auto valid = [](OwnedReadStatus s) { return static_cast<unsigned>(s) >= 1 && static_cast<unsigned>(s) <= 5; };
    return f.sequence && o.capture_sequence == f.sequence && o.started != Clock::time_point{} &&
           o.finished >= o.started && o.started >= f.started && o.finished <= f.finished && valid(o.latest_status) &&
           o.latest_status != OwnedReadStatus::NotAttempted &&
           (!o.value_status || (valid(*o.value_status) && *o.value_status != OwnedReadStatus::Failed &&
                                *o.value_status != OwnedReadStatus::NotAttempted));
}
bool OwnedRawCurrent(const Snapshot& f, const std::vector<OwnedRawField>& fields) {
    return std::all_of(fields.begin(), fields.end(), [&](const auto& field) {
        return field.observation.value_status && OwnedObservationCurrent(f, field.observation);
    });
}
bool VehicleRidersCurrent(const Snapshot& f, const VehicleSeatRidersSample& sample) {
    if (!OwnedObservationCurrent(f, sample.observation))
        return false;
    if (!sample.observation.value_status)
        return true;
    return std::all_of(sample.riders.begin(), sample.riders.end(),
                       [&](const auto& rider) { return OwnedObservationCurrent(f, rider.observation); });
}
bool VehicleSeatsCurrent(const Snapshot& f, const VehicleSeatsSample& sample) {
    if (!OwnedObservationCurrent(f, sample.observation))
        return false;
    if (!sample.observation.value_status)
        return true;
    return std::all_of(sample.seats.begin(), sample.seats.end(), [&](const auto& seat) {
        return OwnedObservationCurrent(f, seat.observation) &&
               (!seat.observation.value_status || !seat.riders || VehicleRidersCurrent(f, *seat.riders));
    });
}
} // namespace
bool CurrentGroundMember(const Snapshot& f, const ContainerSample& owner, const GroundItemMember& member) {
    const auto available = [&](const OwnedObservation& observation) {
        return OwnedObservationCurrent(f, observation) && observation.value_status == OwnedReadStatus::Available &&
               observation.latest_status == OwnedReadStatus::Available && !observation.issue;
    };
    if (!owner.contents || !OwnedObservationCurrent(f, owner.contents->observation) ||
        !owner.contents->observation.value_status || member.owner_actor != owner.actor ||
        !UserAddress(member.component_address) || member.component_address == owner.actor ||
        member.raw_component_value != member.component_address || member.slot_index < 0 ||
        !member.object_internal_index || *member.object_internal_index < 0 || !member.object_serial_number ||
        !member.native_vtable || !UserAddress(*member.native_vtable) || !member.class_address ||
        !UserAddress(*member.class_address) || member.classification_rule_id.empty() ||
        !available(member.observation) || !available(member.classification))
        return false;
    const auto& slots = owner.contents->slots;
    if (static_cast<std::size_t>(member.slot_index) >= slots.size())
        return false;
    const auto& slot = slots[static_cast<std::size_t>(member.slot_index)];
    return slot.slot_index == member.slot_index && slot.allocated && slot.status == GroundSlotStatus::Admitted &&
           (!slot.raw_component || *slot.raw_component == member.component_address);
}
EntityCollection ValidateAndAssignIds(const Snapshot& f, const EntityCollection& input) {
    auto result = input;
    std::unordered_set<u64> ids, pawns, states;
    u64 next = 1;
    u64 newly_conflicting_sources = 0;
    std::unordered_map<u64, unsigned> addresses, id_counts;
    std::unordered_set<u64> cross_type;
    const auto skipped = [&](u64 address, const char* stage, const char* message) {
        result.issues.push_back(
            {{Code::SnapshotChanged, stage, message, address}, f.sequence, Ns(f.started), Ns(f.finished)});
    };
    std::unordered_map<u64, u64> verified_states, verified_pawns;
    std::unordered_set<u64> conflicting_states, conflicting_pawns, roster_without_index;
    std::unordered_map<i32, u64> roster_slots;
    const auto remember_roster = [&](const Player& player) {
        if (player.index >= 0)
            roster_slots.try_emplace(player.index, player.state);
        else
            roster_without_index.insert(player.state);
    };
    for (const auto& player : f.players) {
        remember_roster(player);
        if (!UserAddress(player.pawn) || (player.state ? !UserAddress(player.state) : !player.actor_identity_verified))
            continue;
        if (player.state) {
            const auto [state, state_new] = verified_states.try_emplace(player.state, player.pawn);
            if (!state_new && state->second != player.pawn)
                conflicting_states.insert(player.state);
        }
        const auto [pawn, pawn_new] = verified_pawns.try_emplace(player.pawn, player.state);
        if (!pawn_new && pawn->second != player.state)
            conflicting_pawns.insert(player.pawn);
    }
    for (const auto& player : f.unmatched)
        remember_roster(player);
    std::erase_if(result.characters, [&](const auto& character) {
        const auto verified = verified_states.find(character.player_state_id);
        const auto body = verified_pawns.find(character.actor_id);
        const bool association =
            character.player_state_id
                ? verified != verified_states.end() && verified->second == character.actor_id
                : character.actor_identity_verified && body != verified_pawns.end() && body->second == 0;
        if (association && !conflicting_states.count(character.player_state_id) &&
            !conflicting_pawns.count(character.actor_id) && f.sequence &&
            character.sample_capture_sequence == f.sequence && Start(character) == Ns(f.started) &&
            End(character) == CoreEnd(f))
            return false;
        skipped(character.actor_id, "CHARACTER_ASSOCIATION_REJECTED",
                "Character does not match a currently verified PlayerState/Pawn association and sample");
        if (conflicting_states.count(character.player_state_id) || conflicting_pawns.count(character.actor_id))
            newly_conflicting_sources += character.sources.size();
        return true;
    });
    std::unordered_map<SceneSlotKey, const SceneActor*, SceneSlotHash> scene_slots;
    scene_slots.reserve(f.scene_actors.size());
    for (const auto& row : f.scene_actors) {
        if (row.source_level != f.level || row.source_container != f.scene_header.data ||
            !UserAddress(row.source_level) || !UserAddress(row.source_container))
            continue;
        if (CurrentSceneSlot(f, row) && UserAddress(row.raw) && (!row.actor || row.actor == row.raw))
            scene_slots.try_emplace(SceneSlotKey{row.source_level, row.source_container, row.index}, &row);
    }
    const auto current_source = [&](const EntitySource& source, u64 actor) {
        if (source.kind != SourceKind::SceneArray || !source.slot_index || *source.slot_index < 0 ||
            !UserAddress(source.level_address) || !UserAddress(source.container_address) ||
            source.level_address != f.level || source.container_address != f.scene_header.data ||
            source.raw_value != actor || source.capture_sequence != f.directory_generation)
            return false;
        const auto found = scene_slots.find({source.level_address, source.container_address, *source.slot_index});
        if (found == scene_slots.end() || found->second->raw != actor)
            return false;
        return MatchesDirectoryObservation(f, source.capture_sequence, source.started_ns, source.finished_ns);
    };
    const auto current_sample_source = [&](const auto& sample) {
        if (sample.source_index < 0 || !UserAddress(sample.source_level) || !UserAddress(sample.source_container) ||
            sample.source_level != f.level || sample.source_container != f.scene_header.data)
            return false;
        const auto found = scene_slots.find({sample.source_level, sample.source_container, sample.source_index});
        if (found == scene_slots.end() || found->second->raw != sample.actor)
            return false;
        return ValidDirectoryObservation(f);
    };
    const auto current_roster_source = [&](const EntitySource& source, u64 state) {
        if (source.kind != SourceKind::PlayerRoster || source.raw_value != state ||
            source.container_address != f.players_header.data || source.level_address ||
            source.capture_sequence != f.sequence || source.started_ns != Ns(f.started) ||
            source.finished_ns != CoreEnd(f))
            return false;
        if (!f.player_roster_slots.empty())
            return source.slot_index && *source.slot_index >= 0 &&
                   static_cast<std::size_t>(*source.slot_index) < f.player_roster_slots.size() &&
                   f.player_roster_slots[*source.slot_index] == state;
        // Explicit isolated fixtures can lack the exact copied PlayerArray, but
        // their claimed source must still exist among this Snapshot's checked rows.
        if (!source.slot_index)
            return roster_without_index.count(state) != 0;
        const auto found = roster_slots.find(*source.slot_index);
        return *source.slot_index >= 0 && found != roster_slots.end() && found->second == state;
    };
    const auto unique_sources = [&](auto& sources) {
        std::set<std::tuple<unsigned, u64, u64, i32, u64>> slots;
        std::erase_if(sources, [&](const auto& source) {
            return !slots
                        .emplace(static_cast<unsigned>(source.kind), source.level_address, source.container_address,
                                 source.slot_index.value_or(-1), source.raw_value)
                        .second;
        });
    };
    // Invalid optional scene aliases cannot remove an independently checked
    // roster association, but they must never escape as counterfeit provenance.
    for (auto& character : result.characters)
        std::erase_if(character.sources, [&](const auto& source) {
            if ((source.kind == SourceKind::SceneArray && current_source(source, character.actor_id)) ||
                (source.kind == SourceKind::PlayerRoster && current_roster_source(source, character.player_state_id)))
                return false;
            skipped(character.actor_id, "CHARACTER_SCENE_SOURCE_REJECTED",
                    "Character source is not a current roster or Level/container/slot");
            return true;
        });
    for (auto& character : result.characters)
        unique_sources(character.sources);
    for (auto& character : result.characters) {
        std::erase_if(character.raw_fields, [&](const auto& field) {
            if (field.sample_capture_sequence == f.sequence && field.observed_started_monotonic_ns &&
                field.observed_started_monotonic_ns >= Ns(f.started) &&
                field.observed_finished_monotonic_ns >= field.observed_started_monotonic_ns &&
                field.observed_finished_monotonic_ns <= Ns(f.finished))
                return false;
            skipped(character.actor_id, "CHARACTER_RAW_SAMPLE_REJECTED",
                    "Character raw field has no current checked sample");
            return true;
        });
    }
    std::erase_if(result.characters, [&](const auto& character) {
        if (!character.sources.empty())
            return false;
        skipped(character.actor_id, "CHARACTER_SOURCE_REJECTED", "Character has no current source occurrence");
        return true;
    });
    // 未核验/旧证据没有类型竞争资格，先拒绝，避免它污染当前人物准入。
    std::erase_if(result.vehicles, [&](auto& v) {
        if (!v.sample || !CurrentVehicleClassification(f, *v.sample) || v.sources.empty()) {
            skipped(v.sample ? v.sample->actor : 0, "VEHICLE_CLASSIFICATION_REJECTED",
                    "Vehicle classification is missing, stale or outside this capture");
            return true;
        }
        std::erase_if(v.sources, [&](const auto& source) { return !current_source(source, v.sample->actor); });
        unique_sources(v.sources);
        if (!current_sample_source(*v.sample) || v.sources.empty()) {
            skipped(v.sample->actor, "VEHICLE_SOURCE_REJECTED", "Vehicle source is not a current copied scene slot");
            return true;
        }
        return false;
    });
    std::erase_if(result.pickups, [&](auto& pickup) {
        if (!pickup.sample || !CurrentPickupClassification(f, *pickup.sample) || pickup.sources.empty()) {
            skipped(pickup.sample ? pickup.sample->actor : 0, "PICKUP_CLASSIFICATION_REJECTED",
                    "Pickup classification is missing, stale or outside this capture");
            return true;
        }
        std::erase_if(pickup.sources,
                      [&](const auto& source) { return !current_source(source, pickup.sample->actor); });
        unique_sources(pickup.sources);
        if (!current_sample_source(*pickup.sample) || pickup.sources.empty()) {
            skipped(pickup.sample->actor, "PICKUP_SOURCE_REJECTED", "Pickup source is not a current copied scene slot");
            return true;
        }
        return false;
    });
    std::erase_if(result.containers, [&](auto& container) {
        if (!container.sample || !CurrentContainerClassification(f, *container.sample) || container.sources.empty()) {
            skipped(container.sample ? container.sample->actor : 0, "CONTAINER_CLASSIFICATION_REJECTED",
                    "Container classification is missing, stale or outside this capture");
            return true;
        }
        std::erase_if(container.sources,
                      [&](const auto& source) { return !current_source(source, container.sample->actor); });
        unique_sources(container.sources);
        if (!current_sample_source(*container.sample) || container.sources.empty()) {
            skipped(container.sample->actor, "CONTAINER_SOURCE_REJECTED",
                    "Container source is not a current copied scene slot");
            return true;
        }
        return false;
    });
    std::erase_if(result.creatures, [&](auto& creature) {
        if (!creature.sample || !CurrentCreatureClassification(f, *creature.sample) || creature.sources.empty()) {
            skipped(creature.sample ? creature.sample->actor : 0, "CREATURE_CLASSIFICATION_REJECTED",
                    "Creature native type, object identity or PlayerState association is unverified or stale");
            return true;
        }
        std::erase_if(creature.sources,
                      [&](const auto& source) { return !current_source(source, creature.sample->actor); });
        unique_sources(creature.sources);
        if (!current_sample_source(*creature.sample) || creature.sources.empty()) {
            skipped(creature.sample->actor, "CREATURE_SOURCE_REJECTED",
                    "Creature source is not a current copied main Level slot");
            return true;
        }
        return false;
    });
    for (const auto& c : result.characters) {
        ++addresses[c.actor_id];
        if (c.entity_id)
            ++id_counts[c.entity_id];
    }
    for (const auto& v : result.vehicles) {
        if (addresses.count(v.sample->actor))
            cross_type.insert(v.sample->actor);
        ++addresses[v.sample->actor];
        if (v.entity_id)
            ++id_counts[v.entity_id];
    }
    for (const auto& pickup : result.pickups) {
        if (addresses.count(pickup.sample->actor))
            cross_type.insert(pickup.sample->actor);
        ++addresses[pickup.sample->actor];
        if (pickup.entity_id)
            ++id_counts[pickup.entity_id];
    }
    for (const auto& creature : result.creatures) {
        if (addresses.count(creature.sample->actor))
            cross_type.insert(creature.sample->actor);
        ++addresses[creature.sample->actor];
        if (creature.entity_id)
            ++id_counts[creature.entity_id];
    }
    for (const auto& container : result.containers) {
        if (addresses.count(container.sample->actor))
            cross_type.insert(container.sample->actor);
        ++addresses[container.sample->actor];
        if (container.entity_id)
            ++id_counts[container.entity_id];
    }
    std::erase_if(result.characters, [&](const auto& c) {
        if (!cross_type.count(c.actor_id))
            return false;
        newly_conflicting_sources += c.sources.size();
        skipped(c.actor_id, "ENTITY_CROSS_TYPE_CONFLICT", "Actor passed more than one entity type admission");
        return true;
    });
    std::erase_if(result.vehicles, [&](const auto& v) {
        if (cross_type.count(v.sample->actor) || addresses[v.sample->actor] > 1) {
            newly_conflicting_sources += v.sources.size();
            skipped(v.sample->actor, "ENTITY_CROSS_TYPE_CONFLICT", "Actor has duplicate or conflicting type admission");
            return true;
        }
        if (v.entity_id && id_counts[v.entity_id] > 1) {
            skipped(v.sample->actor, "VEHICLE_ID_CONFLICT", "Vehicle ID collides with another current entity");
            return true;
        }
        return false;
    });
    std::erase_if(result.pickups, [&](const auto& pickup) {
        if (cross_type.count(pickup.sample->actor) || addresses[pickup.sample->actor] > 1) {
            newly_conflicting_sources += pickup.sources.size();
            skipped(pickup.sample->actor, "ENTITY_CROSS_TYPE_CONFLICT",
                    "Actor has duplicate or conflicting type admission");
            return true;
        }
        if (pickup.entity_id && id_counts[pickup.entity_id] > 1) {
            skipped(pickup.sample->actor, "PICKUP_ID_CONFLICT", "Pickup ID collides with another current entity");
            return true;
        }
        return false;
    });
    std::erase_if(result.containers, [&](const auto& container) {
        if (cross_type.count(container.sample->actor) || addresses[container.sample->actor] > 1) {
            newly_conflicting_sources += container.sources.size();
            skipped(container.sample->actor, "ENTITY_CROSS_TYPE_CONFLICT",
                    "Actor has duplicate or conflicting type admission");
            return true;
        }
        if (container.entity_id && id_counts[container.entity_id] > 1) {
            skipped(container.sample->actor, "CONTAINER_ID_CONFLICT",
                    "Container ID collides with another current entity");
            return true;
        }
        return false;
    });
    std::erase_if(result.creatures, [&](const auto& creature) {
        if (cross_type.count(creature.sample->actor) || addresses[creature.sample->actor] > 1) {
            newly_conflicting_sources += creature.sources.size();
            skipped(creature.sample->actor, "ENTITY_CROSS_TYPE_CONFLICT",
                    "Actor has duplicate or conflicting type admission");
            return true;
        }
        if (creature.entity_id && id_counts[creature.entity_id] > 1) {
            skipped(creature.sample->actor, "CREATURE_ID_CONFLICT", "Creature ID collides with another current entity");
            return true;
        }
        return false;
    });
    for (const auto& c : result.characters) {
        if (!UserAddress(c.actor_id) ||
            (c.player_state_id ? !UserAddress(c.player_state_id) : !c.actor_identity_verified) || c.sources.empty() ||
            !pawns.insert(c.actor_id).second || (c.player_state_id && !states.insert(c.player_state_id).second) ||
            (c.position && !ValidPosition(*c.position)))
            throw std::invalid_argument("Unverified or duplicate character passed collection admission");
        if (c.entity_id && !ids.insert(c.entity_id).second)
            throw std::invalid_argument("Duplicate telemetry entity ID");
    }
    for (const auto& v : result.vehicles)
        if (v.entity_id)
            ids.insert(v.entity_id);
    for (const auto& pickup : result.pickups)
        if (pickup.entity_id)
            ids.insert(pickup.entity_id);
    for (const auto& creature : result.creatures)
        if (creature.entity_id)
            ids.insert(creature.entity_id);
    for (const auto& container : result.containers)
        if (container.entity_id)
            ids.insert(container.entity_id);
    for (auto& c : result.characters)
        if (!c.entity_id) {
            while (ids.count(next))
                ++next;
            c.entity_id = next;
            ids.insert(next++);
        }
    for (auto& v : result.vehicles)
        if (!v.entity_id) {
            while (ids.count(next))
                ++next;
            v.entity_id = next;
            ids.insert(next++);
        }
    for (auto& pickup : result.pickups)
        if (!pickup.entity_id) {
            while (ids.count(next))
                ++next;
            pickup.entity_id = next;
            ids.insert(next++);
        }
    for (auto& creature : result.creatures)
        if (!creature.entity_id) {
            while (ids.count(next))
                ++next;
            creature.entity_id = next;
            ids.insert(next++);
        }
    for (auto& container : result.containers)
        if (!container.entity_id) {
            while (ids.count(next))
                ++next;
            container.entity_id = next;
            ids.insert(next++);
        }
    std::unordered_map<u64, u64> character_ids;
    for (const auto& c : result.characters)
        character_ids.emplace(c.actor_id, c.entity_id);
    const auto rider_claim = [](const VehicleSeatRider& rider) -> u64 {
        if (!rider.observation.value_status)
            return 0;
        if (rider.pawn_address && UserAddress(*rider.pawn_address))
            return *rider.pawn_address;
        if (rider.raw_pawn_value && *rider.raw_pawn_value)
            return *rider.raw_pawn_value;
        return 0;
    };
    std::unordered_map<u64, unsigned> rider_claims;
    for (const auto& v : result.vehicles)
        if (v.sample->seats && VehicleSeatsCurrent(f, *v.sample->seats) && v.sample->seats->observation.value_status) {
            for (const auto& seat : v.sample->seats->seats)
                if (seat.observation.value_status && seat.riders && seat.riders->observation.value_status)
                    for (const auto& rider : seat.riders->riders)
                        if (const auto claim = rider_claim(rider))
                            ++rider_claims[claim];
        }
    for (auto& v : result.vehicles)
        if (v.sample->seats && VehicleSeatsCurrent(f, *v.sample->seats)) {
            auto sample = std::make_shared<VehicleSample>(*v.sample);
            auto& seats = *sample->seats;
            if (seats.observation.value_status)
                for (auto& seat : seats.seats)
                    if (seat.observation.value_status && seat.riders && seat.riders->observation.value_status) {
                        auto& riders = *seat.riders;
                        bool conflict = false;
                        std::erase_if(riders.riders, [&](const auto& rider) {
                            const auto claim = rider_claim(rider);
                            if (!claim || rider_claims[claim] <= 1)
                                return false;
                            conflict = true;
                            return true;
                        });
                        if (conflict) {
                            const Issue problem{Code::SnapshotChanged, "VEHICLE_RIDER_OWNERSHIP_CONFLICT",
                                                "Pawn has competing current vehicle/seat references", sample->actor};
                            riders.coverage = OwnedCoverage::Partial;
                            riders.active_members.reset();
                            riders.observation.latest_status = OwnedReadStatus::Failed;
                            riders.observation.value_status = OwnedReadStatus::Partial;
                            riders.observation.issue = problem;
                            seats.coverage = OwnedCoverage::Partial;
                            seats.occupied_slots.reset();
                            seats.observation.latest_status = OwnedReadStatus::Failed;
                            seats.observation.value_status = OwnedReadStatus::Partial;
                            seats.observation.issue = problem;
                            // 竞争关联既不能确认占座，也不能被描述成空座。
                            seat.occupancy = SeatOccupancy::Unknown;
                            seat.observation.latest_status = OwnedReadStatus::Failed;
                            seat.observation.value_status = OwnedReadStatus::Partial;
                            seat.observation.issue = problem;
                        }
                        for (auto& rider : riders.riders) {
                            rider.character_entity_id.reset();
                            if (!rider.observation.value_status || !rider.pawn_address)
                                continue;
                            const auto found = character_ids.find(*rider.pawn_address);
                            if (found != character_ids.end())
                                rider.character_entity_id = found->second;
                        }
                    }
            v.sample = std::move(sample);
        }
    // Encoding can defensively revoke an input source after collection. Such a
    // source must stop counting as admitted; the copied-list examined count and
    // its existing skip categories remain evidence from the original capture.
    u64 admitted_sources = 0;
    for (const auto& c : result.characters)
        admitted_sources += c.sources.size();
    for (const auto& v : result.vehicles)
        admitted_sources += v.sources.size();
    for (const auto& p : result.pickups)
        admitted_sources += p.sources.size();
    for (const auto& creature : result.creatures)
        admitted_sources += creature.sources.size();
    for (const auto& container : result.containers)
        admitted_sources += container.sources.size();
    auto& stats = result.classification;
    if (stats.admitted_source_slots > admitted_sources) {
        const auto revoked = stats.admitted_source_slots - admitted_sources;
        const auto conflict = std::min(revoked, newly_conflicting_sources);
        stats.skipped_conflicting_slots += conflict;
        stats.skipped_invalid_slots += revoked - conflict;
    }
    stats.admitted_source_slots = admitted_sources;
    const u64 admitted_entities = result.characters.size() + result.vehicles.size() + result.pickups.size() +
                                  result.creatures.size() + result.containers.size();
    stats.duplicate_source_slots = admitted_sources - admitted_entities;
    return result;
}
void ValidatePrepared(const Snapshot& f, EntityCollection& entities) {
    // The capture owner closed classification, sources and property tasks.
    // Validate the final entity boundary without reconstructing scene/roster
    // candidates or repeating that frame's full Actor admission pass.
    if (!f.scene_list_valid || f.scene_header.num < 0 || f.scene_header.max < f.scene_header.num ||
        std::size_t(f.scene_header.num) != f.scene_actor_slots.size())
        throw std::invalid_argument("Prepared pipeline frame has no complete opening Actor list");
    std::unordered_set<u64> ids, addresses;
    std::unordered_map<u64, u64> character_ids;
    const auto count = entities.characters.size() + entities.vehicles.size() + entities.pickups.size() +
                       entities.creatures.size() + entities.containers.size();
    ids.reserve(count);
    addresses.reserve(count);
    character_ids.reserve(entities.characters.size());
    const auto check = [&](u64 id, u64 address, const std::vector<EntitySource>& sources) {
        if (!id || !UserAddress(address) || !ids.insert(id).second || !addresses.insert(address).second ||
            sources.empty())
            throw std::invalid_argument("Prepared entity has missing/duplicate ID, source address or provenance");
        for (const auto& source : sources)
            if (source.kind != SourceKind::SceneArray || source.raw_value != address ||
                source.level_address != f.level || source.container_address != f.scene_header.data ||
                !source.slot_index || *source.slot_index < 0 ||
                std::size_t(*source.slot_index) >= f.scene_actor_slots.size() ||
                f.scene_actor_slots[std::size_t(*source.slot_index)] != address ||
                !MatchesDirectoryObservation(f, source.capture_sequence, source.started_ns, source.finished_ns))
                throw std::invalid_argument("Prepared source differs from its real directory observation");
    };
    for (const auto& c : entities.characters) {
        check(c.entity_id, c.actor_id, c.sources);
        if (!c.object_internal_index || *c.object_internal_index < 0 || !c.object_serial_number ||
            (c.player_state_id ? !UserAddress(c.player_state_id) : !c.actor_identity_verified) ||
            c.sample_capture_sequence != f.sequence || Start(c) < Ns(f.started) || End(c) > Ns(f.finished) ||
            c.position && !ValidPosition(*c.position))
            throw std::invalid_argument("Prepared character association or sample is invalid");
        for (const auto& field : c.raw_fields)
            if (field.sample_capture_sequence != f.sequence || field.observed_started_monotonic_ns < Ns(f.started) ||
                field.observed_finished_monotonic_ns < field.observed_started_monotonic_ns ||
                field.observed_finished_monotonic_ns > Ns(f.finished))
                throw std::invalid_argument("Prepared character raw sample is outside its capture interval");
        character_ids.emplace(c.actor_id, c.entity_id);
    }
    for (const auto& v : entities.vehicles) {
        if (!v.object_internal_index || *v.object_internal_index < 0 || !v.object_serial_number || !v.sample ||
            !CurrentVehicleClassification(f, *v.sample))
            throw std::invalid_argument("Prepared vehicle classification is not current");
        check(v.entity_id, v.sample->actor, v.sources);
    }
    for (const auto& p : entities.pickups) {
        if (!p.sample || !p.sample->object_internal_index || *p.sample->object_internal_index < 0 ||
            !p.sample->object_serial_number || !CurrentPickupClassification(f, *p.sample))
            throw std::invalid_argument("Prepared pickup classification is not current");
        check(p.entity_id, p.sample->actor, p.sources);
    }
    for (const auto& c : entities.creatures) {
        if (!c.sample || !CurrentCreatureClassification(f, *c.sample))
            throw std::invalid_argument("Prepared creature classification is not current");
        check(c.entity_id, c.sample->actor, c.sources);
    }
    for (const auto& c : entities.containers) {
        if (!c.sample || !CurrentContainerClassification(f, *c.sample))
            throw std::invalid_argument("Prepared container classification is not current");
        check(c.entity_id, c.sample->actor, c.sources);
    }
    std::unordered_map<u64, unsigned> rider_claims;
    const auto claim = [](const VehicleSeatRider& r) -> u64 {
        return r.observation.value_status
                   ? (r.pawn_address && UserAddress(*r.pawn_address) ? *r.pawn_address : r.raw_pawn_value.value_or(0))
                   : 0;
    };
    for (const auto& v : entities.vehicles)
        if (v.sample->seats && VehicleSeatsCurrent(f, *v.sample->seats))
            for (const auto& seat : v.sample->seats->seats)
                if (seat.riders && seat.observation.value_status && seat.riders->observation.value_status)
                    for (const auto& rider : seat.riders->riders)
                        if (const auto address = claim(rider))
                            ++rider_claims[address];
    for (auto& v : entities.vehicles)
        if (v.sample->seats && VehicleSeatsCurrent(f, *v.sample->seats)) {
            auto sample = std::make_shared<VehicleSample>(*v.sample);
            auto& seats = *sample->seats;
            if (seats.observation.value_status)
                for (auto& seat : seats.seats)
                    if (seat.riders && seat.observation.value_status && seat.riders->observation.value_status) {
                        auto& riders = *seat.riders;
                        bool conflict = false;
                        std::erase_if(riders.riders, [&](const auto& rider) {
                            const auto address = claim(rider);
                            if (!address || rider_claims[address] <= 1)
                                return false;
                            conflict = true;
                            return true;
                        });
                        if (conflict) {
                            const Issue problem{Code::SnapshotChanged, "VEHICLE_RIDER_OWNERSHIP_CONFLICT",
                                                "Pawn has competing current vehicle/seat references", sample->actor};
                            riders.coverage = OwnedCoverage::Partial;
                            riders.active_members.reset();
                            riders.observation.latest_status = OwnedReadStatus::Failed;
                            riders.observation.value_status = OwnedReadStatus::Partial;
                            riders.observation.issue = problem;
                            seats.coverage = OwnedCoverage::Partial;
                            seats.occupied_slots.reset();
                            seats.observation.latest_status = OwnedReadStatus::Failed;
                            seats.observation.value_status = OwnedReadStatus::Partial;
                            seats.observation.issue = problem;
                            seat.occupancy = SeatOccupancy::Unknown;
                            seat.observation.latest_status = OwnedReadStatus::Failed;
                            seat.observation.value_status = OwnedReadStatus::Partial;
                            seat.observation.issue = problem;
                        }
                        for (auto& rider : riders.riders) {
                            rider.character_entity_id.reset();
                            if (!rider.observation.value_status || !rider.pawn_address)
                                continue;
                            if (const auto found = character_ids.find(*rider.pawn_address);
                                found != character_ids.end())
                                rider.character_entity_id = found->second;
                        }
                    }
            v.sample = std::move(sample);
        }
}
} // namespace monitor::model
