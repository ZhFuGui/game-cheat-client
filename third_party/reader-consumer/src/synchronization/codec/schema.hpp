#pragma once
// CodecSchemaGenerator: protocol/generate_codec_schema.py
// CodecSchemaDescriptorSha256: 4e5f26fed497f86a37d686df443be54d42dcc5e1eeeae79143a19f209e5f6f92
// CodecSchemaSourceSha256: foundation/common.proto 172e0693708b05eebda66e5a1385ae8a5f683b59a0b51f229eb11daa4afa91d3
// CodecSchemaSourceSha256: synchronization/v3/consumer.proto c1cc58b424ea8f33281ba6b815af608f535f6fc48b42e72814209946accc1604
// CodecSchemaSourceSha256: synchronization/v3/debug.proto dc673e938216c8c61db942f98b3c3b76bf8ceebef761112bf84653ea9ea643d5
// CodecSchemaSourceSha256: synchronization/v3/model/consumer_values.proto 3a1b4048bbb13b333f40ac0e0c52f11ebc84a73210294da4d1b848e07f0afd25
// CodecSchemaSourceSha256: synchronization/v3/model/container.proto 48b96a8436078b8c8787ed4a522aa9beee32d5ff4e6728ab1ae2308c4b614187
// CodecSchemaSourceSha256: synchronization/v3/model/ground_loot.proto 40b2ef3101014d7a0f2d2cfffc2d9184e17dc2c5eb30f307d26527ef96135384
// CodecSchemaSourceSha256: synchronization/v3/model/living.proto ffd4091b8fb6e9e03260a6a203ef2509b4195e3d166563c1c40d20f95d0c42c7
// CodecSchemaSourceSha256: synchronization/v3/model/local.proto c9d2b060a7ce541880d791c6fc90f68190439ae7595d048f8e786572c6929b53
// CodecSchemaSourceSha256: synchronization/v3/model/projectile.proto 011c2dc2d26790b85cca08fed60d1fd0e7355ecfe6cff1204b234f3d7ae3c8b9
// CodecSchemaSourceSha256: synchronization/v3/model/shared.proto 70d70df54b3d7f7c0954f817d952a0e341b7c49573a52cef132a715b4163899d
// CodecSchemaSourceSha256: synchronization/v3/model/vehicle.proto f58d894f8e0c7c562e80a22eb3331d237e1f29351ee8ed7a557142c73d9aed72
// CodecSchemaSourceSha256: synchronization/v3/synchronization.proto e3a83a569168dee718eb3725f001d4185e0aefa3f6575abef0341dfea10d110e
// CodecSchemaShape: messages=161 fields=810
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
namespace monitor::synchronization::codec::detail {
// 来自官方protoc descriptor的固定v3字段表；不引protobuf运行库，字段改变必须重新生成并完成官方闭环验证。
enum class Scalar {
    Double,
    Float,
    Int64,
    UInt64,
    Int32,
    Fixed64,
    Fixed32,
    Bool,
    String,
    Message,
    Bytes,
    UInt32,
    Enum,
    SFixed32,
    SFixed64,
    SInt32,
    SInt64
};
struct Field {
    unsigned number;
    Scalar scalar;
    int child;
    bool optional, repeated;
    unsigned oneof;
    std::uint64_t enum_values;
};
struct Schema {
    std::string_view name;
    std::span<const Field> fields;
};
inline constexpr std::array Fields0{
    Field{1, Scalar::Float, -1, false, false, 0, 0ULL}, Field{2, Scalar::Float, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Float, -1, false, false, 0, 0ULL}, Field{4, Scalar::Float, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields1{
    Field{1, Scalar::Message, 0, false, false, 0, 0ULL}, Field{2, Scalar::Message, 2, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 2, false, false, 0, 0ULL},
};
inline constexpr std::array Fields2{
    Field{1, Scalar::Float, -1, false, false, 0, 0ULL}, Field{2, Scalar::Float, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Float, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields3{
    Field{1, Scalar::SInt32, -1, false, false, 0, 0ULL}, Field{2, Scalar::SInt32, -1, false, false, 0, 0ULL},
    Field{3, Scalar::SInt32, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields4{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{5, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{6, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{7, Scalar::UInt64, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields5{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{5, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{6, Scalar::Message, 76, false, false, 0, 0ULL},
    Field{7, Scalar::Message, 79, false, true, 0, 0ULL}, Field{8, Scalar::Message, 85, false, false, 0, 0ULL},
    Field{9, Scalar::Fixed64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields6{
    Field{1, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::SInt32, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Message, 117, false, false, 0, 0ULL}, Field{4, Scalar::Message, 1, false, false, 0, 0ULL},
    Field{5, Scalar::Message, 2, false, false, 0, 0ULL}, Field{6, Scalar::SInt32, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields7{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{5, Scalar::UInt64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields8{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::Enum, -1, false, false, 0, 7ULL},
    Field{3, Scalar::Double, -1, true, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields9{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::Enum, -1, false, false, 0, 255ULL},
    Field{3, Scalar::Message, 124, false, false, 0, 0ULL}, Field{4, Scalar::Double, -1, false, false, 0, 0ULL},
    Field{5, Scalar::String, -1, false, false, 0, 0ULL}, Field{6, Scalar::Double, -1, false, false, 0, 0ULL},
    Field{7, Scalar::Double, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields10{
    Field{1, Scalar::Message, 2, false, false, 0, 0ULL}, Field{2, Scalar::Message, 100, false, false, 0, 0ULL},
    Field{3, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{4, Scalar::Message, 13, false, false, 0, 0ULL},
};
inline constexpr std::array Fields11{
    Field{1, Scalar::Message, 137, false, false, 0, 0ULL}, Field{2, Scalar::Message, 6, false, true, 0, 0ULL},
};
inline constexpr std::array Fields12{
    Field{1, Scalar::SInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::Bool, -1, true, false, 0, 0ULL},
    Field{3, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{4, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{5, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{6, Scalar::UInt32, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields13{
    Field{1, Scalar::Message, 2, false, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Message, 100, false, false, 0, 0ULL}, Field{4, Scalar::Message, 100, false, false, 0, 0ULL},
    Field{5, Scalar::Message, 100, false, false, 0, 0ULL},
};
inline constexpr std::array Fields14{
    Field{1, Scalar::String, -1, false, false, 0, 0ULL}, Field{2, Scalar::Enum, -1, false, false, 0, 7ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields15{
    Field{1, Scalar::String, -1, false, false, 0, 0ULL}, Field{2, Scalar::Enum, -1, false, false, 0, 7ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{5, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{6, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{7, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{8, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{9, Scalar::UInt64, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields16{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::Fixed64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields17{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{5, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{6, Scalar::Fixed64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields18{
    Field{1, Scalar::Message, 49, true, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, true, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, true, false, 0, 0ULL}, Field{6, Scalar::Message, 62, true, false, 0, 0ULL},
    Field{7, Scalar::Float, -1, true, false, 0, 0ULL}, Field{8, Scalar::Float, -1, true, false, 0, 0ULL},
    Field{9, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{10, Scalar::Bool, -1, true, false, 0, 0ULL},
    Field{11, Scalar::Float, -1, true, false, 0, 0ULL}, Field{12, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{13, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{14, Scalar::Bool, -1, true, false, 0, 0ULL},
    Field{15, Scalar::String, -1, true, false, 0, 0ULL}, Field{16, Scalar::UInt64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields19{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{5, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{6, Scalar::Message, 20, false, false, 0, 0ULL},
    Field{7, Scalar::Message, 26, false, false, 0, 0ULL}, Field{8, Scalar::Message, 30, false, true, 0, 0ULL},
    Field{9, Scalar::Fixed64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields20{
    Field{1, Scalar::Message, 24, false, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Fixed64, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields21{
    Field{1, Scalar::Enum, -1, false, false, 0, 2097151ULL}, Field{5, Scalar::Message, 62, true, false, 0, 0ULL},
    Field{2, Scalar::Enum, -1, true, false, 0, 2097151ULL},
};
inline constexpr std::array Fields22{
    Field{1, Scalar::Message, 62, true, false, 0, 0ULL}, Field{2, Scalar::Message, 50, true, false, 0, 0ULL},
    Field{3, Scalar::Float, -1, true, false, 0, 0ULL}, Field{4, Scalar::Message, 45, true, false, 0, 0ULL},
    Field{5, Scalar::Float, -1, true, false, 0, 0ULL}, Field{6, Scalar::Float, -1, true, false, 0, 0ULL},
    Field{7, Scalar::Float, -1, true, false, 0, 0ULL}, Field{8, Scalar::UInt64, -1, true, false, 0, 0ULL},
    Field{9, Scalar::UInt64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields23{
    Field{1, Scalar::Bool, -1, true, false, 0, 0ULL}, Field{2, Scalar::Bool, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Message, 49, true, false, 0, 0ULL},
};
inline constexpr std::array Fields24{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{5, Scalar::UInt64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields25{
    Field{1, Scalar::UInt32, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields26{
    Field{1, Scalar::Message, 27, false, false, 0, 0ULL}, Field{2, Scalar::Message, 34, false, true, 0, 0ULL},
    Field{3, Scalar::Message, 31, false, true, 0, 0ULL},
};
inline constexpr std::array Fields27{
    Field{1, Scalar::UInt64, -1, true, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, true, false, 0, 0ULL},
    Field{3, Scalar::String, -1, true, false, 0, 0ULL}, Field{4, Scalar::String, -1, false, false, 0, 0ULL},
    Field{5, Scalar::Double, -1, true, false, 0, 0ULL}, Field{6, Scalar::Message, 45, false, false, 0, 0ULL},
    Field{7, Scalar::Message, 46, false, false, 0, 0ULL}, Field{8, Scalar::UInt64, -1, true, false, 0, 0ULL},
    Field{9, Scalar::Bool, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields28{
    Field{1, Scalar::UInt64, -1, true, false, 0, 0ULL}, Field{2, Scalar::Message, 50, true, false, 0, 0ULL},
};
inline constexpr std::array Fields29{
    Field{1, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields30{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 34, false, true, 0, 0ULL},
};
inline constexpr std::array Fields31{
    Field{1, Scalar::UInt32, -1, false, true, 0, 0ULL}, Field{2, Scalar::UInt32, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 51, false, false, 0, 0ULL}, Field{4, Scalar::Bool, -1, false, false, 0, 0ULL},
    Field{5, Scalar::UInt32, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields32{
    Field{1, Scalar::UInt32, -1, false, false, 0, 0ULL}, Field{2, Scalar::Message, 33, false, true, 0, 0ULL},
};
inline constexpr std::array Fields33{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::Message, 55, true, false, 0, 0ULL},
    Field{3, Scalar::Message, 38, true, false, 0, 0ULL}, Field{4, Scalar::Message, 57, true, false, 0, 0ULL},
    Field{5, Scalar::Message, 37, true, false, 0, 0ULL},
};
inline constexpr std::array Fields34{
    Field{1, Scalar::UInt32, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Bool, -1, false, false, 0, 0ULL}, Field{4, Scalar::Message, 51, false, false, 0, 0ULL},
    Field{5, Scalar::Message, 31, false, true, 0, 0ULL}, Field{6, Scalar::Message, 35, false, false, 0, 0ULL},
};
inline constexpr std::array Fields35{
    Field{1, Scalar::Message, 55, false, false, 1, 0ULL}, Field{2, Scalar::Message, 40, false, false, 1, 0ULL},
    Field{3, Scalar::Message, 44, false, false, 1, 0ULL}, Field{4, Scalar::Message, 42, false, false, 1, 0ULL},
    Field{5, Scalar::Message, 54, false, false, 1, 0ULL}, Field{6, Scalar::Message, 64, false, false, 1, 0ULL},
    Field{7, Scalar::Message, 53, false, false, 1, 0ULL}, Field{8, Scalar::Message, 63, false, false, 1, 0ULL},
    Field{9, Scalar::Message, 65, false, false, 1, 0ULL}, Field{10, Scalar::Message, 29, false, false, 1, 0ULL},
    Field{11, Scalar::Message, 25, false, false, 1, 0ULL}, Field{12, Scalar::Message, 38, false, false, 1, 0ULL},
    Field{13, Scalar::Message, 37, false, false, 1, 0ULL}, Field{14, Scalar::Message, 57, false, false, 1, 0ULL},
    Field{15, Scalar::Message, 32, false, false, 1, 0ULL}, Field{16, Scalar::Message, 41, false, false, 1, 0ULL},
    Field{17, Scalar::Message, 22, false, false, 1, 0ULL}, Field{18, Scalar::Message, 23, false, false, 1, 0ULL},
    Field{19, Scalar::Message, 66, false, false, 1, 0ULL}, Field{20, Scalar::Message, 56, false, false, 1, 0ULL},
    Field{21, Scalar::Message, 28, false, false, 1, 0ULL}, Field{22, Scalar::Message, 36, false, false, 1, 0ULL},
    Field{23, Scalar::Message, 58, false, false, 1, 0ULL}, Field{24, Scalar::Message, 18, false, false, 1, 0ULL},
};
inline constexpr std::array Fields36{
    Field{3, Scalar::Bool, -1, true, false, 0, 0ULL}, Field{4, Scalar::Float, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields37{
    Field{1, Scalar::UInt32, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields38{
    Field{1, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::String, -1, true, false, 0, 0ULL},
    Field{3, Scalar::String, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields39{
    Field{1, Scalar::UInt32, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, false, true, 0, 0ULL},
    Field{3, Scalar::UInt32, -1, false, true, 0, 0ULL},
};
inline constexpr std::array Fields40{
    Field{1, Scalar::SInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::Bool, -1, true, false, 0, 0ULL},
    Field{6, Scalar::UInt32, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields41{
    Field{1, Scalar::UInt64, -1, true, false, 0, 0ULL}, Field{2, Scalar::Message, 49, true, false, 0, 0ULL},
    Field{3, Scalar::Message, 49, true, false, 0, 0ULL},
};
inline constexpr std::array Fields42{
    Field{1, Scalar::Message, 62, true, false, 0, 0ULL}, Field{4, Scalar::Message, 68, true, false, 0, 0ULL},
};
inline constexpr std::array Fields43{
    Field{1, Scalar::Message, 45, true, false, 0, 0ULL}, Field{2, Scalar::String, -1, true, false, 0, 0ULL},
    Field{3, Scalar::String, -1, true, false, 0, 0ULL}, Field{6, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{10, Scalar::Message, 59, true, false, 0, 0ULL},
};
inline constexpr std::array Fields44{
    Field{1, Scalar::String, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields45{
    Field{1, Scalar::SInt32, -1, false, false, 0, 0ULL}, Field{2, Scalar::SInt32, -1, false, false, 0, 0ULL},
    Field{3, Scalar::SInt32, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields46{
    Field{1, Scalar::Double, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields47{
    Field{1, Scalar::Float, -1, false, false, 0, 0ULL}, Field{2, Scalar::Float, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Float, -1, false, false, 0, 0ULL}, Field{4, Scalar::Float, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields48{
    Field{1, Scalar::Float, -1, true, false, 0, 0ULL}, Field{2, Scalar::Float, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Float, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields49{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields50{
    Field{1, Scalar::Float, -1, false, false, 0, 0ULL}, Field{2, Scalar::Float, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Float, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields51{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields52{
    Field{1, Scalar::SInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt32, -1, false, false, 0, 0ULL}, Field{4, Scalar::Message, 49, false, true, 0, 0ULL},
};
inline constexpr std::array Fields53{
    Field{1, Scalar::UInt32, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{3, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{4, Scalar::Message, 52, false, true, 0, 0ULL},
};
inline constexpr std::array Fields54{
    Field{1, Scalar::Message, 45, true, false, 0, 0ULL}, Field{2, Scalar::Message, 21, false, true, 0, 0ULL},
    Field{3, Scalar::Message, 49, true, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, true, false, 0, 0ULL},
    Field{5, Scalar::UInt64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields55{
    Field{1, Scalar::Message, 62, true, false, 0, 0ULL}, Field{2, Scalar::Message, 45, true, false, 0, 0ULL},
};
inline constexpr std::array Fields56{
    Field{1, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::Message, 49, true, false, 0, 0ULL},
};
inline constexpr std::array Fields57{
    Field{1, Scalar::SInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::Float, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Float, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields58{
    Field{3, Scalar::Message, 50, true, false, 0, 0ULL},
};
inline constexpr std::array Fields59{
    Field{1, Scalar::Message, 47, false, false, 0, 0ULL}, Field{2, Scalar::Message, 62, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 62, false, false, 0, 0ULL},
};
inline constexpr std::array Fields60{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{5, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{6, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{7, Scalar::Message, 20, false, false, 0, 0ULL}, Field{8, Scalar::Message, 26, false, false, 0, 0ULL},
    Field{9, Scalar::Message, 30, false, true, 0, 0ULL}, Field{10, Scalar::Message, 30, false, true, 0, 0ULL},
    Field{11, Scalar::Message, 67, false, true, 0, 0ULL}, Field{12, Scalar::Message, 61, false, true, 0, 0ULL},
    Field{13, Scalar::Message, 61, false, true, 0, 0ULL}, Field{14, Scalar::Bool, -1, false, false, 0, 0ULL},
    Field{15, Scalar::Fixed64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields61{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Bool, -1, false, false, 0, 0ULL}, Field{4, Scalar::Message, 35, false, false, 0, 0ULL},
};
inline constexpr std::array Fields62{
    Field{1, Scalar::Float, -1, false, false, 0, 0ULL}, Field{2, Scalar::Float, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Float, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields63{
    Field{1, Scalar::String, -1, true, false, 0, 0ULL}, Field{2, Scalar::String, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields64{
    Field{1, Scalar::Float, -1, true, false, 0, 0ULL}, Field{2, Scalar::Float, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Float, -1, true, false, 0, 0ULL}, Field{4, Scalar::Float, -1, true, false, 0, 0ULL},
    Field{5, Scalar::Bool, -1, true, false, 0, 0ULL}, Field{6, Scalar::Bool, -1, true, false, 0, 0ULL},
    Field{7, Scalar::Bool, -1, true, false, 0, 0ULL}, Field{8, Scalar::Bool, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields65{
    Field{1, Scalar::UInt32, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields66{
    Field{1, Scalar::UInt64, -1, true, false, 0, 0ULL}, Field{2, Scalar::String, -1, true, false, 0, 0ULL},
    Field{3, Scalar::SInt32, -1, true, false, 0, 0ULL}, Field{4, Scalar::Message, 43, true, false, 0, 0ULL},
    Field{5, Scalar::Message, 48, true, false, 0, 0ULL}, Field{6, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{7, Scalar::Bool, -1, true, false, 0, 0ULL}, Field{8, Scalar::Message, 49, true, false, 0, 0ULL},
};
inline constexpr std::array Fields67{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields68{
    Field{1, Scalar::Message, 62, true, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Message, 49, true, false, 0, 0ULL},
};
inline constexpr std::array Fields69{
    Field{1, Scalar::Message, 100, false, false, 0, 0ULL},
};
inline constexpr std::array Fields70{
    Field{1, Scalar::Enum, -1, false, false, 0, 47ULL}, Field{2, Scalar::Enum, -1, false, false, 0, 127ULL},
    Field{3, Scalar::Fixed64, -1, false, false, 0, 0ULL}, Field{4, Scalar::Message, 71, false, false, 0, 0ULL},
    Field{5, Scalar::Message, 84, false, true, 0, 0ULL}, Field{6, Scalar::Message, 134, false, false, 0, 0ULL},
};
inline constexpr std::array Fields71{
    Field{1, Scalar::Message, 160, false, false, 1, 0ULL}, Field{2, Scalar::Message, 116, false, false, 1, 0ULL},
    Field{3, Scalar::Message, 74, false, false, 1, 0ULL}, Field{5, Scalar::Message, 124, false, false, 1, 0ULL},
};
inline constexpr std::array Fields72{
    Field{1, Scalar::Enum, -1, false, false, 0, 127ULL}, Field{2, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 73, false, false, 0, 0ULL}, Field{4, Scalar::Message, 84, false, true, 0, 0ULL},
    Field{5, Scalar::Message, 134, false, false, 0, 0ULL},
};
inline constexpr std::array Fields73{
    Field{1, Scalar::Message, 100, false, false, 0, 0ULL}, Field{2, Scalar::Message, 151, false, false, 0, 0ULL},
};
inline constexpr std::array Fields74{
    Field{1, Scalar::String, -1, true, false, 0, 0ULL}, Field{2, Scalar::Double, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Message, 3, false, false, 0, 0ULL}, Field{4, Scalar::String, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields75{
    Field{1, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{4, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{5, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{6, Scalar::String, -1, true, false, 0, 0ULL},
    Field{7, Scalar::Message, 117, false, false, 0, 0ULL},
};
inline constexpr std::array Fields76{
    Field{1, Scalar::Bool, -1, false, false, 0, 0ULL}, Field{2, Scalar::Message, 7, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 81, false, true, 0, 0ULL}, Field{6, Scalar::Enum, -1, false, false, 0, 127ULL},
    Field{7, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{8, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{9, Scalar::Message, 134, false, false, 0, 0ULL}, Field{10, Scalar::Message, 134, false, false, 0, 0ULL},
    Field{11, Scalar::Message, 142, false, false, 0, 0ULL}, Field{12, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{13, Scalar::Message, 134, false, false, 0, 0ULL}, Field{5, Scalar::Message, 139, false, false, 0, 0ULL},
};
inline constexpr std::array Fields77{
    Field{1, Scalar::Enum, -1, false, false, 0, 127ULL}, Field{2, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 78, false, false, 0, 0ULL}, Field{4, Scalar::Message, 84, false, true, 0, 0ULL},
    Field{5, Scalar::Message, 134, false, false, 0, 0ULL}, Field{6, Scalar::Message, 122, false, true, 0, 0ULL},
};
inline constexpr std::array Fields78{
    Field{1, Scalar::Message, 100, false, false, 0, 0ULL}, Field{2, Scalar::Message, 100, false, false, 0, 0ULL},
    Field{3, Scalar::SInt32, -1, true, false, 0, 0ULL}, Field{4, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{5, Scalar::Message, 115, false, false, 0, 0ULL}, Field{6, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{7, Scalar::Bool, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields79{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::Enum, -1, false, false, 0, 255ULL},
    Field{3, Scalar::Message, 100, false, false, 0, 0ULL}, Field{4, Scalar::Message, 128, false, true, 0, 0ULL},
};
inline constexpr std::array Fields80{
    Field{1, Scalar::String, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::Enum, -1, false, false, 0, 255ULL},
    Field{5, Scalar::Message, 100, false, false, 0, 0ULL},
};
inline constexpr std::array Fields81{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::Enum, -1, false, false, 0, 255ULL},
    Field{3, Scalar::Message, 100, false, false, 0, 0ULL}, Field{4, Scalar::Message, 95, false, true, 0, 0ULL},
};
inline constexpr std::array Fields82{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::Enum, -1, false, false, 0, 31ULL},
};
inline constexpr std::array Fields83{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::Enum, -1, false, false, 0, 255ULL},
    Field{3, Scalar::String, -1, false, false, 0, 0ULL}, Field{4, Scalar::Bool, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields84{
    Field{1, Scalar::UInt32, -1, false, false, 0, 0ULL}, Field{2, Scalar::Message, 134, false, false, 0, 0ULL},
    Field{3, Scalar::Bool, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt32, -1, false, false, 0, 0ULL},
    Field{5, Scalar::Enum, -1, false, false, 0, 127ULL},
};
inline constexpr std::array Fields85{
    Field{1, Scalar::Message, 70, false, true, 0, 0ULL}, Field{2, Scalar::Message, 114, false, false, 0, 0ULL},
};
inline constexpr std::array Fields86{
    Field{1, Scalar::Fixed64, -1, false, false, 0, 0ULL}, Field{2, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Fixed64, -1, false, false, 0, 0ULL}, Field{4, Scalar::SInt32, -1, false, false, 0, 0ULL},
    Field{5, Scalar::SInt32, -1, false, false, 0, 0ULL}, Field{6, Scalar::SInt32, -1, false, false, 0, 0ULL},
    Field{7, Scalar::SInt32, -1, false, false, 0, 0ULL}, Field{8, Scalar::SInt32, -1, false, false, 0, 0ULL},
    Field{9, Scalar::SInt32, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields87{
    Field{1, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{3, Scalar::String, -1, true, false, 0, 0ULL}, Field{4, Scalar::Message, 117, false, false, 0, 0ULL},
};
inline constexpr std::array Fields88{
    Field{1, Scalar::Message, 93, false, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{3, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{4, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{5, Scalar::Message, 86, false, false, 0, 0ULL}, Field{6, Scalar::Message, 86, false, false, 0, 0ULL},
};
inline constexpr std::array Fields89{
    Field{1, Scalar::Message, 105, false, false, 0, 0ULL}, Field{2, Scalar::Message, 106, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 104, false, false, 0, 0ULL}, Field{4, Scalar::Message, 103, false, false, 0, 0ULL},
};
inline constexpr std::array Fields90{
    Field{1, Scalar::Fixed64, -1, false, false, 0, 0ULL}, Field{2, Scalar::SInt32, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Fixed64, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields91{
    Field{1, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{2, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{3, Scalar::String, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields92{
    Field{1, Scalar::Message, 100, false, false, 0, 0ULL}, Field{2, Scalar::Message, 90, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 91, false, false, 0, 0ULL}, Field{4, Scalar::Message, 140, false, false, 0, 0ULL},
    Field{5, Scalar::Message, 89, false, false, 0, 0ULL},
};
inline constexpr std::array Fields93{
    Field{1, Scalar::Message, 92, false, true, 0, 0ULL}, Field{2, Scalar::UInt32, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 94, false, true, 0, 0ULL},
};
inline constexpr std::array Fields94{
    Field{1, Scalar::SInt32, -1, false, false, 0, 0ULL}, Field{2, Scalar::Bool, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Enum, -1, false, false, 0, 255ULL},
};
inline constexpr std::array Fields95{
    Field{1, Scalar::Enum, -1, false, false, 0, 3669999ULL}, Field{2, Scalar::Enum, -1, false, false, 0, 127ULL},
    Field{3, Scalar::Fixed64, -1, false, false, 0, 0ULL}, Field{4, Scalar::Message, 134, false, false, 0, 0ULL},
    Field{5, Scalar::Message, 122, false, true, 0, 0ULL}, Field{6, Scalar::Message, 134, false, false, 0, 0ULL},
};
inline constexpr std::array Fields96{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::Bool, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields97{
    Field{13, Scalar::Enum, -1, false, false, 0, 7ULL}, Field{14, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{1, Scalar::UInt32, -1, false, false, 0, 0ULL}, Field{2, Scalar::String, -1, false, false, 0, 0ULL},
    Field{4, Scalar::Message, 143, false, false, 0, 0ULL}, Field{6, Scalar::Message, 107, false, true, 0, 0ULL},
    Field{7, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{8, Scalar::String, -1, false, false, 0, 0ULL},
    Field{9, Scalar::Enum, -1, false, false, 0, 127ULL}, Field{10, Scalar::Double, -1, false, false, 0, 0ULL},
    Field{11, Scalar::Message, 14, true, false, 0, 0ULL}, Field{12, Scalar::UInt32, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields98{
    Field{15, Scalar::Enum, -1, false, false, 0, 7ULL}, Field{16, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{17, Scalar::Message, 39, false, true, 0, 0ULL}, Field{1, Scalar::UInt32, -1, false, false, 0, 0ULL},
    Field{2, Scalar::String, -1, false, false, 0, 0ULL}, Field{3, Scalar::String, -1, false, false, 0, 0ULL},
    Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{5, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{6, Scalar::Message, 143, false, false, 0, 0ULL}, Field{7, Scalar::Message, 15, false, false, 0, 0ULL},
    Field{8, Scalar::Message, 107, false, true, 0, 0ULL}, Field{9, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{10, Scalar::String, -1, false, false, 0, 0ULL}, Field{11, Scalar::Enum, -1, false, false, 0, 127ULL},
    Field{12, Scalar::Message, 124, false, false, 0, 0ULL}, Field{13, Scalar::UInt32, -1, false, false, 0, 0ULL},
    Field{14, Scalar::Message, 14, true, false, 0, 0ULL},
};
inline constexpr std::array Fields99{
    Field{1, Scalar::Message, 100, false, false, 0, 0ULL}, Field{2, Scalar::Enum, -1, false, false, 0, 255ULL},
    Field{3, Scalar::Message, 138, false, true, 0, 0ULL},
};
inline constexpr std::array Fields100{
    Field{1, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{2, Scalar::SInt32, -1, true, false, 0, 0ULL},
    Field{3, Scalar::UInt32, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields101{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{5, Scalar::Enum, -1, false, false, 0, 251ULL}, Field{6, Scalar::UInt64, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields102{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{5, Scalar::UInt64, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields103{
    Field{1, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{3, Scalar::String, -1, true, false, 0, 0ULL}, Field{4, Scalar::String, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields104{
    Field{1, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::Message, 117, false, false, 0, 0ULL},
    Field{3, Scalar::String, -1, true, false, 0, 0ULL}, Field{4, Scalar::Fixed64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields105{
    Field{1, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{2, Scalar::SInt32, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields106{
    Field{1, Scalar::SInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::Float, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Float, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields107{
    Field{1, Scalar::Enum, -1, false, false, 0, 255ULL}, Field{2, Scalar::Enum, -1, false, true, 0, 3669999ULL},
    Field{3, Scalar::Enum, -1, false, true, 0, 3669999ULL},
};
inline constexpr std::array Fields108{
    Field{1, Scalar::Enum, -1, false, false, 0, 127ULL}, Field{2, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 109, false, false, 0, 0ULL}, Field{4, Scalar::Message, 84, false, true, 0, 0ULL},
    Field{5, Scalar::Message, 134, false, false, 0, 0ULL},
};
inline constexpr std::array Fields109{
    Field{1, Scalar::Message, 100, false, false, 0, 0ULL}, Field{2, Scalar::Message, 100, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 100, false, false, 0, 0ULL}, Field{4, Scalar::Message, 100, false, false, 0, 0ULL},
    Field{5, Scalar::Message, 100, false, false, 0, 0ULL}, Field{6, Scalar::Message, 2, false, false, 0, 0ULL},
    Field{7, Scalar::Float, -1, true, false, 0, 0ULL}, Field{8, Scalar::Float, -1, true, false, 0, 0ULL},
    Field{9, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{10, Scalar::Bool, -1, true, false, 0, 0ULL},
    Field{11, Scalar::Float, -1, true, false, 0, 0ULL}, Field{12, Scalar::Enum, -1, true, false, 0, 7ULL},
    Field{13, Scalar::Enum, -1, true, false, 0, 3ULL}, Field{14, Scalar::Bool, -1, true, false, 0, 0ULL},
    Field{15, Scalar::String, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields110{
    Field{1, Scalar::Enum, -1, false, false, 0, 127ULL}, Field{2, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 111, false, false, 0, 0ULL}, Field{4, Scalar::Message, 84, false, true, 0, 0ULL},
    Field{5, Scalar::Message, 134, false, false, 0, 0ULL},
};
inline constexpr std::array Fields111{
    Field{1, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{2, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{4, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{5, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{6, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{7, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{8, Scalar::Message, 2, false, false, 0, 0ULL},
    Field{9, Scalar::UInt64, -1, true, false, 0, 0ULL}, Field{10, Scalar::UInt64, -1, true, false, 0, 0ULL},
    Field{11, Scalar::Message, 80, false, false, 0, 0ULL}, Field{12, Scalar::Message, 80, false, false, 0, 0ULL},
};
inline constexpr std::array Fields112{
    Field{1, Scalar::Enum, -1, false, false, 0, 127ULL}, Field{2, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 113, false, false, 0, 0ULL}, Field{4, Scalar::Message, 84, false, true, 0, 0ULL},
    Field{5, Scalar::Message, 134, false, false, 0, 0ULL},
};
inline constexpr std::array Fields113{
    Field{1, Scalar::Message, 100, false, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields114{
    Field{1, Scalar::Message, 110, false, false, 0, 0ULL}, Field{2, Scalar::Message, 154, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 77, false, false, 0, 0ULL}, Field{4, Scalar::Message, 112, false, false, 0, 0ULL},
    Field{5, Scalar::Message, 72, false, false, 0, 0ULL}, Field{6, Scalar::Message, 118, false, false, 0, 0ULL},
    Field{7, Scalar::Message, 120, false, false, 0, 0ULL}, Field{8, Scalar::Message, 108, false, false, 0, 0ULL},
};
inline constexpr std::array Fields115{
    Field{1, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{2, Scalar::Message, 117, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 100, false, false, 0, 0ULL}, Field{4, Scalar::Message, 157, false, false, 0, 0ULL},
    Field{5, Scalar::Message, 158, false, false, 0, 0ULL}, Field{6, Scalar::Message, 159, false, false, 0, 0ULL},
};
inline constexpr std::array Fields116{
    Field{1, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{2, Scalar::Message, 117, false, false, 0, 0ULL},
    Field{3, Scalar::Bool, -1, true, false, 0, 0ULL}, Field{4, Scalar::String, -1, true, false, 0, 0ULL},
    Field{5, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{6, Scalar::String, -1, true, false, 0, 0ULL},
    Field{7, Scalar::String, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields117{
    Field{1, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{3, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{4, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{5, Scalar::String, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields118{
    Field{1, Scalar::Enum, -1, false, false, 0, 127ULL}, Field{2, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 119, false, false, 0, 0ULL}, Field{4, Scalar::Message, 84, false, true, 0, 0ULL},
    Field{5, Scalar::Message, 134, false, false, 0, 0ULL},
};
inline constexpr std::array Fields119{
    Field{1, Scalar::Message, 100, false, false, 0, 0ULL}, Field{2, Scalar::Message, 100, false, false, 0, 0ULL},
    Field{3, Scalar::Bool, -1, true, false, 0, 0ULL}, Field{4, Scalar::Float, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields120{
    Field{1, Scalar::Enum, -1, false, false, 0, 127ULL}, Field{2, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 121, false, false, 0, 0ULL}, Field{4, Scalar::Message, 84, false, true, 0, 0ULL},
    Field{5, Scalar::Message, 134, false, false, 0, 0ULL},
};
inline constexpr std::array Fields121{
    Field{1, Scalar::Message, 100, false, false, 0, 0ULL}, Field{2, Scalar::Message, 100, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 151, false, false, 0, 0ULL},
};
inline constexpr std::array Fields122{
    Field{1, Scalar::UInt32, -1, false, true, 0, 0ULL}, Field{2, Scalar::Message, 123, false, false, 0, 0ULL},
};
inline constexpr std::array Fields123{
    Field{1, Scalar::Message, 134, false, false, 0, 0ULL}, Field{2, Scalar::Message, 134, false, false, 0, 0ULL},
    Field{3, Scalar::Enum, -1, true, false, 0, 63ULL}, Field{4, Scalar::Enum, -1, false, false, 0, 63ULL},
};
inline constexpr std::array Fields124{
    Field{1, Scalar::UInt64, -1, true, false, 0, 0ULL}, Field{2, Scalar::Double, -1, true, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields125{
    Field{1, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{2, Scalar::String, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Message, 117, false, false, 0, 0ULL},
};
inline constexpr std::array Fields126{
    Field{1, Scalar::Message, 100, false, false, 0, 0ULL},
};
inline constexpr std::array Fields127{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::Enum, -1, false, false, 0, 3669999ULL},
    Field{3, Scalar::Enum, -1, false, false, 0, 15ULL}, Field{4, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{5, Scalar::Fixed64, -1, false, false, 0, 0ULL}, Field{6, Scalar::Message, 129, false, false, 0, 0ULL},
    Field{7, Scalar::Message, 134, false, false, 0, 0ULL},
};
inline constexpr std::array Fields128{
    Field{1, Scalar::Enum, -1, false, false, 0, 3669999ULL}, Field{2, Scalar::Enum, -1, false, false, 0, 127ULL},
    Field{3, Scalar::Fixed64, -1, false, false, 0, 0ULL}, Field{4, Scalar::Message, 129, false, false, 0, 0ULL},
    Field{5, Scalar::Message, 134, false, false, 0, 0ULL}, Field{6, Scalar::Message, 134, false, false, 0, 0ULL},
    Field{7, Scalar::Message, 122, false, true, 0, 0ULL},
};
inline constexpr std::array Fields129{
    Field{1, Scalar::Message, 99, false, false, 1, 0ULL}, Field{2, Scalar::Message, 140, false, false, 1, 0ULL},
    Field{3, Scalar::Message, 12, false, false, 1, 0ULL}, Field{5, Scalar::Message, 141, false, false, 1, 0ULL},
    Field{6, Scalar::Message, 148, false, false, 1, 0ULL}, Field{7, Scalar::Message, 147, false, false, 1, 0ULL},
    Field{8, Scalar::Message, 117, false, false, 1, 0ULL}, Field{9, Scalar::Message, 146, false, false, 1, 0ULL},
    Field{10, Scalar::Message, 105, false, false, 1, 0ULL}, Field{11, Scalar::Message, 106, false, false, 1, 0ULL},
    Field{12, Scalar::Message, 104, false, false, 1, 0ULL}, Field{13, Scalar::Message, 145, false, false, 1, 0ULL},
    Field{14, Scalar::Message, 125, false, false, 1, 0ULL}, Field{15, Scalar::Message, 75, false, false, 1, 0ULL},
    Field{16, Scalar::Message, 103, false, false, 1, 0ULL}, Field{17, Scalar::Message, 87, false, false, 1, 0ULL},
    Field{18, Scalar::Message, 88, false, false, 1, 0ULL}, Field{20, Scalar::Message, 11, false, false, 1, 0ULL},
    Field{21, Scalar::Message, 10, false, false, 1, 0ULL},
};
inline constexpr std::array Fields130{
    Field{1, Scalar::UInt32, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Enum, -1, false, false, 0, 7ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{5, Scalar::Message, 97, false, false, 1, 0ULL}, Field{6, Scalar::Message, 98, false, false, 1, 0ULL},
    Field{7, Scalar::Message, 5, false, false, 1, 0ULL}, Field{8, Scalar::Message, 4, false, false, 1, 0ULL},
    Field{9, Scalar::Message, 131, false, false, 1, 0ULL}, Field{10, Scalar::Message, 101, false, false, 1, 0ULL},
    Field{11, Scalar::Message, 144, false, false, 1, 0ULL}, Field{12, Scalar::Message, 8, false, false, 1, 0ULL},
    Field{13, Scalar::Message, 9, false, false, 1, 0ULL}, Field{14, Scalar::Message, 96, false, false, 1, 0ULL},
    Field{15, Scalar::Message, 83, false, false, 1, 0ULL}, Field{16, Scalar::Message, 102, false, false, 1, 0ULL},
    Field{17, Scalar::Message, 16, false, false, 1, 0ULL}, Field{19, Scalar::Message, 19, false, false, 1, 0ULL},
    Field{20, Scalar::Message, 60, false, false, 1, 0ULL}, Field{18, Scalar::Message, 17, false, false, 1, 0ULL},
};
inline constexpr std::array Fields131{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{5, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{6, Scalar::Enum, -1, false, false, 0, 251ULL},
};
inline constexpr std::array Fields132{
    Field{1, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{2, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{3, Scalar::SInt32, -1, true, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, true, false, 0, 0ULL},
    Field{5, Scalar::Message, 80, false, false, 0, 0ULL},
};
inline constexpr std::array Fields133{
    Field{1, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{3, Scalar::UInt32, -1, false, false, 0, 0ULL}, Field{4, Scalar::Message, 132, false, true, 0, 0ULL},
};
inline constexpr std::array Fields134{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields135{
    Field{1, Scalar::SInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{3, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{4, Scalar::Message, 133, false, false, 0, 0ULL},
};
inline constexpr std::array Fields136{
    Field{1, Scalar::Message, 135, false, true, 0, 0ULL}, Field{2, Scalar::UInt32, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields137{
    Field{1, Scalar::Message, 100, false, false, 0, 0ULL}, Field{2, Scalar::Message, 100, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 100, false, false, 0, 0ULL}, Field{4, Scalar::UInt32, -1, false, false, 0, 0ULL},
    Field{5, Scalar::UInt32, -1, false, false, 0, 0ULL}, Field{6, Scalar::UInt32, -1, false, false, 0, 0ULL},
    Field{7, Scalar::Message, 1, false, false, 0, 0ULL}, Field{8, Scalar::Message, 3, false, false, 0, 0ULL},
};
inline constexpr std::array Fields138{
    Field{1, Scalar::UInt32, -1, false, false, 0, 0ULL}, Field{2, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Fixed64, -1, false, false, 0, 0ULL}, Field{4, Scalar::SInt32, -1, true, false, 0, 0ULL},
    Field{5, Scalar::Fixed64, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields139{
    Field{1, Scalar::Bool, -1, false, false, 0, 0ULL}, Field{2, Scalar::Bool, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Fixed64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{5, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{6, Scalar::Message, 134, false, false, 0, 0ULL},
    Field{7, Scalar::Message, 134, false, false, 0, 0ULL}, Field{8, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{9, Scalar::Fixed64, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields140{
    Field{1, Scalar::Message, 2, false, false, 0, 0ULL}, Field{2, Scalar::Message, 3, false, false, 0, 0ULL},
    Field{3, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{4, Scalar::Fixed64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields141{
    Field{1, Scalar::String, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields142{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{5, Scalar::UInt64, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields143{
    Field{2, Scalar::UInt32, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt32, -1, false, false, 0, 0ULL},
    Field{5, Scalar::UInt32, -1, false, false, 0, 0ULL}, Field{6, Scalar::UInt32, -1, false, false, 0, 0ULL},
    Field{7, Scalar::UInt32, -1, false, false, 0, 0ULL}, Field{9, Scalar::UInt32, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields144{
    Field{1, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{2, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{4, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{5, Scalar::UInt64, -1, false, false, 0, 0ULL}, Field{6, Scalar::UInt64, -1, false, false, 0, 0ULL},
    Field{7, Scalar::Message, 76, false, false, 0, 0ULL}, Field{8, Scalar::Message, 79, false, true, 0, 0ULL},
    Field{9, Scalar::Message, 82, false, true, 0, 0ULL}, Field{10, Scalar::Message, 127, false, true, 0, 0ULL},
    Field{11, Scalar::Message, 85, false, false, 0, 0ULL}, Field{12, Scalar::Fixed64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields145{
    Field{1, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{2, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{3, Scalar::String, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields146{
    Field{1, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{2, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Message, 117, false, false, 0, 0ULL}, Field{4, Scalar::String, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields147{
    Field{1, Scalar::Message, 136, false, false, 0, 0ULL}, Field{2, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{3, Scalar::UInt32, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields148{
    Field{1, Scalar::Float, -1, true, false, 0, 0ULL}, Field{2, Scalar::Float, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Float, -1, true, false, 0, 0ULL}, Field{4, Scalar::Float, -1, true, false, 0, 0ULL},
    Field{5, Scalar::Bool, -1, true, false, 0, 0ULL}, Field{6, Scalar::Bool, -1, true, false, 0, 0ULL},
    Field{7, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{8, Scalar::Fixed64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields149{
    Field{1, Scalar::Enum, -1, false, false, 0, 127ULL}, Field{2, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 150, false, false, 0, 0ULL}, Field{4, Scalar::Message, 84, false, true, 0, 0ULL},
    Field{5, Scalar::Message, 134, false, false, 0, 0ULL},
};
inline constexpr std::array Fields150{
    Field{1, Scalar::Message, 2, false, false, 0, 0ULL}, Field{2, Scalar::Message, 151, false, false, 0, 0ULL},
    Field{3, Scalar::Float, -1, true, false, 0, 0ULL}, Field{4, Scalar::Message, 3, false, false, 0, 0ULL},
    Field{5, Scalar::Float, -1, true, false, 0, 0ULL}, Field{6, Scalar::Float, -1, true, false, 0, 0ULL},
    Field{7, Scalar::Float, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields151{
    Field{1, Scalar::Float, -1, false, false, 0, 0ULL}, Field{2, Scalar::Float, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Float, -1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields152{
    Field{1, Scalar::Enum, -1, false, false, 0, 127ULL}, Field{2, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 153, false, false, 0, 0ULL}, Field{4, Scalar::Message, 84, false, true, 0, 0ULL},
    Field{5, Scalar::Message, 134, false, false, 0, 0ULL},
};
inline constexpr std::array Fields153{
    Field{1, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{2, Scalar::Message, 100, false, false, 0, 0ULL},
    Field{3, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{4, Scalar::Message, 100, false, false, 0, 0ULL},
};
inline constexpr std::array Fields154{
    Field{1, Scalar::Message, 152, false, false, 0, 0ULL}, Field{2, Scalar::Message, 149, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 155, false, false, 0, 0ULL},
};
inline constexpr std::array Fields155{
    Field{1, Scalar::Enum, -1, false, false, 0, 127ULL}, Field{2, Scalar::Fixed64, -1, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 156, false, false, 0, 0ULL}, Field{4, Scalar::Message, 84, false, true, 0, 0ULL},
    Field{5, Scalar::Message, 134, false, false, 0, 0ULL},
};
inline constexpr std::array Fields156{
    Field{1, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{2, Scalar::Message, 100, false, false, 0, 0ULL},
    Field{3, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{4, Scalar::Fixed64, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields157{
    Field{1, Scalar::Message, 137, false, false, 0, 0ULL}, Field{2, Scalar::Message, 117, false, false, 0, 0ULL},
    Field{3, Scalar::Message, 117, false, false, 0, 0ULL}, Field{4, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{5, Scalar::UInt32, -1, true, false, 0, 0ULL}, Field{6, Scalar::UInt32, -1, true, false, 0, 0ULL},
    Field{7, Scalar::Message, 100, false, false, 0, 0ULL}, Field{8, Scalar::Message, 1, false, false, 0, 0ULL},
    Field{9, Scalar::Message, 1, false, false, 0, 0ULL}, Field{10, Scalar::Message, 1, false, false, 0, 0ULL},
};
inline constexpr std::array Fields158{
    Field{1, Scalar::Float, -1, true, false, 0, 0ULL}, Field{2, Scalar::Float, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Float, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields159{
    Field{1, Scalar::UInt32, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Fields160{
    Field{1, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{2, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{3, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{4, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{5, Scalar::Fixed64, -1, true, false, 0, 0ULL}, Field{6, Scalar::Fixed64, -1, true, false, 0, 0ULL},
    Field{7, Scalar::Float, -1, true, false, 0, 0ULL}, Field{8, Scalar::SInt32, -1, true, false, 0, 0ULL},
    Field{9, Scalar::Bool, -1, true, false, 0, 0ULL}, Field{10, Scalar::Bool, -1, true, false, 0, 0ULL},
};
inline constexpr std::array Schemas{
    Schema{"pubg.reader.foundation.Quaternion", Fields0}, Schema{"pubg.reader.foundation.Transform", Fields1},
    Schema{"pubg.reader.foundation.Vec3", Fields2}, Schema{"pubg.reader.foundation.WorldOrigin", Fields3},
    Schema{"pubg.reader.synchronization.v3.AppliedAck", Fields4},
    Schema{"pubg.reader.synchronization.v3.Baseline", Fields5}, Schema{"pubg.reader.synchronization.v3.Bone", Fields6},
    Schema{"pubg.reader.synchronization.v3.CaptureInterval", Fields7},
    Schema{"pubg.reader.synchronization.v3.CaptureRateRequest", Fields8},
    Schema{"pubg.reader.synchronization.v3.CaptureRateResponse", Fields9},
    Schema{"pubg.reader.synchronization.v3.CharacterMovementValue", Fields10},
    Schema{"pubg.reader.synchronization.v3.CharacterSkeletonValue", Fields11},
    Schema{"pubg.reader.synchronization.v3.CharacterStateValue", Fields12},
    Schema{"pubg.reader.synchronization.v3.CharacterWorldVelocityValue", Fields13},
    Schema{"pubg.reader.synchronization.v3.ClockIdentity", Fields14},
    Schema{"pubg.reader.synchronization.v3.ClockMapping", Fields15},
    Schema{"pubg.reader.synchronization.v3.ClockProbe", Fields16},
    Schema{"pubg.reader.synchronization.v3.ClockProbeReply", Fields17},
    Schema{"pubg.reader.synchronization.v3.ConsumerAimBasis", Fields18},
    Schema{"pubg.reader.synchronization.v3.ConsumerBaseline", Fields19},
    Schema{"pubg.reader.synchronization.v3.ConsumerBatch", Fields20},
    Schema{"pubg.reader.synchronization.v3.ConsumerBone", Fields21},
    Schema{"pubg.reader.synchronization.v3.ConsumerCamera", Fields22},
    Schema{"pubg.reader.synchronization.v3.ConsumerCameraTarget", Fields23},
    Schema{"pubg.reader.synchronization.v3.ConsumerCapture", Fields24},
    Schema{"pubg.reader.synchronization.v3.ConsumerContainerType", Fields25},
    Schema{"pubg.reader.synchronization.v3.ConsumerContext", Fields26},
    Schema{"pubg.reader.synchronization.v3.ConsumerContextValues", Fields27},
    Schema{"pubg.reader.synchronization.v3.ConsumerControllerRotation", Fields28},
    Schema{"pubg.reader.synchronization.v3.ConsumerCreatureType", Fields29},
    Schema{"pubg.reader.synchronization.v3.ConsumerEntity", Fields30},
    Schema{"pubg.reader.synchronization.v3.ConsumerFieldSource", Fields31},
    Schema{"pubg.reader.synchronization.v3.ConsumerGroundContents", Fields32},
    Schema{"pubg.reader.synchronization.v3.ConsumerGroundMember", Fields33},
    Schema{"pubg.reader.synchronization.v3.ConsumerGroupRecord", Fields34},
    Schema{"pubg.reader.synchronization.v3.ConsumerGroupValue", Fields35},
    Schema{"pubg.reader.synchronization.v3.ConsumerHolding", Fields36},
    Schema{"pubg.reader.synchronization.v3.ConsumerItemClassification", Fields37},
    Schema{"pubg.reader.synchronization.v3.ConsumerItemDefinition", Fields38},
    Schema{"pubg.reader.synchronization.v3.ConsumerKindContract", Fields39},
    Schema{"pubg.reader.synchronization.v3.ConsumerLivingState", Fields40},
    Schema{"pubg.reader.synchronization.v3.ConsumerLocalControl", Fields41},
    Schema{"pubg.reader.synchronization.v3.ConsumerMovement", Fields42},
    Schema{"pubg.reader.synchronization.v3.ConsumerMuzzle", Fields43},
    Schema{"pubg.reader.synchronization.v3.ConsumerName", Fields44},
    Schema{"pubg.reader.synchronization.v3.ConsumerOrigin", Fields45},
    Schema{"pubg.reader.synchronization.v3.ConsumerPacing", Fields46},
    Schema{"pubg.reader.synchronization.v3.ConsumerQuaternion", Fields47},
    Schema{"pubg.reader.synchronization.v3.ConsumerRecoil", Fields48},
    Schema{"pubg.reader.synchronization.v3.ConsumerReference", Fields49},
    Schema{"pubg.reader.synchronization.v3.ConsumerRotation", Fields50},
    Schema{"pubg.reader.synchronization.v3.ConsumerSample", Fields51},
    Schema{"pubg.reader.synchronization.v3.ConsumerSeat", Fields52},
    Schema{"pubg.reader.synchronization.v3.ConsumerSeats", Fields53},
    Schema{"pubg.reader.synchronization.v3.ConsumerSkeleton", Fields54},
    Schema{"pubg.reader.synchronization.v3.ConsumerSpatial", Fields55},
    Schema{"pubg.reader.synchronization.v3.ConsumerSpectators", Fields56},
    Schema{"pubg.reader.synchronization.v3.ConsumerStack", Fields57},
    Schema{"pubg.reader.synchronization.v3.ConsumerSway", Fields58},
    Schema{"pubg.reader.synchronization.v3.ConsumerTransform", Fields59},
    Schema{"pubg.reader.synchronization.v3.ConsumerUpdate", Fields60},
    Schema{"pubg.reader.synchronization.v3.ConsumerValueChange", Fields61},
    Schema{"pubg.reader.synchronization.v3.ConsumerVector", Fields62},
    Schema{"pubg.reader.synchronization.v3.ConsumerVehicleModel", Fields63},
    Schema{"pubg.reader.synchronization.v3.ConsumerVehicleState", Fields64},
    Schema{"pubg.reader.synchronization.v3.ConsumerVehicleType", Fields65},
    Schema{"pubg.reader.synchronization.v3.ConsumerWeapon", Fields66},
    Schema{"pubg.reader.synchronization.v3.ConsumerWithdrawal", Fields67},
    Schema{"pubg.reader.synchronization.v3.ConsumerWorldVelocity", Fields68},
    Schema{"pubg.reader.synchronization.v3.ContainerValue", Fields69},
    Schema{"pubg.reader.synchronization.v3.ContextState", Fields70},
    Schema{"pubg.reader.synchronization.v3.ContextValue", Fields71},
    Schema{"pubg.reader.synchronization.v3.ControllerRotationState", Fields72},
    Schema{"pubg.reader.synchronization.v3.ControllerRotationValue", Fields73},
    Schema{"pubg.reader.synchronization.v3.CoordinatesValue", Fields74},
    Schema{"pubg.reader.synchronization.v3.CreatureClassificationValue", Fields75},
    Schema{"pubg.reader.synchronization.v3.CurrentVerification", Fields76},
    Schema{"pubg.reader.synchronization.v3.CurrentWeaponState", Fields77},
    Schema{"pubg.reader.synchronization.v3.CurrentWeaponValue", Fields78},
    Schema{"pubg.reader.synchronization.v3.EntityRecord", Fields79},
    Schema{"pubg.reader.synchronization.v3.EntityReference", Fields80},
    Schema{"pubg.reader.synchronization.v3.EntityVerification", Fields81},
    Schema{"pubg.reader.synchronization.v3.EntityWithdrawal", Fields82},
    Schema{"pubg.reader.synchronization.v3.ErrorResponse", Fields83},
    Schema{"pubg.reader.synchronization.v3.FieldSource", Fields84},
    Schema{"pubg.reader.synchronization.v3.FrameContext", Fields85},
    Schema{"pubg.reader.synchronization.v3.GroundCollectionSourceValue", Fields86},
    Schema{"pubg.reader.synchronization.v3.GroundGroupClassificationValue", Fields87},
    Schema{"pubg.reader.synchronization.v3.GroundGroupContentsValue", Fields88},
    Schema{"pubg.reader.synchronization.v3.GroundItemValue", Fields89},
    Schema{"pubg.reader.synchronization.v3.GroundMemberAssociationValue", Fields90},
    Schema{"pubg.reader.synchronization.v3.GroundMemberClassificationValue", Fields91},
    Schema{"pubg.reader.synchronization.v3.GroundMemberValue", Fields92},
    Schema{"pubg.reader.synchronization.v3.GroundMembersValue", Fields93},
    Schema{"pubg.reader.synchronization.v3.GroundSlotValue", Fields94},
    Schema{"pubg.reader.synchronization.v3.GroupVerification", Fields95},
    Schema{"pubg.reader.synchronization.v3.Heartbeat", Fields96},
    Schema{"pubg.reader.synchronization.v3.Hello", Fields97},
    Schema{"pubg.reader.synchronization.v3.HelloAccepted", Fields98},
    Schema{"pubg.reader.synchronization.v3.IdentityValue", Fields99},
    Schema{"pubg.reader.synchronization.v3.InstanceIdentity", Fields100},
    Schema{"pubg.reader.synchronization.v3.Invalidate", Fields101},
    Schema{"pubg.reader.synchronization.v3.InvalidationAck", Fields102},
    Schema{"pubg.reader.synchronization.v3.ItemClassificationValue", Fields103},
    Schema{"pubg.reader.synchronization.v3.ItemDefinitionValue", Fields104},
    Schema{"pubg.reader.synchronization.v3.ItemOwnerValue", Fields105},
    Schema{"pubg.reader.synchronization.v3.ItemStackValue", Fields106},
    Schema{"pubg.reader.synchronization.v3.KindContract", Fields107},
    Schema{"pubg.reader.synchronization.v3.LocalAimBasisState", Fields108},
    Schema{"pubg.reader.synchronization.v3.LocalAimBasisValue", Fields109},
    Schema{"pubg.reader.synchronization.v3.LocalControlState", Fields110},
    Schema{"pubg.reader.synchronization.v3.LocalControlValue", Fields111},
    Schema{"pubg.reader.synchronization.v3.LocalSpectatorsState", Fields112},
    Schema{"pubg.reader.synchronization.v3.LocalSpectatorsValue", Fields113},
    Schema{"pubg.reader.synchronization.v3.LocalState", Fields114},
    Schema{"pubg.reader.synchronization.v3.LocalWeaponValue", Fields115},
    Schema{"pubg.reader.synchronization.v3.MapValue", Fields116},
    Schema{"pubg.reader.synchronization.v3.NameValue", Fields117},
    Schema{"pubg.reader.synchronization.v3.NativeHoldingState", Fields118},
    Schema{"pubg.reader.synchronization.v3.NativeHoldingValue", Fields119},
    Schema{"pubg.reader.synchronization.v3.NativeSwayState", Fields120},
    Schema{"pubg.reader.synchronization.v3.NativeSwayValue", Fields121},
    Schema{"pubg.reader.synchronization.v3.NestedSample", Fields122},
    Schema{"pubg.reader.synchronization.v3.ObservationEvidence", Fields123},
    Schema{"pubg.reader.synchronization.v3.PacingValue", Fields124},
    Schema{"pubg.reader.synchronization.v3.PickupClassificationValue", Fields125},
    Schema{"pubg.reader.synchronization.v3.ProjectileValue", Fields126},
    Schema{"pubg.reader.synchronization.v3.PropertyPatch", Fields127},
    Schema{"pubg.reader.synchronization.v3.PropertyState", Fields128},
    Schema{"pubg.reader.synchronization.v3.PropertyValue", Fields129},
    Schema{"pubg.reader.synchronization.v3.ReliableEnvelope", Fields130},
    Schema{"pubg.reader.synchronization.v3.ResyncRequest", Fields131},
    Schema{"pubg.reader.synchronization.v3.RiderValue", Fields132},
    Schema{"pubg.reader.synchronization.v3.RidersValue", Fields133},
    Schema{"pubg.reader.synchronization.v3.SampleInterval", Fields134},
    Schema{"pubg.reader.synchronization.v3.SeatValue", Fields135},
    Schema{"pubg.reader.synchronization.v3.SeatsValue", Fields136},
    Schema{"pubg.reader.synchronization.v3.SkeletonSource", Fields137},
    Schema{"pubg.reader.synchronization.v3.SourceValue", Fields138},
    Schema{"pubg.reader.synchronization.v3.SourceVerification", Fields139},
    Schema{"pubg.reader.synchronization.v3.SpatialValue", Fields140},
    Schema{"pubg.reader.synchronization.v3.StringValue", Fields141},
    Schema{"pubg.reader.synchronization.v3.TaskSummary", Fields142},
    Schema{"pubg.reader.synchronization.v3.TransportPolicy", Fields143},
    Schema{"pubg.reader.synchronization.v3.Update", Fields144},
    Schema{"pubg.reader.synchronization.v3.VehicleClassificationValue", Fields145},
    Schema{"pubg.reader.synchronization.v3.VehicleModelValue", Fields146},
    Schema{"pubg.reader.synchronization.v3.VehicleSeatsValue", Fields147},
    Schema{"pubg.reader.synchronization.v3.VehicleStateValue", Fields148},
    Schema{"pubg.reader.synchronization.v3.ViewPoseState", Fields149},
    Schema{"pubg.reader.synchronization.v3.ViewPoseValue", Fields150},
    Schema{"pubg.reader.synchronization.v3.ViewRotation", Fields151},
    Schema{"pubg.reader.synchronization.v3.ViewSourceState", Fields152},
    Schema{"pubg.reader.synchronization.v3.ViewSourceValue", Fields153},
    Schema{"pubg.reader.synchronization.v3.ViewState", Fields154},
    Schema{"pubg.reader.synchronization.v3.ViewTargetState", Fields155},
    Schema{"pubg.reader.synchronization.v3.ViewTargetValue", Fields156},
    Schema{"pubg.reader.synchronization.v3.WeaponMuzzleValue", Fields157},
    Schema{"pubg.reader.synchronization.v3.WeaponRecoilTargetsValue", Fields158},
    Schema{"pubg.reader.synchronization.v3.WeaponTrajectoryGenerationValue", Fields159},
    Schema{"pubg.reader.synchronization.v3.WorldValue", Fields160},
};
inline constexpr int QuaternionSchema = 0;
inline constexpr int TransformSchema = 1;
inline constexpr int Vec3Schema = 2;
inline constexpr int WorldOriginSchema = 3;
inline constexpr int AppliedAckSchema = 4;
inline constexpr int BaselineSchema = 5;
inline constexpr int BoneSchema = 6;
inline constexpr int CaptureIntervalSchema = 7;
inline constexpr int CaptureRateRequestSchema = 8;
inline constexpr int CaptureRateResponseSchema = 9;
inline constexpr int CharacterMovementValueSchema = 10;
inline constexpr int CharacterSkeletonValueSchema = 11;
inline constexpr int CharacterStateValueSchema = 12;
inline constexpr int CharacterWorldVelocityValueSchema = 13;
inline constexpr int ClockIdentitySchema = 14;
inline constexpr int ClockMappingSchema = 15;
inline constexpr int ClockProbeSchema = 16;
inline constexpr int ClockProbeReplySchema = 17;
inline constexpr int ConsumerAimBasisSchema = 18;
inline constexpr int ConsumerBaselineSchema = 19;
inline constexpr int ConsumerBatchSchema = 20;
inline constexpr int ConsumerBoneSchema = 21;
inline constexpr int ConsumerCameraSchema = 22;
inline constexpr int ConsumerCameraTargetSchema = 23;
inline constexpr int ConsumerCaptureSchema = 24;
inline constexpr int ConsumerContainerTypeSchema = 25;
inline constexpr int ConsumerContextSchema = 26;
inline constexpr int ConsumerContextValuesSchema = 27;
inline constexpr int ConsumerControllerRotationSchema = 28;
inline constexpr int ConsumerCreatureTypeSchema = 29;
inline constexpr int ConsumerEntitySchema = 30;
inline constexpr int ConsumerFieldSourceSchema = 31;
inline constexpr int ConsumerGroundContentsSchema = 32;
inline constexpr int ConsumerGroundMemberSchema = 33;
inline constexpr int ConsumerGroupRecordSchema = 34;
inline constexpr int ConsumerGroupValueSchema = 35;
inline constexpr int ConsumerHoldingSchema = 36;
inline constexpr int ConsumerItemClassificationSchema = 37;
inline constexpr int ConsumerItemDefinitionSchema = 38;
inline constexpr int ConsumerKindContractSchema = 39;
inline constexpr int ConsumerLivingStateSchema = 40;
inline constexpr int ConsumerLocalControlSchema = 41;
inline constexpr int ConsumerMovementSchema = 42;
inline constexpr int ConsumerMuzzleSchema = 43;
inline constexpr int ConsumerNameSchema = 44;
inline constexpr int ConsumerOriginSchema = 45;
inline constexpr int ConsumerPacingSchema = 46;
inline constexpr int ConsumerQuaternionSchema = 47;
inline constexpr int ConsumerRecoilSchema = 48;
inline constexpr int ConsumerReferenceSchema = 49;
inline constexpr int ConsumerRotationSchema = 50;
inline constexpr int ConsumerSampleSchema = 51;
inline constexpr int ConsumerSeatSchema = 52;
inline constexpr int ConsumerSeatsSchema = 53;
inline constexpr int ConsumerSkeletonSchema = 54;
inline constexpr int ConsumerSpatialSchema = 55;
inline constexpr int ConsumerSpectatorsSchema = 56;
inline constexpr int ConsumerStackSchema = 57;
inline constexpr int ConsumerSwaySchema = 58;
inline constexpr int ConsumerTransformSchema = 59;
inline constexpr int ConsumerUpdateSchema = 60;
inline constexpr int ConsumerValueChangeSchema = 61;
inline constexpr int ConsumerVectorSchema = 62;
inline constexpr int ConsumerVehicleModelSchema = 63;
inline constexpr int ConsumerVehicleStateSchema = 64;
inline constexpr int ConsumerVehicleTypeSchema = 65;
inline constexpr int ConsumerWeaponSchema = 66;
inline constexpr int ConsumerWithdrawalSchema = 67;
inline constexpr int ConsumerWorldVelocitySchema = 68;
inline constexpr int ContainerValueSchema = 69;
inline constexpr int ContextStateSchema = 70;
inline constexpr int ContextValueSchema = 71;
inline constexpr int ControllerRotationStateSchema = 72;
inline constexpr int ControllerRotationValueSchema = 73;
inline constexpr int CoordinatesValueSchema = 74;
inline constexpr int CreatureClassificationValueSchema = 75;
inline constexpr int CurrentVerificationSchema = 76;
inline constexpr int CurrentWeaponStateSchema = 77;
inline constexpr int CurrentWeaponValueSchema = 78;
inline constexpr int EntityRecordSchema = 79;
inline constexpr int EntityReferenceSchema = 80;
inline constexpr int EntityVerificationSchema = 81;
inline constexpr int EntityWithdrawalSchema = 82;
inline constexpr int ErrorResponseSchema = 83;
inline constexpr int FieldSourceSchema = 84;
inline constexpr int FrameContextSchema = 85;
inline constexpr int GroundCollectionSourceValueSchema = 86;
inline constexpr int GroundGroupClassificationValueSchema = 87;
inline constexpr int GroundGroupContentsValueSchema = 88;
inline constexpr int GroundItemValueSchema = 89;
inline constexpr int GroundMemberAssociationValueSchema = 90;
inline constexpr int GroundMemberClassificationValueSchema = 91;
inline constexpr int GroundMemberValueSchema = 92;
inline constexpr int GroundMembersValueSchema = 93;
inline constexpr int GroundSlotValueSchema = 94;
inline constexpr int GroupVerificationSchema = 95;
inline constexpr int HeartbeatSchema = 96;
inline constexpr int HelloSchema = 97;
inline constexpr int HelloAcceptedSchema = 98;
inline constexpr int IdentityValueSchema = 99;
inline constexpr int InstanceIdentitySchema = 100;
inline constexpr int InvalidateSchema = 101;
inline constexpr int InvalidationAckSchema = 102;
inline constexpr int ItemClassificationValueSchema = 103;
inline constexpr int ItemDefinitionValueSchema = 104;
inline constexpr int ItemOwnerValueSchema = 105;
inline constexpr int ItemStackValueSchema = 106;
inline constexpr int KindContractSchema = 107;
inline constexpr int LocalAimBasisStateSchema = 108;
inline constexpr int LocalAimBasisValueSchema = 109;
inline constexpr int LocalControlStateSchema = 110;
inline constexpr int LocalControlValueSchema = 111;
inline constexpr int LocalSpectatorsStateSchema = 112;
inline constexpr int LocalSpectatorsValueSchema = 113;
inline constexpr int LocalStateSchema = 114;
inline constexpr int LocalWeaponValueSchema = 115;
inline constexpr int MapValueSchema = 116;
inline constexpr int NameValueSchema = 117;
inline constexpr int NativeHoldingStateSchema = 118;
inline constexpr int NativeHoldingValueSchema = 119;
inline constexpr int NativeSwayStateSchema = 120;
inline constexpr int NativeSwayValueSchema = 121;
inline constexpr int NestedSampleSchema = 122;
inline constexpr int ObservationEvidenceSchema = 123;
inline constexpr int PacingValueSchema = 124;
inline constexpr int PickupClassificationValueSchema = 125;
inline constexpr int ProjectileValueSchema = 126;
inline constexpr int PropertyPatchSchema = 127;
inline constexpr int PropertyStateSchema = 128;
inline constexpr int PropertyValueSchema = 129;
inline constexpr int ReliableEnvelopeSchema = 130;
inline constexpr int ResyncRequestSchema = 131;
inline constexpr int RiderValueSchema = 132;
inline constexpr int RidersValueSchema = 133;
inline constexpr int SampleIntervalSchema = 134;
inline constexpr int SeatValueSchema = 135;
inline constexpr int SeatsValueSchema = 136;
inline constexpr int SkeletonSourceSchema = 137;
inline constexpr int SourceValueSchema = 138;
inline constexpr int SourceVerificationSchema = 139;
inline constexpr int SpatialValueSchema = 140;
inline constexpr int StringValueSchema = 141;
inline constexpr int TaskSummarySchema = 142;
inline constexpr int TransportPolicySchema = 143;
inline constexpr int UpdateSchema = 144;
inline constexpr int VehicleClassificationValueSchema = 145;
inline constexpr int VehicleModelValueSchema = 146;
inline constexpr int VehicleSeatsValueSchema = 147;
inline constexpr int VehicleStateValueSchema = 148;
inline constexpr int ViewPoseStateSchema = 149;
inline constexpr int ViewPoseValueSchema = 150;
inline constexpr int ViewRotationSchema = 151;
inline constexpr int ViewSourceStateSchema = 152;
inline constexpr int ViewSourceValueSchema = 153;
inline constexpr int ViewStateSchema = 154;
inline constexpr int ViewTargetStateSchema = 155;
inline constexpr int ViewTargetValueSchema = 156;
inline constexpr int WeaponMuzzleValueSchema = 157;
inline constexpr int WeaponRecoilTargetsValueSchema = 158;
inline constexpr int WeaponTrajectoryGenerationValueSchema = 159;
inline constexpr int WorldValueSchema = 160;
} // namespace monitor::synchronization::codec::detail
