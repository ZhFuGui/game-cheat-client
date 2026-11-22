#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mousebox::sources::cs2_reader::detail
{
struct Field
{
    unsigned wire = 0;
    std::uint64_t n = 0;
    std::string_view bytes;
};
using Fields = std::map<unsigned, std::vector<Field>>;
// Fields borrow the enclosing message for the duration of decoding.
Fields fields(std::string_view);
void require(bool, const char *);
const Field *one(const Fields &, unsigned id, unsigned wire);
const std::vector<Field> &many(const Fields &, unsigned id);
std::uint64_t number(const Fields &, unsigned id, unsigned wire = 0);
std::string_view bytes(const Fields &, unsigned id);
std::string hex(std::string_view);
std::string text(std::string_view);
double real(const Field &);
std::optional<double> optionalFloat(const Fields &, unsigned id);
std::optional<std::uint64_t> optional64(const Fields &, unsigned id);
std::optional<unsigned> optionalUnsigned(const Fields &, unsigned id);
std::optional<int> optionalInt(const Fields &, unsigned id);
std::optional<bool> optionalBool(const Fields &, unsigned id);
std::pair<std::string, std::uint64_t> decodeClock(std::string_view);
} // namespace mousebox::sources::cs2_reader::detail
