#include "memory/issue.hpp"
#include <iomanip>
#include <sstream>
namespace monitor {
const char* CodeName(Code c) noexcept {
#define ITEM(x)                                                                                                        \
    case Code::x:                                                                                                      \
        return #x
    switch (c) {
        ITEM(None);
        ITEM(Cancelled);
        ITEM(Deadline);
        ITEM(ReadFailed);
        ITEM(InvalidAddress);
        ITEM(InvalidLayout);
        ITEM(NullObject);
        ITEM(NoProcess);
        ITEM(MultipleProcesses);
        ITEM(ProcessExited);
        ITEM(IdentityUnavailable);
        ITEM(SessionChanged);
        ITEM(ImageMismatch);
        ITEM(BackendUnavailable);
        ITEM(UnsupportedInstruction);
        ITEM(UnsupportedPrefix);
        ITEM(UnknownRegister);
        ITEM(CodeChanged);
        ITEM(SnapshotChanged);
        ITEM(InstructionLimit);
        ITEM(NoReference);
        ITEM(InternalError);
    }
#undef ITEM
    return "Unknown";
}
std::string Hex(u64 x) {
    std::ostringstream o;
    o << "0x" << std::hex << std::uppercase << std::setw(16) << std::setfill('0') << x;
    return o.str();
}
std::string Describe(const Issue& x) {
    if (!x)
        return "OK";
    std::ostringstream o;
    o << '[' << x.stage << "] " << CodeName(x.code) << ": " << x.detail;
    if (x.address)
        o << " address=" << Hex(x.address);
    if (x.bytes)
        o << " bytes=" << x.bytes;
    if (x.attempts)
        o << " attempts=" << x.attempts;
    if (x.native_error)
        o << " native=" << *x.native_error;
    return o.str();
}
} // namespace monitor
