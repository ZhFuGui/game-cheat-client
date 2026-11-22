#pragma once
#include "wire_facts.hpp"
namespace mousebox::reader::detail
{
void projectNativeObservations(const s::State &, control::Frame &);
std::optional<control::ActiveWeapon> activeWeapon(const s::State &, control::Identity);
} // namespace mousebox::reader::detail
