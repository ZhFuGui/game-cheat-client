#pragma once
#include "application/ports/source_inspection.hpp"
#include <span>
#include <string_view>

namespace mousebox::sources::inspection
{
struct Field
{
    unsigned number;
    std::string_view name;
    unsigned type;
    std::string_view child;
    bool repeated;
};
struct Message
{
    std::string_view name;
    std::span<const Field> fields;
};
// On-demand diagnostics only. Does not validate or grant any runtime eligibility.
// Present fields only; int64/fixed64 are decimal strings, bytes are hex strings.
application::Json protobufJson(std::string_view bytes, std::string_view message);
} // namespace mousebox::sources::inspection
