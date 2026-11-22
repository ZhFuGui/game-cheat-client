#pragma once
#include "model/primitives.hpp"
#include <exception>
#include <utility>
namespace monitor {
enum class Code {
    None,
    Cancelled,
    Deadline,
    ReadFailed,
    InvalidAddress,
    InvalidLayout,
    NullObject,
    NoProcess,
    MultipleProcesses,
    ProcessExited,
    IdentityUnavailable,
    SessionChanged,
    ImageMismatch,
    BackendUnavailable,
    UnsupportedInstruction,
    UnsupportedPrefix,
    UnknownRegister,
    CodeChanged,
    SnapshotChanged,
    InstructionLimit,
    NoReference,
    InternalError
};
const char* CodeName(Code code) noexcept;
struct Issue {
    Code code = Code::None;
    std::string stage;
    std::string detail;
    u64 address = 0;
    std::size_t bytes = 0;
    unsigned attempts = 0;
    std::optional<u32> native_error; // Only when the backend/API actually returns one.
    Issue() = default;
    Issue(Code c, std::string s, std::string d, u64 a = 0, std::size_t n = 0, unsigned tries = 0,
          std::optional<u32> native = std::nullopt)
        : code(c), stage(std::move(s)), detail(std::move(d)), address(a), bytes(n), attempts(tries),
          native_error(native) {}
    explicit operator bool() const noexcept { return code != Code::None; }
};
std::string Describe(const Issue& issue);
std::string Hex(u64 value);
class Fault final : public std::exception {
  public:
    explicit Fault(Issue issue) : issue_(std::move(issue)) {}
    const char* what() const noexcept override { return issue_.detail.c_str(); }
    const Issue& issue() const noexcept { return issue_; }

  private:
    Issue issue_;
};
[[noreturn]] inline void Fail(Code code, const char* stage, std::string message, u64 address = 0,
                              std::size_t bytes = 0) {
    throw Fault(Issue{code, stage, std::move(message), address, bytes, 0, std::nullopt});
}
} // namespace monitor
