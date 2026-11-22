#pragma once
#include "synchronization/codec/wire.hpp"
#include "synchronization/codec/schema.hpp"
#include <stdexcept>

namespace monitor::synchronization::codec::detail {
using Object = Value::Object;
using List = Value::List;
struct Failure : std::runtime_error {
    ErrorCode code;
    Failure(ErrorCode c, const char* text) : std::runtime_error(text), code(c) {}
};
[[noreturn]] inline void Fail(const char* text, ErrorCode code = ErrorCode::InvalidMessage) {
    throw Failure(code, text);
}
void Require(bool, const char*, ErrorCode code = ErrorCode::InvalidMessage);
inline void Require(bool value, const std::string& text, ErrorCode code = ErrorCode::InvalidMessage) {
    if (!value)
        Fail(text.c_str(), code);
}
std::string EncodeMessage(int schema, const Object&, std::size_t max_bytes);
Object DecodeMessage(int schema, std::string_view bytes, std::size_t max_bytes);
Object DeclaredPropertyFields(PropertyGroup, const Object&, u64 declared_fields);
u64 UInt(const Object&, unsigned);
std::int64_t Signed(const Object&, unsigned);
bool Boolean(const Object&, unsigned);
std::string Text(const Object&, unsigned);
const Object& Message(const Object&, unsigned);
const List& Repeated(const Object&, unsigned);
void Put(Object&, unsigned, u64);
void Put(Object&, unsigned, std::int64_t);
void Put(Object&, unsigned, bool);
void Put(Object&, unsigned, std::string);
void Put(Object&, unsigned, Object);
void Put(Object&, unsigned, List);
Object PropertyFields(PropertyGroup, Object, bool to_wire);
Object ContextFields(ContextGroup, const Object&, bool to_wire);
Object Interval(const SampleInterval&, u64 sequence, const Binding&);
SampleInterval ReadInterval(const Object&, const Binding&, u64& sequence);
Object Nested(const std::map<std::vector<unsigned>, GroupState::NestedSample>&, const Binding&);
List NestedList(const std::map<std::vector<unsigned>, GroupState::NestedSample>&, const Binding&);
std::map<std::vector<unsigned>, GroupState::NestedSample> ReadNested(const List&, const Binding&);
Object Instance(const InstanceIdentity&);
InstanceIdentity ReadInstance(const Object&);
Object Entity(const EntityState&, const Binding&);
EntityState ReadEntity(const Object&, const Binding&);
Object Context(const FrameContext&, const Binding&);
FrameContext ReadContext(const Object&, const Binding&);
Object Current(const State&, const Binding&);
Object Current(const Update&, const Binding&);
void ReadCurrent(const Object&, State&, const Binding&);
void ReadCurrent(const Object&, Update&, const Binding&);
void ValidateSendStarted(u64 qpc, u64 finished_qpc, const Binding&);
Object Baseline(const BaselineOffer&, const Binding&);
BaselineOffer ReadBaseline(const Object&, const Binding&);
Object Delta(const Update&, const Binding&);
Update ReadDelta(const Object&, const Binding&);
Object Record(const ReliableRecord&, const Binding&);
ReliableRecord ReadRecord(const Object&, const Binding&);
} // namespace monitor::synchronization::codec::detail
