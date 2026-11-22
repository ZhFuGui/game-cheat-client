#pragma once
namespace monitor::diagnostics {
// 同步算法仅在真正完成字段、实体或成员后调用；没有计时心跳，也不依赖线程池或网络。
struct CpuProgressHook {
    void (*completed)(void*) = nullptr;
    void* owner = nullptr;
};
inline thread_local CpuProgressHook cpu_progress_hook;
class CpuProgressScope {
    CpuProgressHook previous_;
  public:
    explicit CpuProgressScope(CpuProgressHook hook) noexcept : previous_(cpu_progress_hook) { cpu_progress_hook = hook; }
    ~CpuProgressScope() { cpu_progress_hook = previous_; }
    CpuProgressScope(const CpuProgressScope&) = delete;
    CpuProgressScope& operator=(const CpuProgressScope&) = delete;
};
inline void CompletedCpuUnit() {
    if (cpu_progress_hook.completed) cpu_progress_hook.completed(cpu_progress_hook.owner);
}
} // namespace monitor::diagnostics
