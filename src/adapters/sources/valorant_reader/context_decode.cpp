#include "context_decode.hpp"
#include "protocol/utf8.hpp"

namespace mousebox::sources::valorant_reader
{
using namespace wire;
std::optional<unsigned> bombTeam(const Fields &fields, unsigned field)
{
    const auto message = one(fields, field, 2);
    if (!message)
        return {};
    const auto team = parse(message->bytes);
    const auto value = one(team, 1, 0);
    if (!value)
        return {};
    require(value->value <= 255, "VALORANT BombTeam exceeds native byte range");
    return static_cast<unsigned>(value->value);
}
PositionContext decodeContext(std::string_view input)
{
    const auto fields = parse(input);
    PositionContext result;
    if (const auto map = one(fields, 1, 2))
    {
        const auto identity = parse(map->bytes);
        const auto path = bytes(identity, 1), world = bytes(identity, 2);
        require(!path.empty() && path.size() <= 4096 && !world.empty() && world.size() <= 1024 &&
                    protocol::validUtf8(path) && protocol::validUtf8(world) && path.find('\0') == path.npos &&
                    world.find('\0') == world.npos,
                "VALORANT invalid map identity");
        result.map = MapIdentity{std::string(path), std::string(world)};
    }
    const auto local = one(fields, 2, 2);
    require(local, "VALORANT position context lacks local-character state");
    const auto character = parse(local->bytes);
    const auto state = integer(character, 1);
    require(state <= 3, "VALORANT unknown local-character state");
    result.localState = static_cast<unsigned>(state);
    result.localEntity = optionalInteger(character, 2);
    require((state == 3) == result.localEntity.has_value() && (!result.localEntity || *result.localEntity),
            "VALORANT invalid local-character reference");
    result.referenceBombTeam = bombTeam(fields, 3);
    return result;
}
} // namespace mousebox::sources::valorant_reader
