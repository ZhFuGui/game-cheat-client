#pragma once
#include "synchronization/codec/wire.hpp"

namespace mousebox::reader
{
// File replay only: preserve existing v2 Debug recordings without admitting v2
// on a live connection. No identity, value, source interval or profile is invented.
bool decodeRecordedReliable(std::string_view, const monitor::synchronization::codec::Binding &, std::size_t maximum,
                            monitor::synchronization::codec::ReliableRecord &,
                            monitor::synchronization::codec::Error &);
} // namespace mousebox::reader
