#include "synchronization/projection_authority.hpp"
#include <algorithm>
#include <set>
#include <limits>
namespace monitor::synchronization::authority {
namespace {
bool ValidWindow(const SampleInterval& value) noexcept {
    return value.started != Clock::time_point{} && value.finished >= value.started;
}
} // namespace
struct Projector::LockMark {
    LockMark() {
#if defined(MONITOR_PROJECTION_AUTHORITY_TESTING)
        registry_lock_held = true;
#endif
    }
    ~LockMark() {
#if defined(MONITOR_PROJECTION_AUTHORITY_TESTING)
        registry_lock_held = false;
#endif
    }
};
Projector::Projector(u64 next_id) {
    auto initial = std::make_shared<Authority>();
    initial->ids = sync::InstanceRegistry(next_id);
    authority_ = std::move(initial);
}
View Projector::Inspect() const {
    std::lock_guard guard(mutex_);
    LockMark mark;
    return {authority_, permitted_epoch_, fence_};
}
// 显式会话失效不同于失败投影；它保留编号高水位，撤销全部旧epoch资格。
void Projector::AdvanceEpoch(u64 epoch) {
    for (;;) {
        const auto base = Inspect();
        if (!epoch || epoch <= base.permitted_epoch)
            throw std::invalid_argument("Epoch must advance");
        auto changed = std::make_shared<Authority>(*base.authority);
        changed->ids.Fence(epoch);
        changed->evidence.clear();
        changed->tokens = std::make_shared<Authority::TokenMap>();
        ++changed->version;
        std::shared_ptr<const Authority> prepared = std::move(changed);
        std::lock_guard guard(mutex_);
        LockMark mark;
        if (authority_ != base.authority || fence_ != base.fence)
            continue;
        authority_.swap(prepared);
        permitted_epoch_ = epoch;
        ++fence_;
        return;
    }
}
void Projector::ObserveEpoch(u64 epoch) {
    // Output拥有者持自身短锁调用；只推进资格栅栏，不在此复制/释放大型表。
    std::lock_guard guard(mutex_);
    LockMark mark;
    if (!epoch || epoch < permitted_epoch_)
        throw StaleEvidence("Epoch observation went backwards");
    if (epoch == permitted_epoch_)
        return;
    permitted_epoch_ = epoch;
    ++fence_;
}
Plan Projector::Prepare(const Snapshot& frame, u64 epoch, u64 revision, const sync::ProjectionOptions& options) {
    const auto base = Inspect();
    if ((base.permitted_epoch && base.permitted_epoch != epoch) ||
        (!base.permitted_epoch && base.authority->ids.Epoch() && base.authority->ids.Epoch() != epoch))
        throw StaleEvidence("Different epoch requires explicit AdvanceEpoch before preparation");
    // 历史删除范围仅属于本类别；全局注册表仍负责跨流引用的统一ID裁决。
    // Scope仅复制固定数量的共享表指针，不访问其它类别的物资注册项。
    auto previous = base.authority->ids.Scope(frame.category);
    previous.Fence(epoch);
    auto symbols = previous;
    symbols.Fence(epoch);
    Plan plan;
    plan.epoch = epoch;
    plan.category = frame.category;
    plan.fence = base.fence;
    plan.state = std::make_shared<sync::State>(sync::ProjectCurrent(frame, epoch, revision, symbols, options));

#if defined(MONITOR_PROJECTION_AUTHORITY_TESTING)
    plan.projection_runs = 1;
#endif

    plan.symbols.reserve(previous.Entries().size() + symbols.Entries().size());
    plan.symbols_by_key.reserve(symbols.Entries().size());
    if (previous.Epoch() == epoch)
        for (const auto& [key, id] : previous.Entries())
            plan.symbols.emplace(id, key);
    for (const auto& [key, id] : symbols.Entries())
        plan.symbols.emplace(id, key);
    for (const auto& [key, id] : symbols.Entries())
        plan.symbols_by_key.emplace(key, id);
    plan.original_removals = plan.state->removals;
    plan.entity_nodes.reserve(plan.state->entities.size());
    std::unordered_map<sync::InstanceKey, SampleInterval, InstanceKeyHash> current_identity;
    std::unordered_map<u64, SampleInterval> lifecycle_close;
    current_identity.reserve(frame.current_facts->actors.size());
    lifecycle_close.reserve(frame.current_facts->actors.size());
    for (const auto& fact : frame.current_facts->actors) {

#if defined(MONITOR_PROJECTION_AUTHORITY_TESTING)
        ++plan.fact_index_entries;
#endif

        if (!fact.lifecycle_source_verified)
            continue;
        if ((fact.admission == AdmissionState::Retired || fact.admission == AdmissionState::IdentityChanged) &&
            ValidWindow(fact.lifecycle_observed))
            lifecycle_close.emplace(fact.actor_address, fact.lifecycle_observed);
        if (fact.admission == AdmissionState::Admitted && fact.identity && ValidWindow(fact.lifecycle_verified)) {
            current_identity.emplace(sync::InstanceKey{fact.kind, *fact.identity}, fact.lifecycle_verified);
            lifecycle_close.emplace(fact.actor_address, fact.lifecycle_verified);
        }
    }
    for (const auto& [id, entity] : plan.state->entities) {
        const auto proof = current_identity.find(entity.key);
        if (proof == current_identity.end())
            throw std::invalid_argument("Entity has no matching current identity close proof");
        Need(plan, id, proof->second, true);
    }
    auto control = plan.state->context.find(sync::ContextGroup::LocalControl);
    if (control != plan.state->context.end()) {
        auto& fields = control->second.fields;
        for (const auto field : {9u, 10u})
            if (fields.contains(field))
                // 引用仍使用涵盖GU关闭的保守区间。跨身份窗口重叠必须拒绝，
                // 不能把较晚CPU/静态证明尾部当作精确身份观察或只按起点排序。
                Patch(plan, fields.at(field), control->second.verified_sample);
        for (const auto field : {11u, 12u})
            if (fields.contains(field))
                Patch(plan, Object(fields.at(field)).at(3), control->second.verified_sample);
    }
    // 只枚举已知的乘员引用路径；不递归访问 GroundGroup/成员/物品属性树。
    for (auto& [id, entity] : plan.state->entities) {
        (void)id;
        auto group = entity.groups.find(PropertyGroup::VehicleSeats);
        if (group == entity.groups.end() || !group->second.fields.contains(1))
            continue;
        auto& seats = List(Object(group->second.fields.at(1)).at(1));
        for (unsigned seat_index = 0; seat_index < seats.size(); ++seat_index) {
            auto& seat_value = seats[seat_index];
            auto& seat = Object(seat_value);
            if (!seat.contains(4))
                continue;
            auto& riders = List(Object(seat.at(4)).at(4));
            for (unsigned rider_index = 0; rider_index < riders.size(); ++rider_index) {
                auto& rider_value = riders[rider_index];
                auto& rider = Object(rider_value);
                if (!rider.contains(4) && !rider.contains(5))
                    continue;
                const std::vector<unsigned> path{1, 1, 0x80000000u | seat_index, 4, 4, 0x80000000u | rider_index};
                const auto proof = group->second.nested_samples.find(path);
                if (proof == group->second.nested_samples.end() || proof->second.capture_sequence != frame.sequence ||
                    !proof->second.value_status || *proof->second.value_status != OwnedReadStatus::Available ||
                    proof->second.attempt.finished == Clock::time_point{})
                    throw std::invalid_argument("Rider reference lacks current nested identity evidence");
                if (rider.contains(4))
                    Patch(plan, rider.at(4), proof->second.attempt);
                if (rider.contains(5))
                    Patch(plan, Object(rider.at(5)).at(3), proof->second.attempt);
            }
        }
    }
    for (const auto& [symbol, reason] : plan.original_removals) {
        if (reason != sync::RemovalReason::InstanceReplaced && reason != sync::RemovalReason::InstanceRetired)
            continue;
        const auto key = plan.symbols.at(symbol);
        const auto stamp = lifecycle_close.find(key.instance.actor_address);
        const auto observed = stamp == lifecycle_close.end() ? SampleInterval{} : stamp->second;
        if (!ValidWindow(observed))
            continue; // 未准入/短读不是已证明的生命周期变化；不构造虚假的负证据。
        plan.retires.push_back({key, symbol, observed});
    }
    return plan;
}
ConsumerPlan Projector::PrepareConsumer(const Snapshot& frame, u64 epoch, u64 revision,
                                        const sync::ProjectionOptions& options) {
    const auto base = Inspect();
    if ((base.permitted_epoch && base.permitted_epoch != epoch) ||
        (!base.permitted_epoch && base.authority->ids.Epoch() && base.authority->ids.Epoch() != epoch))
        throw StaleEvidence("Different epoch requires explicit observation before Consumer projection");
    auto previous = base.authority->ids.Scope(frame.category);
    previous.Fence(epoch);
    auto symbols = previous;
    auto projected = consumer::ProjectCurrent(frame, epoch, revision, symbols, options);
    ConsumerPlan plan;
    plan.epoch = epoch;
    plan.category = frame.category;
    plan.fence = base.fence;
    plan.state = std::make_shared<consumer::State>(std::move(projected.state));
    plan.entity_keys = std::move(projected.entity_keys);
    plan.tokens = std::move(projected.tokens);
    for (const auto& [key, id] : previous.Entries())
        plan.symbols.emplace(id, key);
    for (const auto& [key, id] : symbols.Entries()) {
        plan.symbols.emplace(id, key);
        plan.symbols_by_key.emplace(key, id);
    }
    for (const auto& [symbol, use] : projected.identities) {
        const auto found = plan.symbols.find(symbol);
        if (found == plan.symbols.end() || found->second != use.key)
            throw std::invalid_argument("Consumer native identity is not in its symbolic transaction");
        Need(plan, symbol, use.interval, use.direct);
    }
    plan.original_removals = plan.state->removals;
    plan.entity_nodes.reserve(plan.state->entities.size());
    const auto patch = [&](u64& value) {
        if (!plan.symbols.contains(value) || !projected.identities.contains(value))
            throw std::invalid_argument("Consumer reference has no current native identity use");
        plan.patches.push_back({&value, value});
    };
    const auto token = [&](std::optional<u64>& value) {
        if (!value)
            return;
        if (!plan.tokens.contains(*value))
            throw std::invalid_argument("Consumer context token has no native key");
        plan.token_patches.push_back({&*value, *value});
    };
    const auto reference = [&](std::optional<consumer::Reference>& value) {
        if (value)
            patch(value->id);
    };
    const auto group = [&](consumer::GroupSample& sample) {
        if (!sample.value)
            return;
        std::visit(
            [&](auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, consumer::LocalControl>) {
                    token(value.controller_token);
                    reference(value.character);
                    reference(value.controlled);
                } else if constexpr (std::is_same_v<T, consumer::Camera>) {
                    token(value.controller_token);
                    token(value.source_token);
                } else if constexpr (std::is_same_v<T, consumer::CameraTarget>)
                    reference(value.target);
                else if constexpr (std::is_same_v<T, consumer::ControllerRotation>)
                    token(value.controller_token);
                else if constexpr (std::is_same_v<T, consumer::Weapon>) {
                    reference(value.owner);
                    token(value.weapon_token);
                } else if constexpr (std::is_same_v<T, consumer::Spectators>)
                    reference(value.owner);
                else if constexpr (std::is_same_v<T, consumer::AimBasis>) {
                    reference(value.owner);
                    token(value.native_context_token);
                    token(value.weapon_token);
                    token(value.controller_token);
                } else if constexpr (std::is_same_v<T, consumer::Skeleton>) {
                    reference(value.owner);
                    token(value.layout_token);
                    token(value.pose_token);
                } else if constexpr (std::is_same_v<T, consumer::Movement>) {
                    if (value.world)
                        reference(value.world->owner);
                } else if constexpr (std::is_same_v<T, consumer::Seats>) {
                    for (auto& seat : value.seats)
                        for (auto& rider : seat.riders)
                            patch(rider.id);
                } else if constexpr (std::is_same_v<T, consumer::GroundContents>) {
                    for (auto& member : value.members)
                        patch(member.id);
                }
            },
            *sample.value);
    };
    for (auto& [id, entity] : plan.state->entities)
        for (auto& [kind, value] : entity.groups)
            group(value);
    for (auto& [kind, value] : plan.state->context.local_groups)
        group(value);
    std::map<u64, SampleInterval> lifecycle_close;
    for (const auto& fact : frame.current_facts->actors)
        if (fact.lifecycle_source_verified) {
            if (fact.admission == AdmissionState::Admitted && ValidWindow(fact.lifecycle_verified))
                lifecycle_close.emplace(fact.actor_address, fact.lifecycle_verified);
            else if ((fact.admission == AdmissionState::Retired || fact.admission == AdmissionState::IdentityChanged) &&
                     ValidWindow(fact.lifecycle_observed))
                lifecycle_close.emplace(fact.actor_address, fact.lifecycle_observed);
        }
    for (const auto& [symbol, reason] : plan.original_removals) {
        if (reason != sync::RemovalReason::InstanceReplaced && reason != sync::RemovalReason::InstanceRetired)
            continue;
        const auto key = plan.symbols.at(symbol);
        const auto stamp = lifecycle_close.find(key.instance.actor_address);
        if (stamp != lifecycle_close.end())
            plan.retires.push_back({key, symbol, stamp->second});
    }
    return plan;
}
std::shared_ptr<const consumer::State> Projector::CommitConsumer(ConsumerPlan& plan, const Hook& hook,
                                                                 const Gate& gate) {
    if (!plan.state)
        throw std::invalid_argument("Consumer plan was already committed");
    CommitIdentity(
        plan, plan.state->entities.empty(),
        [&](const auto& ids, const auto& tokens, auto removals) {
            RenameConsumer(plan, ids, tokens, std::move(removals));
        },
        hook, gate);
    return std::shared_ptr<const consumer::State>(std::move(plan.state));
}
void Projector::RenameConsumer(ConsumerPlan& plan, const std::map<u64, u64>& binding, const std::map<u64, u64>& tokens,
                               std::map<u64, sync::RemovalReason> removals) {
    std::map<u64, sync::InstanceKey> entity_keys;
    for (const auto& [id, entity] : plan.state->entities) {
        const auto& key = plan.entity_keys.at(id);
        const auto actual = binding.at(plan.symbols_by_key.at(key));
        if (!actual || !entity_keys.emplace(actual, key).second)
            throw std::invalid_argument("Consumer entity ID binding collision");
    }
    for (const auto& patch : plan.patches)
        if (!binding.contains(patch.symbolic))
            throw std::invalid_argument("Missing Consumer reference binding");
    for (const auto& patch : plan.token_patches)
        if (!tokens.contains(patch.symbolic))
            throw std::invalid_argument("Missing Consumer context token binding");
    plan.entity_nodes.clear();
    while (!plan.state->entities.empty())
        plan.entity_nodes.push_back(plan.state->entities.extract(plan.state->entities.begin()));
    for (auto& node : plan.entity_nodes) {
        const auto actual = binding.at(plan.symbols_by_key.at(plan.entity_keys.at(node.key())));
        node.key() = node.mapped().id = actual;
    }
    for (auto& node : plan.entity_nodes)
        plan.state->entities.insert(std::move(node));
    plan.entity_keys.swap(entity_keys);
    for (const auto& patch : plan.patches)
        *patch.value = binding.at(patch.symbolic);
    for (const auto& patch : plan.token_patches)
        *patch.value = tokens.at(patch.symbolic);
    plan.state->removals.swap(removals);
}
std::shared_ptr<const sync::State> Projector::Commit(Plan& plan, const Hook& hook, const Gate& gate) {
    if (!plan.state)
        throw std::invalid_argument("Plan was already committed");
    CommitIdentity(
        plan, plan.state->entities.empty(),
        [&](const auto& ids, const auto&, auto removals) { Rename(plan, ids, std::move(removals)); }, hook, gate);
    return std::shared_ptr<const sync::State>(std::move(plan.state));
}
void Projector::CommitIdentity(IdentityPlan& plan, bool entities_empty, const RenameOperation& rename, const Hook& hook,
                               const Gate& gate) {
    for (;;) {
        const auto base = Inspect();
        if (base.fence != plan.fence || (base.permitted_epoch && base.permitted_epoch != plan.epoch))
            throw StaleEvidence("Epoch revoked during projection");
        ++plan.binding_attempts;
        std::map<u64, u64> token_binding;
        bool existing_tokens = base.authority->ids.Epoch() == plan.epoch;
        if (existing_tokens)
            for (const auto& [symbol, key] : plan.tokens) {
                const auto found = base.authority->tokens->find(key);
                if (found == base.authority->tokens->end()) {
                    existing_tokens = false;
                    break;
                }
                token_binding.emplace(symbol, found->second);
            }
        // 已登记且存活的Local引用只读绑定ID，不推进顶层Actor生命周期证据。
        // 仍经过同一个提交栅栏，保证并发退役/epoch变化不能使旧引用复活。
        if (plan.category == DataCategory::Local && base.authority->ids.Epoch() == plan.epoch && plan.retires.empty() &&
            plan.original_removals.empty() && entities_empty && existing_tokens) {
            std::map<u64, u64> references;
            bool existing = true;
            for (const auto& [key, proof] : plan.positive) {
                const auto id = base.authority->ids.Find(key.kind, key.instance);
                const auto evidence = base.authority->evidence.Find(key.instance.actor_address);
                if (proof.direct || !id || !evidence || evidence->key != key || evidence->retired) {
                    existing = false;
                    break;
                }
                references.emplace(plan.symbols_by_key.at(key), *id);
            }
            if (existing) {
                rename(references, token_binding, {});
                if (hook)
                    hook("before_commit", plan.binding_attempts);
                const std::function<CommitResult()> finish = [&] {
                    std::lock_guard guard(mutex_);
                    LockMark mark;
                    if (fence_ != plan.fence || (permitted_epoch_ && permitted_epoch_ != plan.epoch))
                        throw StaleEvidence("Epoch changed before Local reference commit");
                    if (authority_ != base.authority)
                        return CommitResult::Retry;
                    plan.committed_next_id = authority_->ids.NextId();
                    return CommitResult::Committed;
                };
                const auto result = gate ? gate(finish) : finish();
                if (result == CommitResult::Rejected)
                    throw StaleEvidence("Local lost publication/registry commit eligibility");
                if (result == CommitResult::Retry)
                    continue;
                return;
            }
        }
        auto changed = std::make_shared<Authority>(*base.authority);
        if (changed->ids.Epoch() != plan.epoch) {
            changed->ids.Fence(plan.epoch);
            changed->evidence.clear();
            changed->tokens = std::make_shared<Authority::TokenMap>();
        }
        token_binding.clear();
        std::shared_ptr<Authority::TokenMap> tokens;
        for (const auto& [symbol, key] : plan.tokens) {
            const auto found = changed->tokens->find(key);
            if (found != changed->tokens->end()) {
                token_binding.emplace(symbol, found->second);
                continue;
            }
            if (!tokens)
                tokens = std::make_shared<Authority::TokenMap>(*changed->tokens);
            if (const auto duplicate = tokens->find(key); duplicate != tokens->end()) {
                token_binding.emplace(symbol, duplicate->second);
                continue;
            }
            if (!changed->next_token || changed->next_token == std::numeric_limits<u64>::max())
                throw std::overflow_error("Opaque context token space exhausted");
            const auto value = changed->next_token++;
            tokens->emplace(key, value);
            token_binding.emplace(symbol, value);
        }
        if (tokens)
            changed->tokens = std::move(tokens);
        std::map<u64, u64> binding;
        std::map<u64, sync::RemovalReason> removals;
        std::vector<sync::InstanceKey> replace;
        for (const auto& [key, proof] : plan.positive) {

#if defined(MONITOR_PROJECTION_AUTHORITY_TESTING)
            ++plan.positive_index_lookups;
#endif

            const auto old = changed->evidence.Find(key.instance.actor_address);
            if (old) {
                if ((old->key != key || old->retired) && proof.interval.started <= old->observed_until)
                    throw StaleEvidence("Older or overlapping identity interval cannot revive or replace lifecycle");
            }
            for (const auto& [other, id] : changed->ids.AtAddress(key.instance.actor_address))
                if (other != key) {
                    replace.push_back(other);
                    removals[id] = sync::RemovalReason::InstanceReplaced;
                }
        }
        for (const auto& key : replace)
            changed->ids.Retire(key.kind, key.instance);
        for (const auto& [key, proof] : plan.positive) {
            const auto prior = changed->evidence.Find(key.instance.actor_address);
            const bool already_alive = prior && prior->key == key && !prior->retired;
            const auto id = changed->ids.Resolve(key.kind, key.instance);
            binding.emplace(plan.symbols_by_key.at(key), id);
            auto evidence = prior ? *prior : Evidence{};
            // 已登记且存活的跨流引用只绑定ID，不充当新的顶层生命周期观察。
            // 真正GU成功观察保留所有重叠区间的保守上界，避免遗忘不确定窗口。
            if (!already_alive)
                evidence = {key, proof.interval.started, proof.interval.finished, false};
            else if (proof.direct) {
                evidence.observed = std::max(evidence.observed, proof.interval.started);
                evidence.observed_until = std::max(evidence.observed_until, proof.interval.finished);
            }
            if (!already_alive || proof.direct)
                changed->evidence.Set(key.instance.actor_address, std::move(evidence));
        }
        for (const auto& proposal : plan.retires) {
            const auto id = changed->ids.Find(proposal.key.kind, proposal.key.instance);
            const auto observed = changed->evidence.Find(proposal.key.instance.actor_address);
            if (!id || *id != proposal.expected_id ||
                (observed && proposal.observed.started <= observed->observed_until))
                continue;
            if (plan.positive.contains(proposal.key))
                throw std::invalid_argument("Same batch both admits and retires one instance");
            changed->ids.Retire(proposal.key.kind, proposal.key.instance);
            removals[*id] = plan.original_removals.at(proposal.expected_id);
            if (!observed || observed->key == proposal.key)
                changed->evidence.Set(proposal.key.instance.actor_address,
                                      {proposal.key, proposal.observed.started, proposal.observed.finished, true});
        }
        for (const auto& [symbol, reason] : plan.original_removals) {
            // 旧/重叠负证据不仅不能更新注册表，也不能向接收端发送错误的生命周期墓碑。
            if (reason == sync::RemovalReason::InstanceReplaced || reason == sync::RemovalReason::InstanceRetired)
                continue;
            const auto found = binding.find(symbol);
            // 未重新Resolve的历史ID已由曾经成功的事务提交，永不复用。
            removals[found == binding.end() ? symbol : found->second] = reason;
        }
        rename(binding, token_binding, std::move(removals));
        ++changed->version;
        if (hook)
            hook("before_commit", plan.binding_attempts); // 异常/竞争夹具；在全局锁外。
        std::shared_ptr<const Authority> prepared = std::move(changed);
        const std::function<CommitResult()> finish = [&] {
            std::lock_guard guard(mutex_);
            LockMark mark;
            if (fence_ != plan.fence || (permitted_epoch_ && permitted_epoch_ != plan.epoch))
                throw StaleEvidence("Epoch changed before atomic commit");
            if (authority_ != base.authority)
                return CommitResult::Retry;
            authority_.swap(prepared);
            if (!permitted_epoch_)
                permitted_epoch_ = plan.epoch;
            plan.committed_next_id = authority_->ids.NextId();
            return CommitResult::Committed;
        };
        const auto committed = gate ? gate(finish) : finish();
        if (committed == CommitResult::Rejected)
            throw StaleEvidence("Output ticket lost publication/registry commit eligibility");
        if (committed == CommitResult::Retry)
            continue;
        return;
    }
}
void Projector::Need(IdentityPlan& plan, u64 symbol, SampleInterval stamp, bool direct) {
    const auto key = plan.symbols.at(symbol);
    if (!ValidWindow(stamp))
        throw std::invalid_argument("ID source has no interpretable current identity interval");
    if (!plan.symbols_by_key.contains(key))
        throw std::invalid_argument("Current reference contradicts this batch's lifecycle retirement");
    const auto [address, inserted] = plan.positive_by_address.emplace(key.instance.actor_address, key);
    if (!inserted && address->second != key)
        throw std::invalid_argument("One batch claims contradictory instances for same address");
    auto [entry, added] = plan.positive.emplace(key, IdentityObservation{stamp, direct});
    // 同批次实际顶层GU闭验负责生命周期，引用只核对一致并登记标量重命名。
    if (!added && direct && !entry->second.direct)
        entry->second = {stamp, direct};
    else if (!added && direct == entry->second.direct) {
        entry->second.interval.started = std::max(entry->second.interval.started, stamp.started);
        entry->second.interval.finished = std::max(entry->second.interval.finished, stamp.finished);
    }
}
void Projector::Patch(Plan& plan, sync::Value& value, SampleInterval stamp) {
    auto& number = Scalar(value);
    Need(plan, number, stamp);
    plan.patches.push_back({&number, number});
}
void Projector::Rename(Plan& plan, const std::map<u64, u64>& ids, std::map<u64, sync::RemovalReason> removals) {
    // 先检查全部小ID映射，后批量extract/rekey，避免真实ID撞仍未搬出的临时表键。
    std::set<u64> distinct;
    std::map<sync::InstanceKey, u64> entity_ids;
    for (const auto& [id, entity] : plan.state->entities) {

#if defined(MONITOR_PROJECTION_AUTHORITY_TESTING)
        ++plan.entity_key_lookups;
#endif

        (void)id;
        const auto symbol = plan.symbols_by_key.find(entity.key);
        if (symbol == plan.symbols_by_key.end() || !ids.contains(symbol->second))
            throw std::invalid_argument("Missing entity binding");
        const auto actual = ids.at(symbol->second);
        if (!actual || !distinct.insert(actual).second)
            throw std::invalid_argument("Entity binding collision");
        entity_ids.emplace(entity.key, actual);
    }
    for (const auto& patch : plan.patches)
        if (!ids.contains(patch.symbolic))
            throw std::invalid_argument("Missing reference binding");
    plan.entity_nodes.clear();
    while (!plan.state->entities.empty())
        plan.entity_nodes.push_back(plan.state->entities.extract(plan.state->entities.begin()));
    for (auto& node : plan.entity_nodes) {
        const auto actual = entity_ids.at(node.mapped().key);
        node.key() = node.mapped().id = actual;
    }
    for (auto& node : plan.entity_nodes)
        plan.state->entities.insert(std::move(node));
    for (const auto& patch : plan.patches)
        *patch.value = ids.at(patch.symbolic);
    plan.state->removals.swap(removals);
}

} // namespace monitor::synchronization::authority
