#pragma once
#include "model/current_facts.hpp"
#include "model/category.hpp"
#include <array>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <optional>

namespace monitor::synchronization {
// 只保留实例对应关系，不缓存属性值。不同 epoch、索引或序列号绝不复用 ID。
struct InstanceKey {
    EntityKind kind = EntityKind::Unknown;
    InstanceIdentity instance;
    bool operator==(const InstanceKey&) const = default;
    bool operator<(const InstanceKey& other) const noexcept;
};
class InstanceRegistry {
  public:
    using EntriesMap = std::map<InstanceKey, u64>;
    explicit InstanceRegistry(u64 next_id = 1);
    void Fence(u64 epoch);
    u64 Resolve(EntityKind, const InstanceIdentity&);
    std::optional<u64> Find(EntityKind, const InstanceIdentity&) const noexcept;
    void Retire(EntityKind, const InstanceIdentity&);
    u64 NextId() const noexcept { return next_id_; }
    u64 Epoch() const noexcept { return epoch_; }
    // 快照只复制固定数量的种类指针；首次修改某种类时才复制该种类的身份表。
    // 高频活体/载具事务不会复制低频地面物资表。
    InstanceRegistry Scope(DataCategory) const;
    std::vector<std::pair<InstanceKey, u64>> AtAddress(u64) const;
    const EntriesMap& Entries() const;
#if defined(MONITOR_PROJECTION_AUTHORITY_TESTING)
    const void* StorageIdentity(EntityKind kind) const noexcept { return instances_[unsigned(kind)].get(); }
#endif

  private:
    static constexpr std::size_t KindCount = unsigned(EntityKind::Container) + 1;
    struct Aggregate {
        std::once_flag once;
        EntriesMap entries;
        std::atomic_bool materialized{false};
    };
    EntriesMap& Writable(EntityKind);
    u64 epoch_ = 0, next_id_ = 1;
    std::array<std::shared_ptr<EntriesMap>, KindCount> instances_{};
    // 仅显式枚举时按需物化。快照共享不可变缓存，call_once保护并发Inspect读者。
    std::shared_ptr<Aggregate> aggregate_ = std::make_shared<Aggregate>();
};
} // namespace monitor::synchronization
