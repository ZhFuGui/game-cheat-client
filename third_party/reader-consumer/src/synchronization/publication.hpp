#pragma once
#include "model/current_facts.hpp"
#include "model/category.hpp"

namespace monitor::synchronization {
// 连接仅观察共同元数据；业务模型和Debug原生模型保持独立强类型。
// 不含地址/属性树，不提供把Consumer伪装为Native State的转换。
struct PublicationState {
    virtual ~PublicationState() = default;
    u64 epoch = 0, revision = 0, capture_sequence = 0;
    bool accepted = false;
    SampleInterval capture;
    std::optional<Clock::time_point> read_finished;
    virtual DataCategory Category() const noexcept = 0;
    virtual std::size_t EntityCount() const noexcept = 0;
    virtual bool SameEntity(u64 id, const PublicationState&) const noexcept = 0;
};
} // namespace monitor::synchronization
