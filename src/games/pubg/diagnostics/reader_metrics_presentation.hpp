#pragma once
#include "reader_metrics.hpp"
#include "application/json.hpp"
namespace mousebox::application
{
Json describeReaderMetrics(const ReaderMetricsSnapshot &, bool gpuCompletionAvailable);
}
