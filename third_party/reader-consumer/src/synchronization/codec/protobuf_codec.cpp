#include "synchronization/codec/detail.hpp"
#include "diagnostics/work_progress.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <limits>

namespace monitor::synchronization::codec::detail {
void Require(bool value, const char* text, ErrorCode code) {
    if (!value)
        Fail(text, code);
}
namespace {
const Field& Find(int schema, unsigned number) {
    for (const auto& field : Schemas.at(schema).fields)
        if (field.number == number)
            return field;
    Fail("Unknown protobuf field");
}
bool Utf8(std::string_view text) {
    for (std::size_t i = 0; i < text.size();) {
        const auto lead = static_cast<unsigned char>(text[i++]);
        if (lead < 0x80)
            continue;
        unsigned count = lead >= 0xc2 && lead <= 0xdf   ? 1
                         : lead >= 0xe0 && lead <= 0xef ? 2
                         : lead >= 0xf0 && lead <= 0xf4 ? 3
                                                        : 0;
        if (!count || text.size() - i < count)
            return false;
        const auto first = static_cast<unsigned char>(text[i]);
        if ((lead == 0xe0 && first < 0xa0) || (lead == 0xed && first >= 0xa0) || (lead == 0xf0 && first < 0x90) ||
            (lead == 0xf4 && first >= 0x90))
            return false;
        while (count--)
            if ((static_cast<unsigned char>(text[i++]) & 0xc0) != 0x80)
                return false;
    }
    return true;
}
unsigned WireType(Scalar scalar) {
    switch (scalar) {
        case Scalar::Fixed64:
        case Scalar::SFixed64:
        case Scalar::Double:
            return 1;
        case Scalar::String:
        case Scalar::Bytes:
        case Scalar::Message:
            return 2;
        case Scalar::Fixed32:
        case Scalar::SFixed32:
        case Scalar::Float:
            return 5;
        default:
            return 0;
    }
}
u64 Varint(std::string_view& bytes) {
    u64 value = 0;
    for (unsigned i = 0; i < 10; ++i) {
        Require(!bytes.empty(), "Truncated protobuf varint");
        auto byte = static_cast<unsigned char>(bytes.front());
        bytes.remove_prefix(1);
        Require(i < 9 || byte <= 1, "Overflow protobuf varint");
        value |= u64(byte & 0x7f) << (i * 7);
        if (!(byte & 0x80))
            return value;
    }
    Fail("Overflow protobuf varint");
}
u64 Fixed(std::string_view& bytes, unsigned size) {
    Require(bytes.size() >= size, "Truncated protobuf fixed value");
    u64 result = 0;
    for (unsigned i = 0; i < size; ++i)
        result |= u64(static_cast<unsigned char>(bytes[i])) << (i * 8);
    bytes.remove_prefix(size);
    return result;
}
std::string_view Data(std::string_view& bytes) {
    const auto size = Varint(bytes);
    Require(size <= bytes.size(), "Truncated protobuf message");
    auto value = bytes.substr(0, static_cast<std::size_t>(size));
    bytes.remove_prefix(static_cast<std::size_t>(size));
    return value;
}
class Writer {
  public:
    std::string bytes;
    std::size_t limit;
    explicit Writer(std::size_t maximum) : limit(maximum) {}
    void Append(std::string_view text) {
        Require(text.size() <= limit - bytes.size(), "Encoded message exceeds limit", ErrorCode::TooLarge);
        bytes.append(text);
    }
    void Byte(unsigned value) {
        Require(bytes.size() < limit, "Encoded message exceeds limit", ErrorCode::TooLarge);
        bytes.push_back(char(value));
    }
    void Integer(u64 value) {
        while (value >= 0x80) {
            Byte((value & 0x7f) | 0x80);
            value >>= 7;
        }
        Byte(unsigned(value));
    }
    void FixedValue(u64 value, unsigned count) {
        for (unsigned i = 0; i < count; ++i)
            Byte(unsigned((value >> (i * 8)) & 0xff));
    }
};
Value Default(const Field& f) {
    switch (f.scalar) {
        case Scalar::String:
        case Scalar::Bytes:
            return Value(std::string{});
        case Scalar::Float:
        case Scalar::Double:
            return Value(0.0);
        case Scalar::Bool:
            return Value(false);
        case Scalar::Int32:
        case Scalar::Int64:
        case Scalar::SInt32:
        case Scalar::SInt64:
        case Scalar::SFixed32:
        case Scalar::SFixed64:
            return Value(std::int64_t(0));
        default:
            return Value(u64(0));
    }
}
Object Parse(int schema, std::string_view bytes, unsigned depth, bool restore_defaults = true);
Value ScalarValue(const Field& f, std::string_view& bytes, unsigned wire, unsigned depth,
                  bool restore_message_defaults = true) {
    Require(wire == WireType(f.scalar), "Wrong protobuf wire type");
    switch (f.scalar) {
        case Scalar::Message:
            return Value(Parse(f.child, Data(bytes), depth + 1, restore_message_defaults));
        case Scalar::String:
        case Scalar::Bytes: {
            auto text = Data(bytes);
            Require(f.scalar == Scalar::Bytes || Utf8(text), "Invalid UTF-8 protobuf string");
            return Value(std::string(text));
        }
        case Scalar::Float: {
            auto value = std::bit_cast<float>(static_cast<std::uint32_t>(Fixed(bytes, 4)));
            Require(std::isfinite(value), "Non-finite float");
            return Value(double(value));
        }
        case Scalar::Double: {
            auto value = std::bit_cast<double>(Fixed(bytes, 8));
            Require(std::isfinite(value), "Non-finite double");
            return Value(value);
        }
        case Scalar::Fixed64:
            return Value(Fixed(bytes, 8));
        case Scalar::Fixed32:
            return Value(Fixed(bytes, 4));
        case Scalar::SFixed32:
            return Value(std::int64_t(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(Fixed(bytes, 4)))));
        case Scalar::SFixed64:
            return Value(std::bit_cast<std::int64_t>(Fixed(bytes, 8)));
        default:
            break;
    }
    const auto value = Varint(bytes);
    switch (f.scalar) {
        case Scalar::Bool:
            Require(value <= 1, "Non-boolean protobuf value");
            return Value(value != 0);
        case Scalar::Enum:
            Require(value < 64 && (f.enum_values & (u64(1) << value)), "Unknown protobuf enum");
            return Value(value);
        case Scalar::UInt32:
            Require(value <= UINT32_MAX, "Overflow uint32");
            return Value(value);
        case Scalar::SInt32: {
            Require(value <= UINT32_MAX, "Overflow sint32");
            return Value(std::int64_t(static_cast<std::int32_t>((value >> 1) ^ u64(-std::int64_t(value & 1)))));
        }
        case Scalar::SInt64:
            return Value(std::bit_cast<std::int64_t>((value >> 1) ^ u64(-std::int64_t(value & 1))));
        case Scalar::Int32:
            return Value(std::int64_t(static_cast<std::int32_t>(value)));
        case Scalar::Int64:
            return Value(std::bit_cast<std::int64_t>(value));
        default:
            return Value(value);
    }
}
Object Parse(int schema, std::string_view bytes, unsigned depth, bool restore_defaults) {
    Require(depth <= 64, "Protobuf nesting too deep");
    Object out;
    std::set<unsigned> oneofs;
    while (!bytes.empty()) {
        const auto tag = Varint(bytes);
        Require((tag >> 3) > 0 && (tag >> 3) < (u64(1) << 29), "Invalid protobuf tag");
        const auto& f = Find(schema, unsigned(tag >> 3));
        const auto wire = unsigned(tag & 7);
        if (f.oneof)
            Require(oneofs.insert(f.oneof).second, "Duplicate protobuf oneof");
        if (f.repeated) {
            auto [it, added] = out.try_emplace(f.number, Value(List{}));
            auto& list = std::get<List>(it->second.data);
            if (wire == 2 && WireType(f.scalar) != 2) {
                auto packed = Data(bytes);
                while (!packed.empty())
                    list.push_back(ScalarValue(f, packed, WireType(f.scalar), depth));
            } else
                list.push_back(ScalarValue(f, bytes, wire, depth));
        } else {
            Require(!out.contains(f.number), "Duplicate singular protobuf field");
            // Property masks define the top-level value presence. Keep actual wire fields
            // here; nested messages still use ordinary protobuf default semantics.
            out.emplace(f.number, ScalarValue(f, bytes, wire, depth, schema != PropertyValueSchema));
        }
    }
    if (restore_defaults)
        for (const auto& f : Schemas.at(schema).fields)
            if (!out.contains(f.number) && !f.oneof && !f.optional && (f.repeated || f.scalar != Scalar::Message))
                out.emplace(f.number, f.repeated ? Value(List{}) : Default(f));
    return out;
}
void Encode(int schema, const Object& object, Writer& writer, unsigned depth);
void WriteScalar(const Field& f, const Value& value, Writer& writer, unsigned depth) {
    writer.Integer((u64(f.number) << 3) | WireType(f.scalar));
    if (f.scalar == Scalar::Message) {
        const auto* object = std::get_if<Object>(&value.data);
        Require(object, "Message expected");
        // 子消息直接写入同一缓冲。先占最小长度前缀，完成后只扩展实际需要的字节；
        // 全程保留offset，不跨递归持有可能因string扩容而失效的指针或引用。
        const auto prefix = writer.bytes.size();
        writer.Byte(0);
        const auto body = writer.bytes.size();
        Encode(f.child, *object, writer, depth + 1);
        auto size = writer.bytes.size() - body;
        std::array<char, 10> encoded{};
        unsigned count = 0;
        while (size >= 128) {
            encoded[count++] = char((size & 0x7f) | 0x80);
            size >>= 7;
        }
        encoded[count++] = char(size);
        Require(writer.bytes.size() <= writer.limit && count - 1 <= writer.limit - writer.bytes.size(),
                "Encoded message exceeds limit", ErrorCode::TooLarge);
        if (count > 1)
            writer.bytes.insert(prefix + 1, count - 1, '\0');
        for (unsigned i = 0; i < count; ++i)
            writer.bytes[prefix + i] = encoded[i];
        return;
    }
    if (f.scalar == Scalar::String || f.scalar == Scalar::Bytes) {
        const auto* text = std::get_if<std::string>(&value.data);
        Require(text, "String expected");
        Require(f.scalar == Scalar::Bytes || Utf8(*text), "Invalid UTF-8 protobuf string");
        writer.Integer(text->size());
        writer.Append(*text);
        return;
    }
    if (f.scalar == Scalar::Float || f.scalar == Scalar::Double) {
        const auto* real = std::get_if<double>(&value.data);
        Require(real && std::isfinite(*real), "Finite real expected");
        if (f.scalar == Scalar::Float) {
            const float converted = static_cast<float>(*real);
            Require(std::isfinite(converted), "Overflow float");
            writer.FixedValue(std::bit_cast<std::uint32_t>(converted), 4);
        } else
            writer.FixedValue(std::bit_cast<u64>(*real), 8);
        return;
    }
    if (f.scalar == Scalar::Bool) {
        const auto* v = std::get_if<bool>(&value.data);
        Require(v, "Boolean expected");
        writer.Integer(*v);
        return;
    }
    if (f.scalar == Scalar::Int32 || f.scalar == Scalar::Int64 || f.scalar == Scalar::SInt32 ||
        f.scalar == Scalar::SInt64 || f.scalar == Scalar::SFixed32 || f.scalar == Scalar::SFixed64) {
        const auto* v = std::get_if<std::int64_t>(&value.data);
        Require(v, "Signed integer expected");
        if (f.scalar == Scalar::Int32 || f.scalar == Scalar::SInt32 || f.scalar == Scalar::SFixed32)
            Require(*v >= INT32_MIN && *v <= INT32_MAX, "Overflow signed32");
        if (f.scalar == Scalar::SInt32 || f.scalar == Scalar::SInt64)
            writer.Integer((u64(*v) << 1) ^ u64(-(*v < 0)));
        else if (f.scalar == Scalar::SFixed32 || f.scalar == Scalar::SFixed64)
            writer.FixedValue(u64(*v), f.scalar == Scalar::SFixed32 ? 4 : 8);
        else
            writer.Integer(u64(*v));
        return;
    }
    const auto* v = std::get_if<u64>(&value.data);
    Require(v, "Unsigned integer expected");
    if (f.scalar == Scalar::UInt32 || f.scalar == Scalar::Fixed32)
        Require(*v <= UINT32_MAX, "Overflow unsigned32");
    if (f.scalar == Scalar::Enum)
        Require(*v < 64 && (f.enum_values & (u64(1) << *v)), "Unknown protobuf enum");
    if (f.scalar == Scalar::Fixed32 || f.scalar == Scalar::Fixed64)
        writer.FixedValue(*v, f.scalar == Scalar::Fixed32 ? 4 : 8);
    else
        writer.Integer(*v);
}
void Encode(int schema, const Object& object, Writer& writer, unsigned depth) {
    Require(depth <= 64, "Protobuf nesting too deep");
    std::set<unsigned> oneofs;
    for (const auto& [number, value] : object) {
        const auto& f = Find(schema, number);
        if (f.oneof)
            Require(oneofs.insert(f.oneof).second, "Duplicate protobuf oneof");
        if (f.repeated) {
            const auto* list = std::get_if<List>(&value.data);
            Require(list, "Repeated protobuf field expected");
            for (const auto& item : *list) {
                WriteScalar(f, item, writer, depth);
                diagnostics::CompletedCpuUnit();
            }
        } else if (f.optional || f.oneof || f.scalar == Scalar::Message || value != Default(f))
            WriteScalar(f, value, writer, depth);
        diagnostics::CompletedCpuUnit();
    }
}
} // namespace
std::string EncodeMessage(int schema, const Object& object, std::size_t maximum) {
    Require(maximum && maximum <= 16777216, "Invalid message size limit", ErrorCode::TooLarge);
    Writer writer(maximum);
    Encode(schema, object, writer, 0);
    return std::move(writer.bytes);
}
Object DecodeMessage(int schema, std::string_view bytes, std::size_t maximum) {
    Require(maximum && maximum <= 16777216 && bytes.size() <= maximum, "Message exceeds limit", ErrorCode::TooLarge);
    return Parse(schema, bytes, 0);
}
Object DeclaredPropertyFields(PropertyGroup group, const Object& input, u64 declared_fields) {
    Require(!(declared_fields & ~AllowedFields(group)), "Property mask outside group");
    const auto& arm = Find(PropertyValueSchema, PropertyGroupToWire(group));
    Require(arm.scalar == Scalar::Message, "Invalid property value schema");
    Object out = input;
    for (const auto& [number, value] : input) {
        Require(number && number <= 64 && (declared_fields & (u64(1) << (number - 1))),
                "Undeclared property value field");
        Find(arm.child, number);
    }
    for (unsigned number = 1; number <= 64; ++number) {
        if (!(declared_fields & (u64(1) << (number - 1))) || out.contains(number))
            continue;
        const auto& field = Find(arm.child, number);
        // An omitted implicit scalar/repeated field is its declared zero/empty value.
        // Explicit-presence fields cannot be reconstructed from the mask alone.
        Require(!field.optional && !field.oneof && (field.repeated || field.scalar != Scalar::Message),
                "Declared property value field missing");
        out.emplace(number, field.repeated ? Value(List{}) : Default(field));
    }
    return out;
}
u64 UInt(const Object& object, unsigned field) {
    const auto it = object.find(field);
    Require(it != object.end() && std::holds_alternative<u64>(it->second.data), "Missing unsigned field");
    return std::get<u64>(it->second.data);
}
std::int64_t Signed(const Object& object, unsigned field) {
    const auto it = object.find(field);
    Require(it != object.end() && std::holds_alternative<std::int64_t>(it->second.data), "Missing signed field");
    return std::get<std::int64_t>(it->second.data);
}
bool Boolean(const Object& object, unsigned field) {
    const auto it = object.find(field);
    Require(it != object.end() && std::holds_alternative<bool>(it->second.data), "Missing boolean field");
    return std::get<bool>(it->second.data);
}
std::string Text(const Object& object, unsigned field) {
    const auto it = object.find(field);
    Require(it != object.end() && std::holds_alternative<std::string>(it->second.data), "Missing string field");
    return std::get<std::string>(it->second.data);
}
const Object& Message(const Object& object, unsigned field) {
    const auto it = object.find(field);
    Require(it != object.end() && std::holds_alternative<Object>(it->second.data), "Missing message field");
    return std::get<Object>(it->second.data);
}
const List& Repeated(const Object& object, unsigned field) {
    const auto it = object.find(field);
    Require(it != object.end() && std::holds_alternative<List>(it->second.data), "Missing repeated field");
    return std::get<List>(it->second.data);
}
void Put(Object& o, unsigned f, u64 v) {
    o.emplace(f, Value(v));
}
void Put(Object& o, unsigned f, std::int64_t v) {
    o.emplace(f, Value(v));
}
void Put(Object& o, unsigned f, bool v) {
    o.emplace(f, Value(v));
}
void Put(Object& o, unsigned f, std::string v) {
    o.emplace(f, Value(std::move(v)));
}
void Put(Object& o, unsigned f, Object v) {
    o.emplace(f, Value(std::move(v)));
}
void Put(Object& o, unsigned f, List v) {
    o.emplace(f, Value(std::move(v)));
}
} // namespace monitor::synchronization::codec::detail
