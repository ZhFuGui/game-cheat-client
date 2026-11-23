#pragma once
#include "wire.hpp"
#include "model.hpp"

namespace mousebox::sources::valorant_reader
{
inline constexpr std::string_view SchemaHash = "ef15d4c567861cd4f98895cd36b5fd3037045ce5b059155bc891b3bd6d4caf01";
inline constexpr unsigned MaximumRecord = 16 * 1024 * 1024;
struct Envelope
{
    unsigned body = 0;
    std::uint64_t event = 0;
    std::optional<std::uint64_t> sent, mapping;
    wire::Fields payload;
};
struct Hello
{
    std::string instance, domain;
    std::uint64_t frequency = 0, clockRevision = 0;
    std::array<bool, 4> streams{};
    bool clockExchange = false;
    double maximumHz = 0;
};
Envelope decode(std::string_view);
Hello decodeHello(const Envelope &);
// Returns false only for a valid delta that needs a new baseline. Malformed
// messages throw; a failed apply never partially mutates the accepted state.
bool applyUpdate(const Envelope &, unsigned stream, std::uint64_t clockRevision, StreamState &);
std::string request(unsigned body, std::string payload);
std::string subscribe(std::uint64_t requestId, unsigned stream, double hz);
std::string rate(std::uint64_t requestId, std::uint64_t revision, double hz);
std::string schemaBytes();
} // namespace mousebox::sources::valorant_reader
