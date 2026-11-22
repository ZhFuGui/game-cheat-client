#pragma once
#include <optional>
namespace native_cli {
// Returns nullopt only for the dedicated bridge mode. CLI modes never create
// the desktop backend; reader and replay remain read-only.
std::optional<int> dispatch(int argc, wchar_t** argv);
}
