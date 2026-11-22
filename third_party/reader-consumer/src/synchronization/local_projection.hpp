#pragma once
#include "model/frame.hpp"
#include "synchronization/state.hpp"

namespace monitor::synchronization {
// 投影本批本地专属事实；相机、武器及观战数各自保留终态与真实采样时间。
void ProjectLocalValues(const Snapshot&, State&);
} // namespace monitor::synchronization
