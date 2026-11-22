#include "synchronization/projection.hpp"
#include "diagnostics/work_progress.hpp"
#include "synchronization/container_projection.hpp"
#include "synchronization/local_projection.hpp"
#include "synchronization/skeleton_projection.hpp"
#include "synchronization/motion_weapon_projection.hpp"
#include "model/entity_collection.hpp"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace monitor::synchronization {
namespace {
using Fields = std::map<unsigned, Value>;
template <class T> Value Numeric(T value) {
    if constexpr (std::is_same_v<T, bool>)
        return Value(value);
    else if constexpr (std::is_floating_point_v<T>)
        return Value(double(value));
    else if constexpr (std::is_signed_v<T>)
        return Value(std::int64_t(value));
    else
        return Value(u64(value));
}
template <class T> void Put(Fields& out, unsigned field, const std::optional<T>& value) {
    if (value)
        out.emplace(field, Numeric(*value));
}
void Put(Fields& out, unsigned field, const std::optional<std::string>& value) {
    if (value)
        out.emplace(field, Value(*value));
}
Value Position(const Vec3& p) {
    return Value(Value::List{Value(double(p.x)), Value(double(p.y)), Value(double(p.z))});
}
Value Origin(const WorldOrigin& o) {
    return Value(Value::List{Value(std::int64_t(o.x)), Value(std::int64_t(o.y)), Value(std::int64_t(o.z))});
}
Fields NameFields(const ObjectName& name) {
    Fields out;
    Put(out, 1, name.raw_index);
    Put(out, 2, name.raw_number);
    Put(out, 3, name.index);
    Put(out, 4, name.number);
    if (name.verified)
        out.emplace(5, Value(name.full));
    return out;
}
Value Name(const ObjectName& name) {
    return Value(NameFields(name));
}
Value Reference(const std::string& reader, u64 epoch, u64 id, EntityKind kind, const InstanceIdentity& instance) {
    return Value(Value::Object{{1, Value(reader)},
                               {2, Value(epoch)},
                               {3, Value(id)},
                               {4, Value(u64(kind))},
                               {5, Value(Value::Object{{1, Value(instance.actor_address)},
                                                       {2, Value(std::int64_t(instance.internal_index))},
                                                       {3, Value(u64(instance.serial_number))}})}});
}
Value Seats(const VehicleSeatsSample& sample, const std::unordered_map<u64, u64>& character_ids,
            const std::string& reader, u64 epoch) {
    Value::List seats;
    for (const auto& seat : sample.seats) {
        Fields fields;
        Put(fields, 1, seat.seat_index);
        Put(fields, 2, seat.component_address);
        if (seat.observation.value_status)
            fields.emplace(3, Value(u64(seat.occupancy)));
        if (seat.riders) {
            Fields riders;
            if (seat.riders->observation.value_status) {
                Put(riders, 1, seat.riders->declared_slots);
                Put(riders, 2, seat.riders->active_members);
            }
            riders.emplace(3, Value(u64(seat.riders->coverage)));
            Value::List entries;
            for (const auto& rider : seat.riders->riders) {
                Fields entry;
                Put(entry, 1, rider.raw_pawn_value);
                if (rider.observation.value_status)
                    Put(entry, 2, rider.pawn_address);
                Put(entry, 3, rider.source_slot_index);
                if (rider.observation.value_status && rider.pawn_address)
                    if (const auto found = character_ids.find(*rider.pawn_address); found != character_ids.end()) {
                        entry.emplace(4, Value(found->second));
                        if (rider.instance_identity && !reader.empty())
                            entry.emplace(5, Reference(reader, epoch, found->second, EntityKind::Character,
                                                       *rider.instance_identity));
                    }
                entries.emplace_back(std::move(entry));
            }
            riders.emplace(4, Value(std::move(entries)));
            fields.emplace(4, Value(std::move(riders)));
        }
        seats.emplace_back(std::move(fields));
    }
    Fields out;
    out.emplace(1, Value(std::move(seats)));
    out.emplace(2, Value(u64(sample.coverage)));
    return Value(std::move(out));
}
template <class Sample> Fields Spatial(const Sample& sample) {
    Fields out;
    if (sample.position)
        out.emplace(1, Position(*sample.position));
    if (sample.origin)
        out.emplace(2, Origin(*sample.origin));
    Put(out, 3, sample.root_component_address);
    Put(out, 4, sample.raw_root_component_value);
    return out;
}
struct Typed {
    const model::Character* character = nullptr;
    const model::Vehicle* vehicle = nullptr;
    const model::Pickup* pickup = nullptr;
    const model::Creature* creature = nullptr;
    const model::Container* container = nullptr;
};
Fields Properties(const Snapshot& frame, const ActorFact& fact, const Typed& typed, PropertyGroup group,
                  const std::unordered_map<u64, u64>& character_ids, const std::string& reader, u64 epoch) {
    Fields out;
    if (group == PropertyGroup::Identity) {
        const auto& instance = *fact.identity;
        out.emplace(1, Value(Value::Object{{1, Value(instance.actor_address)},
                                           {2, Value(std::int64_t(instance.internal_index))},
                                           {3, Value(u64(instance.serial_number))}}));
        out.emplace(2, Value(u64(fact.kind)));
        const auto& sources = typed.character  ? typed.character->sources
                              : typed.vehicle  ? typed.vehicle->sources
                              : typed.pickup   ? typed.pickup->sources
                              : typed.creature ? typed.creature->sources
                                               : typed.container->sources;
        Value::List source_values;
        source_values.reserve(sources.size());
        for (const auto& source : sources) {
            Value::Object entry{{1, Value(u64(source.kind))},
                                {2, Value(source.container_address)},
                                {3, Value(source.level_address)},
                                {5, Value(source.raw_value)}};
            if (source.slot_index)
                entry.emplace(4, Value(std::int64_t(*source.slot_index)));
            source_values.emplace_back(std::move(entry));
        }
        out.emplace(3, Value(std::move(source_values)));
        return out;
    }
    if (const auto c = typed.character) {
        if (group == PropertyGroup::Spatial) {
            if (c->position)
                out.emplace(1, Position(*c->position));
            if (c->origin)
                out.emplace(2, Origin(*c->origin));
            if (c->root_component_id)
                out.emplace(3, Value(c->root_component_id));
        } else if (group == PropertyGroup::CharacterName)
            Put(out, 1, c->display_name);
        else if (group == PropertyGroup::CharacterState) {
            Put(out, 1, c->team_id);
            if (c->bot_classification) {
                Put(out, 2, c->bot_classification->is_bot);
                Put(out, 3, c->bot_classification->player_state_flags_raw);
                Put(out, 4, c->bot_classification->player_state_vtable);
            }
            if (c->character_type) {
                Put(out, 5, c->character_type->character_type_raw);
                Put(out, 6, c->character_type->character_type);
                Put(out, 4, c->character_type->player_state_vtable);
            }
        } else if (group == PropertyGroup::CharacterSkeleton && c->skeleton) {
            out = SkeletonFields(*c->skeleton);
        } else if (group == PropertyGroup::CharacterMovement && c->movement &&
                   MovementObservation(*c->movement).value_status) {
            if (c->movement->velocity)
                out.emplace(1, Position(*c->movement->velocity));
            if (const auto& component = c->movement->component)
                out.emplace(2, Value(Value::Object{{1, Value(component->actor_address)},
                                                   {2, Value(std::int64_t(component->internal_index))},
                                                   {3, Value(u64(component->serial_number))}}));
            Put(out, 3, c->movement->raw_component);
            if (c->movement->world)
                out.emplace(4, WorldVelocityValue(*c->movement->world));
        }
    } else if (const auto v = typed.vehicle) {
        const auto& sample = *v->sample;
        if (group == PropertyGroup::Spatial)
            out = Spatial(sample.spatial);
        else if (group == PropertyGroup::VehicleState && sample.state) {
            const auto& state = *sample.state;
            Put(out, 1, state.health);
            Put(out, 2, state.max_health);
            Put(out, 3, state.fuel);
            Put(out, 4, state.max_fuel);
            Put(out, 5, state.is_dying);
            Put(out, 6, state.is_broken);
            Put(out, 7, state.common_component_address);
            Put(out, 8, state.raw_common_component_value);
        } else if (group == PropertyGroup::VehicleSeats && sample.seats) {
            out.emplace(1, Seats(*sample.seats, character_ids, reader, epoch));
            Put(out, 2, sample.seats->declared_slots);
            Put(out, 3, sample.seats->occupied_slots);
        } else if (group == PropertyGroup::VehicleName)
            out = NameFields(sample.name);
        else if (group == PropertyGroup::VehicleModel && sample.model) {
            const auto& model = *sample.model;
            Put(out, 1, model.class_address);
            Put(out, 2, model.class_pointer_raw);
            if (model.class_name.verified)
                out.emplace(3, Name(model.class_name));
            Put(out, 4, model.model_key);
        } else if (group == PropertyGroup::VehicleClassification) {
            out.emplace(1, Value(u64(sample.kind)));
            out.emplace(2, Value(sample.vtable));
            out.emplace(3, Value(sample.classification_rule_id));
        }
    } else if (const auto p = typed.pickup) {
        const auto& sample = *p->sample;
        if (group == PropertyGroup::Spatial)
            out = Spatial(sample.spatial);
        else if (sample.item) {
            const auto& item = *sample.item;
            if (group == PropertyGroup::PickupOwner) {
                Put(out, 1, item.source_object_address);
                Put(out, 2, item.slot_index);
            } else if (group == PropertyGroup::PickupStack && item.stack) {
                Put(out, 1, item.stack->quantity);
                Put(out, 2, item.stack->durability);
                Put(out, 3, item.stack->max_durability);
            } else if (group == PropertyGroup::PickupDefinition && item.item_info) {
                const auto& info = *item.item_info;
                Put(out, 1, info.item_id);
                if (info.identifier)
                    out.emplace(2, Name(*info.identifier));
                Put(out, 3, info.display_name);
                Put(out, 4, info.definition_address);
            } else if (group == PropertyGroup::PickupItemClassification && item.item_info &&
                       item.item_info->classification && item.item_info->classification->value_status) {
                const auto& info = *item.item_info;
                out.emplace(1, Value(u64(info.category)));
                out.emplace(2, Value(u64(info.classification_status)));
                out.emplace(3, Value(info.classification_rule_id));
                out.emplace(4, Value(info.classification_rules_revision));
            }
        }
        if (group == PropertyGroup::PickupClassification) {
            out.emplace(1, Value(sample.vtable));
            out.emplace(2, Value(sample.classification_rule_id));
            out.emplace(3, Name(sample.instance_name_raw));
        }
    } else if (typed.container) {
        const auto& sample = *typed.container->sample;
        if (group == PropertyGroup::ContainerClassification) {
            out.emplace(1, Value(u64(sample.kind)));
            out.emplace(2, Value(sample.vtable));
            out.emplace(3, Value(sample.classification_rule_id));
            out.emplace(4, Name(sample.instance_name_raw));
        } else if (group == PropertyGroup::ContainerContents && sample.contents)
            out = detail::ContainerContentsFields(*sample.contents);
    } else if (typed.creature) {
        const auto& sample = *typed.creature->sample;
        if (group == PropertyGroup::Spatial)
            out = Spatial(sample.spatial);
        else if (group == PropertyGroup::CreatureClassification) {
            Put(out, 1, sample.character_type_raw);
            out.emplace(2, Value(u64(sample.species)));
            out.emplace(3, Value(sample.player_state));
            out.emplace(4, Value(sample.player_state_vtable));
            out.emplace(5, Value(sample.vtable));
            out.emplace(6, Value(sample.interpretation_revision));
            out.emplace(7, Name(sample.instance_name_raw));
        }
    }
    return out;
}
bool CurrentInterval(const SampleInterval& sample, const Snapshot& frame) {
    return sample.started != Clock::time_point{} && sample.started >= frame.started &&
           sample.finished >= sample.started && sample.finished <= frame.finished;
}
using NestedSamples = std::map<std::vector<unsigned>, GroupState::NestedSample>;
constexpr unsigned Element = 0x80000000u;
std::vector<unsigned> Child(std::vector<unsigned> path, unsigned field) {
    path.push_back(field);
    return path;
}
void Observe(NestedSamples& samples, std::vector<unsigned> path, const OwnedObservation& source) {
    GroupState::NestedSample sample;
    sample.capture_sequence = source.capture_sequence;
    sample.attempt = {source.started, source.finished};
    sample.latest_status = source.latest_status;
    sample.value_status = source.value_status;
    if (source.value_status)
        sample.source = sample.attempt;
    samples.emplace(std::move(path), sample);
}
NestedSamples Samples(const Typed& typed, PropertyGroup group, const Fields& fields) {
    if (group == PropertyGroup::ContainerContents && typed.container && typed.container->sample->contents)
        return detail::ContainerContentsSamples(*typed.container->sample->contents, fields);
    NestedSamples out;
    if (group == PropertyGroup::CharacterMovement && fields.contains(4) && typed.character &&
        typed.character->movement && typed.character->movement->world) {
        Observe(out, {4}, typed.character->movement->world->observation);
        return out;
    }
    if (!fields.contains(1))
        return out;
    if (group == PropertyGroup::VehicleSeats && typed.vehicle && typed.vehicle->sample->seats) {
        const auto& seats = *typed.vehicle->sample->seats;
        Observe(out, {1}, seats.observation);
        for (unsigned i = 0; i < seats.seats.size(); ++i) {
            const auto& seat = seats.seats[i];
            std::vector<unsigned> path{1, 1, Element | i};
            Observe(out, path, seat.observation);
            if (!seat.riders)
                continue;
            const auto riders_path = Child(path, 4);
            Observe(out, riders_path, seat.riders->observation);
            for (unsigned n = 0; n < seat.riders->riders.size(); ++n) {
                auto path = Child(riders_path, 4);
                path.push_back(Element | n);
                Observe(out, std::move(path), seat.riders->riders[n].observation);
            }
        }
    }
    return out;
}
void ProjectContext(const Snapshot& frame, State& out, const ProjectionOptions& options,
                    const std::unordered_map<u64, Typed>& typed, InstanceRegistry& registry) {
    std::map<ContextFactGroup, const ContextFact*> facts;
    for (const auto& fact : frame.current_facts->context_groups)
        if (!facts.emplace(fact.group, &fact).second)
            throw std::invalid_argument("Duplicate context fact");
    const auto terminal = [&](ContextGroup kind, FactStatus status, const SampleInterval& verified) {
        auto& sample = out.context[kind];
        sample.status = status;
        sample.verified_sample = verified;
        sample.verified_capture_sequence = frame.sequence;
        return &sample;
    };
    const auto from_fact = [&](ContextFactGroup group, ContextGroup kind, Fields fields) {
        const auto found = facts.find(group);
        if (found == facts.end())
            throw std::invalid_argument("Missing actual context fact");
        const auto& fact = *found->second;
        auto& sample = *terminal(kind, fact.status, fact.verified);
        if (!CurrentInterval(fact.verified, frame))
            throw std::invalid_argument("Context verification is outside capture");
        if (fact.present_fields & ~AllowedContextFields(kind))
            throw std::invalid_argument("Unknown current context field");
        if (fact.status != FactStatus::Available && fact.status != FactStatus::Partial) {
            if (fact.present_fields)
                throw std::invalid_argument("Unavailable context declares values");
            return &sample;
        }
        std::set<unsigned> source_fields;
        for (const auto& source : fact.field_sources) {
            if (!source.field || source.field > 64 || !(AllowedContextFields(kind) & (u64(1) << (source.field - 1))) ||
                !source_fields.insert(source.field).second)
                throw std::invalid_argument("Duplicate or undeclared context source");
            if (source.source != ContextSourceKind::Memory && source.source != ContextSourceKind::Interpretation)
                throw std::invalid_argument("Invalid context source kind");
            if (!(fact.present_fields & (u64(1) << (source.field - 1))))
                continue; // 失败尝试有真实窗口，但没有可发布值。
            const auto value = fields.find(source.field);
            if (value == fields.end())
                throw std::invalid_argument("Context proof has no actual value");
            sample.fields.emplace(source.field, value->second);
            sample.field_sources.emplace(source.field,
                                         GroupState::FieldSource{source.capture_sequence, source.sample, source.reused,
                                                                 source.source == ContextSourceKind::Memory ? 1u : 2u});
        }
        if (FieldMask(sample.fields) != fact.present_fields)
            throw std::invalid_argument("Context original field sources are incomplete");
        return &sample;
    };
    Fields world{{1, Value(frame.world_state)},
                 {2, Value(frame.world)},
                 {3, Value(frame.game_state_raw)},
                 {4, Value(frame.game_state)},
                 {5, Value(frame.level_raw)},
                 {6, Value(frame.level)},
                 {9, Value(frame.world_root_unverified)},
                 {10, Value(frame.world_cache_used)}};
    Put(world, 7, frame.time_seconds);
    Put(world, 8, frame.alive_teams_raw);
    auto& world_context = *from_fact(ContextFactGroup::World, ContextGroup::World, std::move(world));
    if (frame.category == DataCategory::Local) {
        Fields local{{1, Value(frame.game_instance_raw)}, {2, Value(frame.game_instance)},
                     {3, Value(frame.local_player)},      {4, Value(frame.controller)},
                     {5, Value(frame.self_pawn)},         {6, Value(frame.local_player_state)},
                     {7, Value(frame.local_character)}};
        if (frame.reference)
            local.emplace(8, Position(*frame.reference));
        auto& control = *from_fact(ContextFactGroup::LocalControl, ContextGroup::LocalControl, std::move(local));
        if (control.status == FactStatus::Available || control.status == FactStatus::Partial) {
            if (frame.local_character_identity && control.fields.contains(7) &&
                frame.local_character_identity->actor_address == frame.local_character &&
                !options.reader_instance_id.empty()) {
                const auto id = registry.Resolve(EntityKind::Character, *frame.local_character_identity);
                control.fields.emplace(9, Value(id));
                control.field_sources.emplace(9, control.field_sources.at(7));
                control.fields.emplace(11, Reference(options.reader_instance_id, out.epoch, id, EntityKind::Character,
                                                     *frame.local_character_identity));
                control.field_sources.emplace(11, control.field_sources.at(7));
            }
            if (frame.self_pawn_identity && control.fields.contains(5) &&
                frame.self_pawn_identity->actor_address == frame.self_pawn &&
                (frame.self_pawn_kind == EntityKind::Character || frame.self_pawn_kind == EntityKind::Vehicle) &&
                !options.reader_instance_id.empty()) {
                const auto id = registry.Resolve(frame.self_pawn_kind, *frame.self_pawn_identity);
                control.fields.emplace(10, Value(id));
                control.field_sources.emplace(10, control.field_sources.at(5));
                control.fields.emplace(12, Reference(options.reader_instance_id, out.epoch, id, frame.self_pawn_kind,
                                                     *frame.self_pawn_identity));
                control.field_sources.emplace(12, control.field_sources.at(5));
            }
        }
    }
    const auto origin = facts.find(ContextFactGroup::Origin);
    if (origin == facts.end())
        throw std::invalid_argument("Missing original coordinate origin fact");
    auto& coordinates = *terminal(ContextGroup::Coordinates, origin->second->status, origin->second->verified);
    const auto configuration = [&](unsigned field, Value value) {
        coordinates.fields.emplace(field, std::move(value));
        coordinates.field_sources.emplace(
            field, GroupState::FieldSource{frame.sequence, {frame.started, frame.started}, false, 3});
    };
    if (!options.coordinate_convention.empty())
        configuration(1, Value(options.coordinate_convention));
    if (options.verified_meters_per_unit)
        configuration(2, Value(*options.verified_meters_per_unit));
    if (!options.coordinate_rules_revision.empty())
        configuration(4, Value(options.coordinate_rules_revision));
    if (origin->second->status == FactStatus::Available || origin->second->status == FactStatus::Partial) {
        if (origin->second->present_fields != 1 || !frame.origin || origin->second->field_sources.size() != 1 ||
            origin->second->field_sources[0].field != 1)
            throw std::invalid_argument("Origin proof has no exact current source");
        const auto& source = origin->second->field_sources[0];
        coordinates.fields.emplace(3, Origin(*frame.origin));
        coordinates.field_sources.emplace(
            3, GroupState::FieldSource{source.capture_sequence, source.sample, source.reused,
                                       source.source == ContextSourceKind::Memory ? 1u : 2u});
    } else if (origin->second->present_fields)
        throw std::invalid_argument("Unavailable origin has values");
    if (!coordinates.fields.empty())
        coordinates.status = origin->second->present_fields ? FactStatus::Available : FactStatus::Partial;
    auto& map = *terminal(ContextGroup::Map, FactStatus::NotAttempted,
                          {world_context.verified_sample.finished, world_context.verified_sample.finished});
    if (frame.world_name_status != NameCaptureStatus::NotAttempted) {
        const SampleInterval sample{frame.world_name_observed, frame.world_name_finished};
        const bool current = frame.world_name_capture_sequence == frame.sequence && CurrentInterval(sample, frame);
        const bool directory = frame.directory_generation && frame.world_name_capture_sequence &&
                               frame.world_name_capture_sequence == frame.directory_generation &&
                               sample.started != Clock::time_point{} && sample.started <= sample.finished &&
                               sample.started >= frame.directory_sample.started && sample.finished <= frame.started;
        if (frame.name_world == frame.world && (current || directory)) {
            map.status = frame.world_name_status == NameCaptureStatus::Succeeded && frame.world_name.verified
                             ? FactStatus::Available
                             : FactStatus::Partial;
            // 名称在目录层实际观察，本批以已关闭的同 World 链证明关联；不能刷新其原采样时间。
            if (current)
                map.verified_sample = sample;
            map.fields.emplace(1, Value(frame.name_world));
            const auto name = NameFields(frame.world_name);
            if (!name.empty())
                map.fields.emplace(2, Value(name));
            map.fields.emplace(3, Value(frame.name_root_unverified));
            for (const auto& [field, value] : map.fields)
                map.field_sources.emplace(field, GroupState::FieldSource{frame.world_name_capture_sequence, sample,
                                                                         !current, field == 3 ? 2u : 1u});
        } else
            map.status = FactStatus::DependencyBlocked;
    }
    if (!frame.pipeline || !frame.pipeline->target_interval_ns)
        throw std::invalid_argument("No frozen pacing metadata");
    auto& pacing = *terminal(ContextGroup::Pacing, FactStatus::Available, {frame.started, frame.started});
    pacing.fields = {{1, Value(frame.pipeline->pacing_revision)},
                     {2, Value(double(frame.pipeline->target_fps))},
                     {3, Value(frame.pipeline->target_interval_ns)}};
    for (const auto& [field, value] : pacing.fields)
        pacing.field_sources.emplace(field,
                                     GroupState::FieldSource{frame.sequence, {frame.started, frame.started}, false, 3});
}
} // namespace
State ProjectCurrent(const Snapshot& frame, u64 epoch, u64 revision, InstanceRegistry& registry,
                     const ProjectionOptions& options) {
    if (!frame.current_facts || frame.current_facts->capture_sequence != frame.sequence)
        throw std::invalid_argument("Snapshot has no matching CurrentFacts");
    const auto& facts = *frame.current_facts;
    State out;
    out.epoch = epoch;
    out.revision = revision;
    out.capture_sequence = frame.sequence;
    out.capture = {frame.started, frame.finished};
    out.read_finished = frame.read_finished;
    out.batch.category = frame.category;
    out.batch.directory_generation = frame.directory_generation;
    out.batch.classification_generation = frame.classification_generation;
    out.batch.directory_sample = frame.directory_sample;
    out.batch.classification_sample = frame.classification_sample;
    out.batch.membership_generation = frame.membership_generation;
    out.batch.membership_sample = frame.membership_sample;
    for (const auto& actor : facts.actors)
        for (const auto& group : actor.groups) {
            if (group.status == FactStatus::NotImplemented)
                continue;
            ++out.batch.planned;
            switch (group.status) {
                case FactStatus::Available:
                case FactStatus::Partial:
                    ++out.batch.succeeded;
                    break;
                case FactStatus::ReadFailed:
                    ++out.batch.read_failed;
                    break;
                case FactStatus::DependencyBlocked:
                    ++out.batch.dependency_blocked;
                    break;
                default:
                    ++out.batch.not_executed;
                    break;
            }
        }
    if (frame.pipeline) {
        const auto& tasks = frame.pipeline->tasks;
        out.batch.planned = tasks.planned;
        out.batch.succeeded = tasks.succeeded;
        out.batch.read_failed = tasks.read_failed;
        out.batch.dependency_blocked = tasks.dependency_blocked;
        out.batch.not_executed = tasks.unexecuted;
    }
    out.contract = options.contract;
    if (frame.pipeline && frame.pipeline->epoch != epoch)
        throw std::invalid_argument("Projection epoch contradicts the sealed frame");
    if (frame.category == DataCategory::Local) {
        if (!options.contract.empty())
            throw std::invalid_argument("Local cannot negotiate an entity property contract");
        out.accepted = frame.complete && !frame.fatal && facts.frame_accepted && frame.local.has_value();
        if (out.accepted) {
            registry.Fence(epoch);
            ProjectContext(frame, out, options, {}, registry);
            ProjectLocalValues(frame, out);
            for (auto& [kind, group] : out.context)
                for (auto& [field, source] : group.field_sources)
                    source.source_category = source.reused ? DataCategory::None : DataCategory::Local;
        }
        std::string error;
        if (!ValidateState(out, error))
            throw std::invalid_argument(error);
        return out; // Local不持有Actor名单；引用只来自本批本地生命周期核验。
    }
    out.source = {facts.source_open_complete,
                  facts.source_close_complete,
                  facts.source_level,
                  u64(frame.scene_actor_slots.size()),
                  u64(facts.closing_source_slots.size()),
                  facts.opening_container,
                  facts.closing_container,
                  facts.opening_source,
                  facts.closing_source};
    auto staged = registry;
    staged.Fence(epoch);
    out.accepted = frame.complete && !frame.fatal && facts.frame_accepted && facts.source_open_complete &&
                   facts.source_close_complete;
    if (!out.accepted) {
        const auto scope = staged.Scope(frame.category);
        for (const auto& [key, id] : scope.Entries())
            out.removals[id] = RemovalReason::ReadFailure;
        std::string error;
        if (!ValidateState(out, error))
            throw std::invalid_argument(error);
        registry = std::move(staged);
        return out;
    }
    if (!frame.prepared_entities || facts.source_level != frame.level ||
        facts.opening_source.started == Clock::time_point{} ||
        facts.opening_source.finished < facts.opening_source.started ||
        facts.opening_source.finished > frame.finished || facts.closing_source.started == Clock::time_point{} ||
        facts.closing_source.finished < facts.closing_source.started || facts.closing_source.finished > frame.finished)
        throw std::invalid_argument("Accepted projection lacks current source boundaries");
    std::unordered_map<u64, Typed> typed;
    const auto insert = [&](u64 actor, Typed entry) {
        if (!actor || !typed.emplace(actor, entry).second)
            throw std::invalid_argument("Duplicate typed Actor address");
    };
    for (const auto& c : frame.prepared_entities->characters)
        insert(c.actor_id, {&c});
    for (const auto& v : frame.prepared_entities->vehicles) {
        if (!v.sample)
            throw std::invalid_argument("Vehicle sample missing");
        insert(v.sample->actor, {nullptr, &v});
    }
    for (const auto& p : frame.prepared_entities->pickups) {
        if (!p.sample)
            throw std::invalid_argument("Pickup sample missing");
        insert(p.sample->actor, {nullptr, nullptr, &p});
    }
    for (const auto& c : frame.prepared_entities->creatures) {
        if (!c.sample)
            throw std::invalid_argument("Creature sample missing");
        insert(c.sample->actor, {nullptr, nullptr, nullptr, &c});
    }
    for (const auto& c : frame.prepared_entities->containers) {
        if (!c.sample)
            throw std::invalid_argument("Container sample missing");
        insert(c.sample->actor, {nullptr, nullptr, nullptr, nullptr, &c});
    }
    std::unordered_map<u64, const ActorFact*> by_address;
    std::unordered_map<u64, u64> character_ids;
    for (const auto& fact : facts.actors) {
        if (!by_address.emplace(fact.actor_address, &fact).second)
            throw std::invalid_argument("Duplicate ActorFact address");
        if (fact.admission != AdmissionState::Admitted)
            continue;
        if (!fact.identity || !fact.lifecycle_source_verified || !fact.source_present_at_close ||
            fact.identity->actor_address != fact.actor_address || fact.verified_finished < frame.started ||
            fact.verified_finished > frame.finished)
            throw std::invalid_argument("Admitted ActorFact lacks current lifecycle/source proof");
        const auto value = typed.find(fact.actor_address);
        if (value == typed.end())
            throw std::invalid_argument("Admitted ActorFact has no matching typed data");
        const auto& t = value->second;
        if (!options.contract.contains(fact.kind))
            continue; // 此订阅没有启用该种类，不是实例退役。
        if ((fact.kind == EntityKind::Character) != bool(t.character) ||
            (fact.kind == EntityKind::Vehicle) != bool(t.vehicle) ||
            (fact.kind == EntityKind::Pickup) != bool(t.pickup) ||
            (fact.kind == EntityKind::Creature) != bool(t.creature) ||
            (fact.kind == EntityKind::Container) != bool(t.container))
            throw std::invalid_argument("ActorFact kind contradicts typed data");
        const auto id = staged.Resolve(fact.kind, *fact.identity);
        out.entities.emplace(id, EntityState{id, {fact.kind, *fact.identity}, {}});
        if (fact.kind == EntityKind::Character)
            character_ids.emplace(fact.actor_address, id);
    }
    const std::unordered_set<u64> closing(facts.closing_source_slots.begin(), facts.closing_source_slots.end());
    for (const auto& vehicle : frame.prepared_entities->vehicles)
        if (vehicle.sample && vehicle.sample->seats)
            for (const auto& seat : vehicle.sample->seats->seats)
                if (seat.riders)
                    for (const auto& rider : seat.riders->riders)
                        if (rider.observation.value_status && rider.pawn_address && rider.instance_identity &&
                            rider.instance_identity->actor_address == *rider.pawn_address)
                            character_ids[*rider.pawn_address] =
                                staged.Resolve(EntityKind::Character, *rider.instance_identity);
    std::vector<InstanceKey> retire;
    const auto scope = staged.Scope(frame.category);
    for (const auto& [key, id] : scope.Entries())
        if (!out.entities.contains(id)) {
            RemovalReason reason = RemovalReason::Unknown;
            if (!closing.contains(key.instance.actor_address))
                reason = RemovalReason::WithdrawnFromScope;
            if (const auto attempt = by_address.find(key.instance.actor_address); attempt != by_address.end()) {
                const auto& fact = *attempt->second;
                if (fact.lifecycle_source_verified && fact.identity &&
                    fact.identity->actor_address == key.instance.actor_address && *fact.identity != key.instance) {
                    reason = RemovalReason::InstanceReplaced;
                    retire.push_back(key);
                } else if (fact.identity && *fact.identity == key.instance && fact.lifecycle_source_verified) {
                    if (fact.admission == AdmissionState::Retired) {
                        reason = RemovalReason::InstanceRetired;
                        retire.push_back(key);
                    } else if (fact.admission == AdmissionState::IdentityChanged) {
                        reason = RemovalReason::InstanceReplaced;
                        retire.push_back(key);
                    } else if (fact.admission == AdmissionState::Unadmitted)
                        reason = RemovalReason::ReadFailure;
                } else if (fact.admission == AdmissionState::Unadmitted)
                    reason = RemovalReason::ReadFailure;
            }
            out.removals[id] = reason;
        }
    for (auto& [id, entity] : out.entities) {
        const auto& fact = *by_address.at(entity.key.instance.actor_address);
        const auto& t = typed.at(fact.actor_address);
        std::set<PropertyGroup> seen_groups;
        for (const auto& attempt : fact.groups) {
            if (!GroupAllowed(fact.kind, attempt.group) || !seen_groups.insert(attempt.group).second)
                throw std::invalid_argument("Invalid or duplicate current group");
            if (attempt.present_fields & ~AllowedFields(attempt.group))
                throw std::invalid_argument("Unknown current field mask");
            if (!options.contract.at(fact.kind).enabled.contains(attempt.group))
                continue;
            GroupState group;
            group.status = attempt.status;
            group.verified_capture_sequence = frame.sequence;
            group.verified_sample = CurrentInterval(attempt.sample, frame)
                                        ? attempt.sample
                                        : SampleInterval{fact.verified_finished, fact.verified_finished};
            if (attempt.status == FactStatus::Available || attempt.status == FactStatus::Partial) {
                if (!CurrentInterval(attempt.sample, frame))
                    throw std::invalid_argument("Available group has no real current sample");
                group.source_sample = attempt.sample;
                group.source_capture_sequence = frame.sequence;
                auto values =
                    Properties(frame, fact, t, attempt.group, character_ids, options.reader_instance_id, epoch);
                for (unsigned field = 1; field <= 64; ++field)
                    if (attempt.present_fields & (u64(1) << (field - 1))) {
                        const auto value = values.find(field);
                        if (value == values.end())
                            throw std::invalid_argument("Present field has no sampled value");
                        group.fields.emplace(field, std::move(value->second));
                    }
                group.nested_samples = Samples(t, attempt.group, group.fields);
            } else if (attempt.present_fields)
                throw std::invalid_argument("Unavailable group declares present fields");
            entity.groups.emplace(attempt.group, std::move(group));
            diagnostics::CompletedCpuUnit();
        }
        diagnostics::CompletedCpuUnit();
    }
    std::string error;
    ProjectContext(frame, out, options, typed, staged);
    for (auto& [kind, group] : out.context)
        for (auto& [field, source] : group.field_sources)
            source.source_category = source.reused ? DataCategory::None : frame.category;
    if (!ValidateState(out, error))
        throw std::invalid_argument(error);
    for (const auto& key : retire)
        staged.Retire(key.kind, key.instance);
    registry = std::move(staged);
    return out;
}
} // namespace monitor::synchronization
