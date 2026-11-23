#pragma once
#include "protocol/protobuf_reader.hpp"
#include <bit>
#include <cmath>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mousebox::sources::valorant_reader::wire
{
inline void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
struct Field
{
    unsigned type = 0;
    std::uint64_t value = 0;
    std::string_view bytes;
};
using Fields = std::map<unsigned, std::vector<Field>>;
inline Fields parse(std::string_view input)
{
    require(input.size() <= 16 * 1024 * 1024, "VALORANT message exceeds record limit");
    Fields out;
    protocol::protobuf::Reader reader(input);
    while (!reader.empty())
    {
        const auto [id, type] = reader.tag();
        Field field{type, 0, {}};
        if (type == 0)
            field.value = reader.varint();
        else if (type == 1)
            field.value = reader.fixed64();
        else if (type == 2)
            field.bytes = reader.block();
        else if (type == 5)
            field.value = reader.fixed32();
        else
            throw std::runtime_error("VALORANT unsupported protobuf wire type");
        out[id].push_back(field);
    }
    return out;
}
inline const std::vector<Field> &many(const Fields &fields, unsigned id)
{
    static const std::vector<Field> empty;
    const auto found = fields.find(id);
    return found == fields.end() ? empty : found->second;
}
inline const Field *one(const Fields &fields, unsigned id, unsigned type)
{
    const auto &values = many(fields, id);
    if (values.empty())
        return nullptr;
    require(values.size() == 1 && values.front().type == type, "VALORANT duplicate field or wrong wire type");
    return &values.front();
}
inline std::uint64_t integer(const Fields &fields, unsigned id, unsigned type = 0)
{
    const auto value = one(fields, id, type);
    return value ? value->value : 0;
}
inline std::optional<std::uint64_t> optionalInteger(const Fields &fields, unsigned id, unsigned type = 0)
{
    const auto value = one(fields, id, type);
    return value ? std::optional(value->value) : std::nullopt;
}
inline std::string_view bytes(const Fields &fields, unsigned id)
{
    const auto value = one(fields, id, 2);
    return value ? value->bytes : std::string_view{};
}
inline double real(const Field &field)
{
    const auto value = field.type == 5
                           ? static_cast<double>(std::bit_cast<float>(static_cast<std::uint32_t>(field.value)))
                           : std::bit_cast<double>(field.value);
    require(std::isfinite(value), "VALORANT nonfinite number");
    return value;
}
inline std::optional<double> optionalReal(const Fields &fields, unsigned id, unsigned type)
{
    const auto value = one(fields, id, type);
    return value ? std::optional(real(*value)) : std::nullopt;
}
inline std::string hex(std::string_view bytes)
{
    std::string result;
    for (const auto ch : bytes)
    {
        const auto byte = static_cast<unsigned char>(ch);
        result += "0123456789abcdef"[byte >> 4];
        result += "0123456789abcdef"[byte & 15];
    }
    return result;
}
inline void varint(std::string &out, std::uint64_t value)
{
    while (value >= 128)
    {
        out += static_cast<char>((value & 127) | 128);
        value >>= 7;
    }
    out += static_cast<char>(value);
}
inline void number(std::string &out, unsigned id, std::uint64_t value, unsigned type = 0)
{
    varint(out, (std::uint64_t(id) << 3) | type);
    if (!type)
        varint(out, value);
    else
        for (unsigned i = 0; i < (type == 1 ? 8u : 4u); ++i)
            out += static_cast<char>(value >> (i * 8));
}
inline void block(std::string &out, unsigned id, std::string_view value)
{
    varint(out, (std::uint64_t(id) << 3) | 2);
    varint(out, value.size());
    out += value;
}
} // namespace mousebox::sources::valorant_reader::wire
