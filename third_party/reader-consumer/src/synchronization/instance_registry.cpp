#include "synchronization/instance_registry.hpp"
#include <limits>
#include <stdexcept>
#include <tuple>

namespace monitor::synchronization {
bool InstanceKey::operator<(const InstanceKey& b) const noexcept {
    return std::tie(kind, instance.actor_address, instance.internal_index, instance.serial_number) <
           std::tie(b.kind, b.instance.actor_address, b.instance.internal_index, b.instance.serial_number);
}
InstanceRegistry::InstanceRegistry(u64 next_id) : next_id_(next_id) {
    if (!next_id)
        throw std::invalid_argument("Entity ID zero is reserved");
}
void InstanceRegistry::Fence(u64 epoch) {
    if (!epoch || epoch < epoch_)
        throw std::invalid_argument("Instance epoch cannot regress");
    if (epoch != epoch_) {
        auto cache = std::make_shared<Aggregate>();
        instances_.fill({});
        aggregate_ = std::move(cache);
        epoch_ = epoch;
    }
}
u64 InstanceRegistry::Resolve(EntityKind kind, const InstanceIdentity& instance) {
    if (!epoch_ || kind < EntityKind::Character || kind > EntityKind::Container || !instance.actor_address ||
        instance.internal_index < 0)
        throw std::invalid_argument("Instance requires a verified kind and lifecycle identity");
    const InstanceKey key{kind, instance};
    if (const auto found = Find(kind, instance))
        return *found;
    if (!next_id_ || next_id_ == std::numeric_limits<u64>::max())
        throw std::overflow_error("Entity ID space exhausted");
    const auto id = next_id_;
    Writable(kind).emplace(key, id);
    ++next_id_;
    return id;
}
std::optional<u64> InstanceRegistry::Find(EntityKind kind, const InstanceIdentity& instance) const noexcept {
    if (unsigned(kind) >= instances_.size() || !instances_[unsigned(kind)])
        return std::nullopt;
    const auto& entries = *instances_[unsigned(kind)];
    const auto found = entries.find({kind, instance});
    return found == entries.end() ? std::nullopt : std::optional<u64>(found->second);
}
void InstanceRegistry::Retire(EntityKind kind, const InstanceIdentity& instance) {
    if (Find(kind, instance))
        Writable(kind).erase({kind, instance});
}
InstanceRegistry::EntriesMap& InstanceRegistry::Writable(EntityKind kind) {
    auto& entries = instances_.at(unsigned(kind));
    // 提前准备所有可能分配的对象，再替换本地指针；异常不改变原快照。
    auto cache = aggregate_.use_count() != 1 || aggregate_->materialized.load(std::memory_order_acquire)
                     ? std::make_shared<Aggregate>() : aggregate_;
    auto changed = !entries ? std::make_shared<EntriesMap>()
                           : entries.use_count() == 1 ? entries : std::make_shared<EntriesMap>(*entries);
    aggregate_ = std::move(cache);
    entries = std::move(changed);
    return *entries;
}
InstanceRegistry InstanceRegistry::Scope(DataCategory category) const {
    InstanceRegistry scoped(next_id_);
    scoped.epoch_ = epoch_;
    const auto include = [&](EntityKind kind) { scoped.instances_[unsigned(kind)] = instances_[unsigned(kind)]; };
    switch (category) {
        case DataCategory::Living:
            include(EntityKind::Character);
            include(EntityKind::Creature);
            break;
        case DataCategory::Vehicles:
            include(EntityKind::Vehicle);
            break;
        case DataCategory::GroundLoot:
            include(EntityKind::Pickup);
            include(EntityKind::Container); // GroundGroup属于地面物资，不是真正容器订阅。
            break;
        case DataCategory::Containers:
            include(EntityKind::Container);
            break;
        case DataCategory::Projectiles:
            break; // 尚无实现，不伪造任何种类。
        case DataCategory::Local:
            break; // 只在本批中登记需要绑定的引用，不枚举任何历史实体。
        default:
            return *this; // 独立协议/旧夹具的未分类全量入口。
    }
    return scoped;
}
std::vector<std::pair<InstanceKey, u64>> InstanceRegistry::AtAddress(u64 address) const {
    std::vector<std::pair<InstanceKey, u64>> found;
    for (unsigned index = unsigned(EntityKind::Character); index < instances_.size(); ++index) {
        if (!instances_[index])
            continue;
        const auto& entries = *instances_[index];
        const auto kind = static_cast<EntityKind>(index);
        for (auto it = entries.lower_bound({kind, {address, 0, 0}});
             it != entries.end() && it->first.instance.actor_address == address; ++it)
            found.emplace_back(it->first, it->second);
    }
    return found;
}
const InstanceRegistry::EntriesMap& InstanceRegistry::Entries() const {
    std::call_once(aggregate_->once, [&] {
        EntriesMap collected;
        for (const auto& entries : instances_)
            if (entries)
                collected.insert(entries->begin(), entries->end());
        aggregate_->entries = std::move(collected);
        aggregate_->materialized.store(true, std::memory_order_release);
    });
    return aggregate_->entries;
}
} // namespace monitor::synchronization
