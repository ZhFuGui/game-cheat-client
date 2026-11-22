#pragma once
#include "application/ports/source_inspection.hpp"
#include "session.hpp"
namespace mousebox::reader
{
application::ports::SourcePacket inspectConsumer(const Session *);
}
