#pragma once
#include "synchronization/codec/detail.hpp"
#include "synchronization/consumer/session.hpp"

namespace monitor::synchronization::codec::detail {
Object ConsumerValue(const consumer::GroupValue&);
consumer::GroupValue ReadConsumerValue(const Object&);
Object ConsumerBaseline(const consumer::BaselineOffer&, const Binding&);
consumer::BaselineOffer ReadConsumerBaseline(const Object&, const Binding&);
Object ConsumerDelta(const consumer::Update&, const Binding&);
consumer::Update ReadConsumerDelta(const Object&, const Binding&);
} // namespace monitor::synchronization::codec::detail
