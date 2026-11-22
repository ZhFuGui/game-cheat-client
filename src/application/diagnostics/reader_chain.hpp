#pragma once
#include "application/json.hpp"
#include "domain/reader_chain_metrics.hpp"
namespace mousebox::application
{
Json describeReaderChain(const domain::ReaderChainSnapshot &);
}
