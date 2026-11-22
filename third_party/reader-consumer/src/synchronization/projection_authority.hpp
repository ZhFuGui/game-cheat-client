#pragma once
// 私有属性投影与统一身份事务分离；完整属性树和校验不持有权威注册表锁。
#include "synchronization/projection.hpp"
#include "synchronization/consumer/projection.hpp"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace monitor::synchronization::authority {
namespace sync = monitor::synchronization;
#if defined(MONITOR_PROJECTION_AUTHORITY_TESTING)
// 仅隔离夹具观察锁边界，不属于发布数据或产品状态。
inline thread_local bool registry_lock_held = false;
#endif
struct StaleEvidence : std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct Evidence {
    sync::InstanceKey key;
    Clock::time_point observed{};
    Clock::time_point observed_until{};
    bool retired = false;
    bool operator==(const Evidence&) const = default;
};
struct IdentityObservation {
    SampleInterval interval;
    // 顶层实体使用实际FUObjectItem关闭区间；跨流引用保守窗口仅供首次绑定与失效排序。
    bool direct = false;
};
// 证据按实体种类共享不可变表。事务只复制实际修改的种类，不复制低频物资证据。
// 每地址只有一个当前证据；跨种类复用仍通过所有种类的对数查询统一裁决。
class EvidenceIndex {
    using Table = std::map<u64, Evidence>;
    static constexpr std::size_t Count = unsigned(EntityKind::Container) + 1;
    std::array<std::shared_ptr<Table>, Count> tables_{};
    Table& Writable(EntityKind kind) {
        auto& table = tables_.at(unsigned(kind));
        if (!table)
            table = std::make_shared<Table>();
        else if (table.use_count() != 1)
            table = std::make_shared<Table>(*table);
        return *table;
    }

  public:
    const Evidence* Find(u64 address) const noexcept {
        for (const auto& table : tables_)
            if (table) {
                const auto found = table->find(address);
                if (found != table->end())
                    return &found->second;
            }
        return nullptr;
    }
    void Set(u64 address, Evidence value) {
        const auto kind = value.key.kind;
        // 所有分配只发生于私有changed快照，任何异常都不影响已发布权威表。
        for (unsigned i = 0; i < Count; ++i)
            if (i != unsigned(kind) && tables_[i] && tables_[i]->contains(address))
                Writable(static_cast<EntityKind>(i)).erase(address);
        Writable(kind).insert_or_assign(address, std::move(value));
    }
    void clear() noexcept { tables_.fill({}); }
    std::size_t size() const noexcept {
        std::size_t count = 0;
        for (const auto& table : tables_)
            if (table)
                count += table->size();
        return count;
    }
    bool empty() const noexcept { return size() == 0; }
    const Evidence& at(u64 address) const {
        if (const auto found = Find(address))
            return *found;
        throw std::out_of_range("Identity evidence is absent");
    }
#if defined(MONITOR_PROJECTION_AUTHORITY_TESTING)
    const void* StorageIdentity(EntityKind kind) const noexcept { return tables_[unsigned(kind)].get(); }
#endif
};
struct Authority {
    sync::InstanceRegistry ids;
    EvidenceIndex evidence;
    using TokenMap = std::map<consumer::TokenKey, u64>;
    std::shared_ptr<const TokenMap> tokens = std::make_shared<TokenMap>();
    u64 next_token = 1;
    u64 version = 0;
};
struct View {
    std::shared_ptr<const Authority> authority;
    u64 permitted_epoch = 0, fence = 0;
};
struct PointerPatch {
    u64* value = nullptr;
    u64 symbolic = 0;
};
struct RetireProposal {
    sync::InstanceKey key;
    u64 expected_id = 0;
    SampleInterval observed;
};
struct InstanceKeyHash {
    std::size_t operator()(const sync::InstanceKey& key) const noexcept {
        auto value = std::hash<u64>{}(key.instance.actor_address);
        const auto mix = [&](u64 part) {
            value ^= std::hash<u64>{}(part) + 0x9e3779b97f4a7c15ull + (value << 6) + (value >> 2);
        };
        mix(static_cast<u64>(key.kind));
        mix(static_cast<u32>(key.instance.internal_index));
        mix(key.instance.serial_number);
        return value;
    }
};
struct IdentityPlan {
    u64 epoch = 0;
    DataCategory category = DataCategory::None;
    std::unordered_map<u64, sync::InstanceKey> symbols;
    std::unordered_map<sync::InstanceKey, u64, InstanceKeyHash> symbols_by_key;
    std::map<sync::InstanceKey, IdentityObservation> positive;
    std::map<u64, sync::InstanceKey> positive_by_address;
    std::vector<PointerPatch> patches;
    std::map<u64, consumer::TokenKey> tokens;
    std::vector<PointerPatch> token_patches;
    std::vector<RetireProposal> retires;
    std::map<u64, sync::RemovalReason> original_removals;
    u64 fence = 0, committed_next_id = 0;
    unsigned binding_attempts = 0;
#if defined(MONITOR_PROJECTION_AUTHORITY_TESTING)
    unsigned projection_runs = 0;
    std::size_t fact_index_entries = 0, registry_index_entries = 0;
    std::size_t positive_index_lookups = 0, entity_key_lookups = 0;
#endif
};
struct Plan : IdentityPlan {
    std::shared_ptr<sync::State> state;
    std::vector<std::map<u64, sync::EntityState>::node_type> entity_nodes;
};
struct ConsumerPlan : IdentityPlan {
    std::shared_ptr<consumer::State> state;
    std::map<u64, sync::InstanceKey> entity_keys;
    std::vector<std::map<u64, consumer::Entity>::node_type> entity_nodes;
};

