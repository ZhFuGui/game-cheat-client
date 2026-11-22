#include "protobuf_json.hpp"
#include "generated_schema.hpp"
#include "protocol/protobuf_reader.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace mousebox::sources::inspection
{
namespace
{
using application::Json;
void require(bool value)
{
    if (!value)
        throw std::runtime_error("源数据格式或大小超出查看范围");
}
using protocol::protobuf::Reader;
std::string hex(std::string_view bytes)
{
    constexpr char chars[] = "0123456789abcdef";
    std::string out = "0x";
    for (const auto b : bytes)
    {
        out += chars[static_cast<unsigned char>(b) >> 4];
        out += chars[static_cast<unsigned char>(b) & 15];
    }
    return out;
}
Json real(double value)
{
    if (std::isfinite(value))
        return value;
    return std::isnan(value) ? Json("NaN") : value > 0 ? Json("Infinity") : Json("-Infinity");
}
class Decoder
{
    std::size_t nodes_ = 0;
    Json scalar(Reader &reader, const Field &field, unsigned wire, unsigned depth)
    {
        require(++nodes_ <= 200000);
        if (field.type == 9 || field.type == 11 || field.type == 12)
        {
            require(wire == 2);
            const auto bytes = reader.block();
            if (field.type == 9)
                return std::string(bytes);
            if (field.type == 12)
                return hex(bytes);
            return message(bytes, field.child, depth + 1);
        }
        const unsigned expected = field.type == 1 || field.type == 6 || field.type == 16   ? 1
                                  : field.type == 2 || field.type == 7 || field.type == 15 ? 5
                                                                                           : 0;
        require(wire == expected);
        const auto n = wire == 1 ? reader.fixed64() : wire == 5 ? reader.fixed32() : reader.varint();
        switch (field.type)
        {
        case 1:
            return real(std::bit_cast<double>(n));
        case 2:
            return real(std::bit_cast<float>(static_cast<std::uint32_t>(n)));
        case 3:
        case 16:
            return std::to_string(std::bit_cast<std::int64_t>(n));
        case 4:
        case 6:
            return std::to_string(n);
        case 5:
        case 14:
        case 15:
            return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(n));
        case 7:
        case 13:
            return static_cast<std::uint32_t>(n);
        case 8:
            require(n <= 1);
            return n != 0;
        case 17:
            return static_cast<std::int32_t>((n >> 1) ^ (0ULL - (n & 1)));
        case 18:
            return std::to_string(std::bit_cast<std::int64_t>((n >> 1) ^ (0ULL - (n & 1))));
        default:
            throw std::runtime_error("源数据字段类型不可用");
        }
    }

  public:
    Json message(std::string_view bytes, std::string_view name, unsigned depth = 0)
    {
        require(depth <= 32);
        const auto schema = std::lower_bound(std::begin(generated::messages), std::end(generated::messages), name,
                                             [](const Message &m, std::string_view key) { return m.name < key; });
        require(schema != std::end(generated::messages) && schema->name == name);
        Json out = Json::object();
        Reader reader{bytes};
        while (!reader.empty())
        {
            const auto [number, wire] = reader.tag();
            const auto f = std::find_if(schema->fields.begin(), schema->fields.end(),
                                        [&](const Field &v) { return v.number == number; });
            if (f == schema->fields.end())
            {
                require(++nodes_ <= 200000);
                Json value;
                if (wire == 0)
                    value = std::to_string(reader.varint());
                else if (wire == 1 || wire == 5)
                    value = std::to_string((wire == 1 ? reader.fixed64() : reader.fixed32()));
                else
                {
                    require(wire == 2);
                    value = hex(reader.block());
                }
                out["$unknown"][std::to_string(number)].push_back({{"wire", wire}, {"value", std::move(value)}});
                continue;
            }
            const auto key = std::string(f->name);
            if (f->repeated)
            {
                if (!out.contains(key))
                    out[key] = Json::array();
                if (wire == 2 && f->type != 9 && f->type != 11 && f->type != 12)
                {
                    Reader packed{reader.block()};
                    const unsigned kind = f->type == 1 || f->type == 6 || f->type == 16   ? 1
                                          : f->type == 2 || f->type == 7 || f->type == 15 ? 5
                                                                                          : 0;
                    while (!packed.empty())
                        out[key].push_back(scalar(packed, *f, kind, depth));
                }
                else
                    out[key].push_back(scalar(reader, *f, wire, depth));
            }
            else
                out[key] = scalar(reader, *f, wire, depth);
        }
        return out;
    }
};
} // namespace
application::Json protobufJson(std::string_view bytes, std::string_view message)
{
    require(bytes.size() <= 16 * 1024 * 1024);
    try
    {
        return Decoder{}.message(bytes, message);
    }
    catch (const protocol::protobuf::WireError &)
    {
        throw std::runtime_error("源数据格式或大小超出查看范围");
    }
}
} // namespace mousebox::sources::inspection
