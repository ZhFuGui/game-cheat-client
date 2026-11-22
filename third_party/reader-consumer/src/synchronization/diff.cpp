#include "synchronization/diff.hpp"
#include "synchronization/session.hpp"
#include "diagnostics/work_progress.hpp"
#include <set>
#include <stdexcept>

namespace monitor::synchronization {
namespace {
Update BuildValidatedUpdate(const State& baseline, const State& current, u64 generation, u64 fence) {
    std::string error;
    if (!baseline.accepted || baseline.epoch != current.epoch || !generation || !fence ||
        current.revision <= baseline.revision || baseline.contract != current.contract ||
        baseline.batch.category != current.batch.category)
        throw std::invalid_argument("Delta requires a fixed accepted baseline of the same epoch");
    if (current.accepted && !SameEpochContext(baseline, current.context, error))
        throw std::invalid_argument(error);
    Update out;
    out.epoch = current.epoch;
    out.baseline_generation = generation;
    out.fence = fence;
    out.base_revision = baseline.revision;
    out.revision = current.revision;
    out.capture_sequence = current.capture_sequence;
    out.accepted = current.accepted;
    out.capture = current.capture;
    out.read_finished = current.read_finished;
    out.source = current.source;
    out.batch = current.batch;
    out.context = current.context;
    for (const auto& [id, old] : baseline.entities)
        if (!current.entities.contains(id)) {
            const auto reason = current.removals.find(id);
            out.removals[id] = reason == current.removals.end() ? RemovalReason::Unknown : reason->second;
        }
    for (const auto& [id, entity] : current.entities) {
        auto& proof = out.proofs[id];
        proof.key = entity.key;
        for (const auto& [group, sample] : entity.groups)
            proof.groups[group] = {sample.status,
                                   FieldMask(sample.fields),
                                   sample.verified_sample,
                                   sample.verified_capture_sequence,
                                   sample.nested_samples,
                                   sample.source_sample,
                                   sample.source_capture_sequence};
        const auto old_entity = baseline.entities.find(id);
        if (old_entity == baseline.entities.end()) {
            out.additions.emplace(id, entity);
            continue;
        }
        if (old_entity->second.key != entity.key)
            throw std::invalid_argument("Stable ID changed its instance");
        for (const auto& [group, old] : old_entity->second.groups)
            if (!entity.groups.contains(group))
                out.patches.push_back({id, group, PatchOperation::Clear, {}, AllowedFields(group), {}});
        for (const auto& [group, sample] : entity.groups) {
            const auto old = old_entity->second.groups.find(group);
            if (old == old_entity->second.groups.end()) {
                out.patches.push_back({id, group, PatchOperation::Replace, sample.fields, 0, sample.source_sample,
                                       sample.source_capture_sequence});
                continue;
            }
            if (group == PropertyGroup::ContainerContents) {
                // 与原流程同义：值或存在性变化仍整组 Replace，不先复制最终丢弃的 changed。
                if (sample.fields != old->second.fields)
                    out.patches.push_back({id, group, PatchOperation::Replace, sample.fields, 0, sample.source_sample,
                                           sample.source_capture_sequence});
                diagnostics::CompletedCpuUnit();
                continue;
            }
            std::map<unsigned, Value> changed;
            u64 cleared = 0;
            for (const auto& [field, value] : sample.fields) {
                const auto before = old->second.fields.find(field);
                if (before == old->second.fields.end() || before->second != value)
                    changed.emplace(field, value);
            }
            for (const auto& [field, value] : old->second.fields)
                if (!sample.fields.contains(field))
                    cleared |= u64(1) << (field - 1);
            if (!changed.empty() || cleared) {
                out.patches.push_back({id, group, PatchOperation::Set, std::move(changed), cleared,
                                       sample.source_sample, sample.source_capture_sequence});
            }
            diagnostics::CompletedCpuUnit();
        }
        diagnostics::CompletedCpuUnit();
    }
    return out;
}
} // namespace
Update BuildUpdate(const State& baseline, const State& current, u64 generation, u64 fence) {
    std::string error;
    if (!ValidateState(baseline, error) || !ValidateState(current, error))
        throw std::invalid_argument(error);
    return BuildValidatedUpdate(baseline, current, generation, fence);
}
Update DebugSessionPolicy::BuildAcknowledgedUpdate(const State& baseline, const State& current, u64 generation,
                                                          u64 fence) {
    // baseline 仅来自 OfferBaseline 已完整核验的独占 const 副本，ACK 不会更改其内容。
    // 当前状态仍可能来自不同投影/契约，必须每次完整核验；公共 BuildUpdate 继续双验。
    std::string error;
    if (!ValidateState(current, error))
        throw std::invalid_argument(error);
    return BuildValidatedUpdate(baseline, current, generation, fence);
}
bool ApplyUpdate(const State& baseline, const Update& update, State& candidate, std::string& error) {
    try {
        error.clear();
        const auto fail = [&](const char* why) {
            error = why;
            return false;
        };
        if (!ValidateState(baseline, error) || !baseline.accepted)
            return fail("Invalid acknowledged baseline");
        if (update.epoch != baseline.epoch || !update.baseline_generation || !update.fence ||
            update.base_revision != baseline.revision || update.revision <= baseline.revision)
            return fail("Wrong epoch, baseline or revision");
        if (update.accepted && !SameEpochContext(baseline, update.context, error))
            return false;
        State next = baseline;
        next.revision = update.revision;
        next.capture_sequence = update.capture_sequence;
        next.accepted = update.accepted;
        next.capture = update.capture;
        next.read_finished = update.read_finished;
        next.source = update.source;
        next.batch = update.batch;
        next.context = update.context;
        next.removals = update.removals;
        for (const auto& [id, reason] : update.removals) {
            if (static_cast<unsigned>(reason) > static_cast<unsigned>(RemovalReason::InstanceReplaced))
                return fail("Unknown withdrawal reason");
            if (!baseline.entities.contains(id) || update.additions.contains(id))
                return fail("Invalid removal target");
            next.entities.erase(id);
        }
        for (const auto& [id, entity] : update.additions) {
            if (baseline.entities.contains(id) || id != entity.id || !id)
                return fail("Invalid added instance");
            next.entities.emplace(id, entity);
        }
        std::set<std::pair<u64, PropertyGroup>> targets;
        for (const auto& patch : update.patches) {
            if (!targets.emplace(patch.entity_id, patch.group).second)
                return fail("Duplicate property patch");
            auto entity = next.entities.find(patch.entity_id);
            if (entity == next.entities.end() || update.additions.contains(patch.entity_id) ||
                !GroupAllowed(entity->second.key.kind, patch.group))
                return fail("Invalid property target");
            const auto set = FieldMask(patch.values), allowed = AllowedFields(patch.group);
            if ((set | patch.clear_fields) & ~allowed || set & patch.clear_fields)
                return fail("Unknown or overlapping property masks");
            switch (patch.operation) {
                case PatchOperation::Clear:
                    if (set || patch.clear_fields != allowed)
                        return fail("CLEAR must remove the whole property group");
                    entity->second.groups[patch.group].fields.clear();
                    entity->second.groups[patch.group].source_sample = {};
                    entity->second.groups[patch.group].source_capture_sequence = 0;
                    break;
                case PatchOperation::Replace: {
                    if (patch.clear_fields)
                        return fail("REPLACE cannot contain a separate clear mask");
                    auto& group = entity->second.groups[patch.group];
                    group.fields = patch.values;
                    group.source_sample = patch.source_sample;
                    group.source_capture_sequence = patch.source_capture_sequence;
                    break;
                }
                case PatchOperation::Set: {
                    if (!set && !patch.clear_fields)
                        return fail("Empty SET patch");
                    auto group = entity->second.groups.find(patch.group);
                    if (group == entity->second.groups.end())
                        return fail("SET requires an existing group");
                    for (unsigned field = 1; field <= 64; ++field)
                        if (patch.clear_fields & (u64(1) << (field - 1)))
                            group->second.fields.erase(field);
                    for (const auto& [field, value] : patch.values)
                        group->second.fields.insert_or_assign(field, value);
                    group->second.source_sample = patch.source_sample;
                    group->second.source_capture_sequence = patch.source_capture_sequence;
                    break;
                }
                default:
                    return fail("Unknown patch operation");
            }
        }
        if (next.entities.size() != update.proofs.size())
            return fail("Current instance proof list is incomplete");
        for (auto& [id, entity] : next.entities) {
            const auto proof = update.proofs.find(id);
            if (proof == update.proofs.end() || proof->second.key != entity.key ||
                proof->second.groups.size() != entity.groups.size())
                return fail("Missing or contradictory current proof");
            for (auto& [group, sample] : entity.groups) {
                const auto checked = proof->second.groups.find(group);
                if (checked == proof->second.groups.end() || checked->second.present_fields != FieldMask(sample.fields))
                    return fail("Current field presence does not match reconstructed values");
                sample.status = checked->second.status;
                sample.verified_sample = checked->second.verified_sample;
                sample.verified_capture_sequence = checked->second.verified_capture_sequence;
                sample.nested_samples = checked->second.nested_samples;
                sample.source_sample = checked->second.source_sample;
                sample.source_capture_sequence = checked->second.source_capture_sequence;
            }
        }
        if (!ValidateState(next, error))
            return false;
        candidate = std::move(next);
        return true;
    } catch (const std::invalid_argument& problem) {
        error = problem.what();
        return false;
    }
}
} // namespace monitor::synchronization
