#pragma once
#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace mousebox::protocol::protobuf
{
class WireError : public std::runtime_error
{
  public:
    using std::runtime_error::runtime_error;
};

struct Tag
{
    unsigned number, wire;
};

// Borrows the supplied bytes. Record size, nesting, cardinality and schema
// validation belong to the decoder that consumes this cursor.
class Reader
{
    std::string_view remaining_;

    std::string_view take(std::uint64_t count, const char *error)
    {
        if (count > remaining_.size())
            throw WireError(error);
        auto value = remaining_.substr(0, static_cast<std::size_t>(count));
        remaining_.remove_prefix(value.size());
        return value;
    }

    std::uint64_t fixed(unsigned size)
    {
        auto bytes = take(size, "truncated fixed value");
        std::uint64_t value = 0;
        for (unsigned i = 0; i < size; ++i)
            value |= std::uint64_t(static_cast<unsigned char>(bytes[i])) << (8 * i);
        return value;
    }

  public:
    explicit Reader(std::string_view bytes) : remaining_(bytes) {}
    bool empty() const noexcept
    {
        return remaining_.empty();
    }
    std::uint64_t varint()
    {
        std::uint64_t value = 0;
        for (unsigned i = 0; i < 10; ++i)
        {
            const auto byte = static_cast<unsigned char>(take(1, "truncated varint").front());
            if (i == 9 && byte > 1)
                throw WireError("varint overflow");
            value |= std::uint64_t(byte & 127) << (7 * i);
            if (!(byte & 128))
                return value;
        }
        throw WireError("invalid varint");
    }
    Tag tag()
    {
        const auto raw = varint();
        const auto number = raw >> 3;
        if (number > 0x1fffffff)
            throw WireError("field tag overflow");
        if (number == 0)
            throw WireError("invalid field tag");
        return {static_cast<unsigned>(number), static_cast<unsigned>(raw & 7)};
    }
    std::uint64_t fixed32()
    {
        return fixed(4);
    }
    std::uint64_t fixed64()
    {
        return fixed(8);
    }
    std::string_view block()
    {
        return take(varint(), "truncated message");
    }
};
} // namespace mousebox::protocol::protobuf
