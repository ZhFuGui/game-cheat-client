#pragma once
#include <cstdint>
#include <string>

namespace mousebox
{
struct QpcStamp
{
    std::string domain;
    std::uint64_t ticks, frequency;
};
QpcStamp readQpc();
std::uint64_t readQpcTicks();
} // namespace mousebox
