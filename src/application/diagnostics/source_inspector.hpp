#pragma once
#include "application/ports/source_inspection.hpp"
#include <memory>
#include <string_view>
namespace mousebox::application
{
// One on-demand work slot, one serialized result; no continuous subscription.
class SourceInspector
{
    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    SourceInspector();
    ~SourceInspector();
    Json read(ports::SourcePackets packets, std::string_view selected, std::string_view after);
};
} // namespace mousebox::application
