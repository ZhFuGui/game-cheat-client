#include "synchronization/consumer/diff.hpp"
#include "diagnostics/work_progress.hpp"
#include <stdexcept>

namespace monitor::synchronization::consumer {
namespace {
GroupRefresh Refresh(const GroupSample& sample) {
    return {sample.status, sample.value.has_value(), sample.source_capture_sequence, sample.source, sample.sources};
}
bool Same(const GroupRefresh& refresh, const GroupSample& sample) {
    return refresh.status == sample.status && refresh.has_value == sample.value.has_value() &&
           refresh.source_capture_sequence == sample.source_capture_sequence &&
           refresh.source.started == sample.source.started && refresh.source.finished == sample.source.finished &&
           refresh.sources == sample.sources;
}
void RefreshSample(GroupSample& sample, const GroupRefresh& refresh) {
    sample.status = refresh.status;
    sample.source_capture_sequence = refresh.source_capture_sequence;
    sample.source = refresh.source;
    sample.sources = refresh.sources;
    if (!refresh.has_value)
        sample.value.reset();
}
Update BuildValidated(const State& baseline, const State& current, u64 generation, u64 fence) {
    if (!baseline.accepted || !current.accepted || baseline.epoch != current.epoch || !generation || !fence ||
        current.revision <= baseline.revision || current.capture_sequence <= baseline.capture_sequence ||
        baseline.category != current.category || baseline.features != current.features)
        throw std::invalid_argument("Consumer delta requires the same accepted fixed baseline contract");
    std::string error;
    if (!SameEpochContext(baseline, current.context, error))
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
    out.category = current.category;
    out.features = current.features;
    // Local终态与业务值在下方独立差分；这里不复制后马上销毁整张Local组表。
    const auto& context = current.context;
    out.context.world_token = context.world_token;
    out.context.level_token = context.level_token;
    out.context.game_state_token = context.game_state_token;
    out.context.native_world_velocity_supported = context.native_world_velocity_supported;
    out.context.map_name = context.map_name;
    out.context.coordinate_convention = context.coordinate_convention;
    out.context.verified_meters_per_unit = context.verified_meters_per_unit;
    out.context.origin = context.origin;
    out.context.pacing = context.pacing;
    out.context.sources = context.sources;
    out.replace_context_values = !SameContextValues(baseline.context, current.context);
    for (const auto& [id, old] : baseline.entities)
        if (!current.entities.contains(id)) {
            const auto reason = current.removals.find(id);
            out.removals.emplace(id, reason == current.removals.end() ? RemovalReason::Unknown : reason->second);
        }
    for (const auto& [id, entity] : current.entities) {
        auto& refresh = out.current[id];
        refresh.kind = entity.kind;
        for (const auto& [group, sample] : entity.groups)
            refresh.groups.emplace(group, Refresh(sample));
        const auto previous = baseline.entities.find(id);
        if (previous == baseline.entities.end())
            out.additions.emplace(id, entity);
        else {
            if (previous->second.kind != entity.kind)
                throw std::invalid_argument("Consumer stable ID changed entity kind");
            for (const auto& [group, sample] : entity.groups) {
                const auto old = previous->second.groups.find(group);
                if (old == previous->second.groups.end() || old->second.value != sample.value)
                    out.changes.emplace(std::make_pair(id, group), sample.value);
            }
        }
        diagnostics::CompletedCpuUnit();
    }
    for (const auto& [group, sample] : current.context.local_groups) {
        out.local_current.emplace(group, Refresh(sample));
        const auto old = baseline.context.local_groups.find(group);
        if (old == baseline.context.local_groups.end() || old->second.value != sample.value)
            out.local_changes.emplace(group, sample.value);
    }
    return out;
}
} // namespace
bool SameContextValues(const Context& a, const Context& b) noexcept {
    return a.world_token == b.world_token && a.level_token == b.level_token &&
           a.game_state_token == b.game_state_token && a.map_name == b.map_name &&
           a.coordinate_convention == b.coordinate_convention &&
           a.verified_meters_per_unit == b.verified_meters_per_unit && a.origin == b.origin && a.pacing == b.pacing &&
           a.native_world_velocity_supported == b.native_world_velocity_supported;
}
Update BuildUpdate(const State& baseline, const State& current, u64 generation, u64 fence) {
    std::string error;
    if (!ValidateState(baseline, error) || !ValidateState(current, error))
        throw std::invalid_argument(error);
    return BuildValidated(baseline, current, generation, fence);
}
Update BuildAcknowledgedUpdate(const State& baseline, const State& current, u64 generation, u64 fence) {
    std::string error;
    if (!ValidateState(current, error))
        throw std::invalid_argument(error);
    return BuildValidated(baseline, current, generation, fence);
}
bool ApplyUpdate(const State& baseline, const Update& update, State& candidate, std::string& error) {
    try {
        error.clear();
        const auto fail = [&](const char* message) {
            error = message;
            return false;
        };
        if (!ValidateState(baseline, error) || !baseline.accepted)
            return fail("Invalid Consumer acknowledged baseline");
        if (!update.accepted || update.epoch != baseline.epoch || !update.baseline_generation || !update.fence ||
            update.base_revision != baseline.revision || update.revision <= baseline.revision ||
            update.capture_sequence <= baseline.capture_sequence || update.category != baseline.category ||
            update.features != baseline.features || !update.context.local_groups.empty())
            return fail("Consumer delta epoch, revision or contract mismatch");
        State next = baseline;
        next.epoch = update.epoch;
        next.revision = update.revision;
        next.capture_sequence = update.capture_sequence;
        next.accepted = update.accepted;
        next.capture = update.capture;
        next.read_finished = update.read_finished;
        if (update.replace_context_values) {
            next.context = update.context;
        } else {
            next.context.local_groups.clear();
            next.context.sources = update.context.sources;
        }
        next.context.local_groups.clear();
        next.removals = update.removals;
        for (const auto& [id, reason] : update.removals) {
            if (!baseline.entities.contains(id) || update.additions.contains(id) || update.current.contains(id))
                return fail("Invalid Consumer withdrawal");
            next.entities.erase(id);
        }
        for (const auto& [id, entity] : update.additions) {
            if (!id || entity.id != id || baseline.entities.contains(id) || update.removals.contains(id) ||
                !update.current.contains(id) || !next.entities.emplace(id, entity).second)
                return fail("Invalid Consumer addition");
        }
        if (next.entities.size() != update.current.size())
            return fail("Consumer current entity set is incomplete");
        for (auto& [id, entity] : next.entities) {
            const auto current = update.current.find(id);
            if (current == update.current.end() || current->second.kind != entity.kind)
                return fail("Consumer current entity identity mismatch");
            if (update.additions.contains(id) && entity.groups.size() != current->second.groups.size())
                return fail("Consumer addition contains groups absent from current set");
            std::erase_if(entity.groups,
                          [&](const auto& entry) { return !current->second.groups.contains(entry.first); });
            for (const auto& [group, refresh] : current->second.groups) {
                const auto addition = update.additions.find(id);
                if (addition != update.additions.end()) {
                    const auto sample = entity.groups.find(group);
                    if (sample == entity.groups.end() || !Same(refresh, sample->second))
                        return fail("Consumer addition metadata differs from current set");
                } else {
                    auto& sample = entity.groups[group];
                    const auto patch = update.changes.find({id, group});
                    if (patch != update.changes.end())
                        sample.value = patch->second;
                    RefreshSample(sample, refresh);
                    if (refresh.has_value != sample.value.has_value())
                        return fail("Consumer delta omitted required group value");
                }
            }
            diagnostics::CompletedCpuUnit();
        }
        for (const auto& [key, value] : update.changes) {
            const auto current = update.current.find(key.first);
            if (update.additions.contains(key.first) || current == update.current.end() ||
                !current->second.groups.contains(key.second) ||
                current->second.groups.at(key.second).has_value != value.has_value())
                return fail("Consumer change does not belong to current retained entity");
        }
        for (const auto& [group, refresh] : update.local_current) {
            GroupSample sample;
            if (const auto old = baseline.context.local_groups.find(group); old != baseline.context.local_groups.end())
                sample = old->second;
            if (const auto patch = update.local_changes.find(group); patch != update.local_changes.end())
                sample.value = patch->second;
            RefreshSample(sample, refresh);
            if (sample.value.has_value() != refresh.has_value)
                return fail("Consumer delta omitted required Local value");
            next.context.local_groups.emplace(group, std::move(sample));
        }
        for (const auto& [group, value] : update.local_changes)
            if (!update.local_current.contains(group) || update.local_current.at(group).has_value != value.has_value())
                return fail("Consumer Local change does not belong to current group");
        if (!SameEpochContext(baseline, next.context, error) || !ValidateState(next, error))
            return false;
        candidate = std::move(next);
        return true;
    } catch (const std::exception& problem) {
        error = problem.what();
        return false;
    }
}
} // namespace monitor::synchronization::consumer
