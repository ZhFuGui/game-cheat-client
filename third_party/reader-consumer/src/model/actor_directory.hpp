#pragma once
#include "model/frame.hpp"

namespace monitor::model {
// 名单是独立采集层。其真实观察可早于属性批次，不能以业务起点替代或刷新。
inline bool ValidDirectoryObservation(const Snapshot& frame) noexcept {
    const auto& sample = frame.directory_sample;
    return frame.directory_generation != 0 && frame.scene_list_valid &&
           sample.started != Clock::time_point{} && sample.finished >= sample.started &&
           sample.finished <= frame.started && sample.finished <= frame.finished && sample.started == frame.scene_list_started &&
           sample.finished == frame.scene_list_finished;
}
inline u64 DirectoryObservationNs(Clock::time_point value) noexcept {
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(value.time_since_epoch()).count();
    return ns > 0 ? static_cast<u64>(ns) : 0;
}
inline bool MatchesDirectoryObservation(const Snapshot& frame, u64 generation, u64 started_ns, u64 finished_ns) noexcept {
    return ValidDirectoryObservation(frame) && generation == frame.directory_generation &&
           started_ns == DirectoryObservationNs(frame.directory_sample.started) &&
           finished_ns == DirectoryObservationNs(frame.directory_sample.finished);
}
} // namespace monitor::model
