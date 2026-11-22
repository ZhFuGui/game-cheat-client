#include "field_values.hpp"
#include "codec.hpp"
#include "protocol/protobuf_reader.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace mousebox::sources::cs2_reader::detail
{
void require(bool v, const char *m)
{
    if (!v)
        throw std::runtime_error(m);
}
Fields fields(std::string_view input)
{
    require(input.size() <= MaximumRecord, "CS2 oversized message");
    Fields out;
    protocol::protobuf::Reader reader(input);
    try
    {
        for (std::size_t count = 0; !reader.empty(); ++count)
        {
            require(count < 200000, "CS2 field bound");
            const auto [id, wire] = reader.tag();
            Field field;
            field.wire = wire;
            switch (wire)
            {
            case 0:
                field.n = reader.varint();
                break;
            case 1:
                field.n = reader.fixed64();
                break;
            case 2:
                field.bytes = reader.block();
                break;
            case 5:
                field.n = reader.fixed32();
                break;
            default:
                throw std::runtime_error("CS2 unsupported wire type");
            }
            out[id].push_back(field);
        }
    }
    catch (const protocol::protobuf::WireError &error)
    {
        throw std::runtime_error(std::string("CS2 ") + error.what());
    }
    return out;
}
const Field *one(const Fields &f, unsigned id, unsigned w)
{
    auto it = f.find(id);
    if (it == f.end())
        return nullptr;
    require(it->second.size() == 1 && it->second.front().wire == w, "CS2 duplicate/wrong scalar type");
    return &it->second.front();
}
const std::vector<Field> &many(const Fields &f, unsigned id)
{
    static const std::vector<Field> empty;
    auto i = f.find(id);
    return i == f.end() ? empty : i->second;
}
std::uint64_t number(const Fields &f, unsigned id, unsigned w)
{
    auto p = one(f, id, w);
    return p ? p->n : 0;
}
std::string_view bytes(const Fields &f, unsigned id)
{
    auto p = one(f, id, 2);
    return p ? p->bytes : std::string_view{};
}
std::string hex(std::string_view b)
{
    std::string out;
    for (unsigned char c : b)
    {
        out += "0123456789abcdef"[c >> 4];
        out += "0123456789abcdef"[c & 15];
    }
    return out;
}
std::string text(std::string_view input)
{
    require(input.size() <= 4096, "CS2 text bound");
    std::size_t offset = 0;
    while (offset < input.size())
    {
        auto lead = static_cast<unsigned char>(input[offset++]);
        if (lead < 0x80)
            continue;
        const unsigned count = lead >= 0xc2 && lead <= 0xdf   ? 1
                               : lead >= 0xe0 && lead <= 0xef ? 2
                               : lead >= 0xf0 && lead <= 0xf4 ? 3
                                                              : 0;
        require(count && count <= input.size() - offset, "CS2 invalid UTF-8");
        std::uint32_t code = lead & ((1u << (6 - count)) - 1u);
        for (unsigned i = 0; i < count; ++i)
        {
            auto tail = static_cast<unsigned char>(input[offset++]);
            require((tail & 0xc0) == 0x80, "CS2 invalid UTF-8 continuation");
            code = (code << 6) | (tail & 63);
        }
        require(code >= (count == 1   ? 0x80u
                         : count == 2 ? 0x800u
                                      : 0x10000u) &&
                    code <= 0x10ffff && !(code >= 0xd800 && code <= 0xdfff),
                "CS2 noncanonical UTF-8");
    }
    return std::string(input);
}
double real(const Field &f)
{
    auto v = f.wire == 5 ? static_cast<double>(std::bit_cast<float>(static_cast<std::uint32_t>(f.n)))
                         : std::bit_cast<double>(f.n);
    require(std::isfinite(v), "CS2 nonfinite number");
    return v;
}
std::optional<double> optionalFloat(const Fields &f, unsigned id)
{
    auto p = one(f, id, 5);
    return p ? std::optional(real(*p)) : std::nullopt;
}
std::optional<std::uint64_t> optional64(const Fields &f, unsigned id)
{
    auto p = one(f, id, 1);
    return p ? std::optional(p->n) : std::nullopt;
}
std::optional<unsigned> optionalUnsigned(const Fields &f, unsigned id)
{
    auto p = one(f, id, 0);
    if (!p)
        return {};
    require(p->n <= UINT32_MAX, "CS2 uint32 overflow");
    return static_cast<unsigned>(p->n);
}
std::optional<int> optionalInt(const Fields &f, unsigned id)
{
    auto p = optionalUnsigned(f, id);
    if (!p)
        return {};
    auto n = *p;
    return static_cast<int>((n >> 1) ^ static_cast<unsigned>(-static_cast<int>(n & 1)));
}
std::optional<bool> optionalBool(const Fields &f, unsigned id)
{
    auto p = one(f, id, 0);
    if (!p)
        return {};
    require(p->n <= 1, "CS2 invalid bool");
    return p->n != 0;
}
std::pair<std::string, std::uint64_t> decodeClock(std::string_view b)
{
    auto f = fields(b);
    auto domain = bytes(f, 1);
    auto freq = number(f, 2, 1);
    require(domain.size() == 16 && freq > 0, "CS2 clock descriptor invalid");
    return {hex(domain), freq};
}
} // namespace mousebox::sources::cs2_reader::detail
