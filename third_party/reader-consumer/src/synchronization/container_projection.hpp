#pragma once
#include "model/entities/container.hpp"
#include "synchronization/state.hpp"

namespace monitor::synchronization::detail {
// 只投影关闭后的本帧值；组件保持在容器内，不分配顶层 Actor ID。
std::map<unsigned, Value> ContainerContentsFields(const ContainerContentsSample&);
std::map<std::vector<unsigned>, GroupState::NestedSample> ContainerContentsSamples(const ContainerContentsSample&,
                                                                                   const std::map<unsigned, Value>&);
} // namespace monitor::synchronization::detail
