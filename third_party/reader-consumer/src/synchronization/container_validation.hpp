#pragma once
#include "synchronization/state.hpp"

namespace monitor::synchronization {
// 容器仍是一条主 Level Actor；组件只能通过本帧所属集合的嵌套证据准入。
bool ContainerValueShape(PropertyGroup, unsigned field, const Value&, const State&, const EntityState&);
bool ContainerGroupValid(PropertyGroup, const GroupState&, const State&, const EntityState&,
                         const std::set<u64>& actor_addresses, std::set<u64>& member_addresses);
std::set<std::vector<unsigned>> ContainerNestedPaths(const std::map<unsigned, Value>&);
bool ContainerHasBusinessValue(const std::vector<unsigned>&, const Value&);
} // namespace monitor::synchronization