inline sync::Value::Object& Object(sync::Value& value) {
    return std::get<sync::Value::Object>(value.data);
}
inline sync::Value::List& List(sync::Value& value) {
    return std::get<sync::Value::List>(value.data);
}
inline u64& Scalar(sync::Value& value) {
    return std::get<u64>(value.data);
}

enum class CommitResult { Rejected, Retry, Committed };
class Projector {
  public:
    // 仅用于隔离夹具制造提交异常或CAS竞争；正式Output不传入此钩子。
    using Hook = std::function<void(const char*, unsigned)>;
    // owner gate必须在自身短锁内核对ticket；then()仅原子提交小注册表指针。
    using Gate = std::function<CommitResult(const std::function<CommitResult()>&)>;
    explicit Projector(u64 next_id = 1);
    View Inspect() const;
    void AdvanceEpoch(u64 epoch);
    void ObserveEpoch(u64 epoch);
    Plan Prepare(const Snapshot&, u64 epoch, u64 revision, const sync::ProjectionOptions&);
    std::shared_ptr<const sync::State> Commit(Plan&, const Hook& = {}, const Gate& = {});
    ConsumerPlan PrepareConsumer(const Snapshot&, u64 epoch, u64 revision, const sync::ProjectionOptions&);
    std::shared_ptr<const consumer::State> CommitConsumer(ConsumerPlan&, const Hook& = {}, const Gate& = {});

  private:
    struct LockMark;
    using RenameOperation =
        std::function<void(const std::map<u64, u64>&, const std::map<u64, u64>&, std::map<u64, sync::RemovalReason>)>;
    void CommitIdentity(IdentityPlan&, bool entities_empty, const RenameOperation&, const Hook&, const Gate&);
    static void Need(IdentityPlan&, u64, SampleInterval, bool direct = false);
    static void Patch(Plan&, sync::Value&, SampleInterval);
    static void Rename(Plan&, const std::map<u64, u64>&, std::map<u64, sync::RemovalReason>);
    static void RenameConsumer(ConsumerPlan&, const std::map<u64, u64>&, const std::map<u64, u64>&,
                               std::map<u64, sync::RemovalReason>);
    mutable std::mutex mutex_;
    std::shared_ptr<const Authority> authority_;
    u64 permitted_epoch_ = 0, fence_ = 0;
};
} // namespace monitor::synchronization::authority
