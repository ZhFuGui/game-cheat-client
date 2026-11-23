#pragma once
#include <cstdint>
#include <string_view>

namespace mousebox::protocol
{
inline bool validUtf8(std::string_view text) noexcept
{
    std::size_t offset = 0;
    while (offset < text.size())
    {
        const auto lead = static_cast<unsigned char>(text[offset++]);
        if (lead < 128)
            continue;
        const unsigned count = lead >= 0xc2 && lead <= 0xdf   ? 1
                               : lead >= 0xe0 && lead <= 0xef ? 2
                               : lead >= 0xf0 && lead <= 0xf4 ? 3
                                                              : 0;
        if (!count || count > text.size() - offset)
            return false;
        std::uint32_t code = lead & ((1u << (6 - count)) - 1u);
        for (unsigned i = 0; i < count; ++i)
        {
            const auto tail = static_cast<unsigned char>(text[offset++]);
            if ((tail & 0xc0) != 0x80)
                return false;
            code = (code << 6) | (tail & 63);
        }
        if (code < (count == 1   ? 0x80u
                    : count == 2 ? 0x800u
                                 : 0x10000u) ||
            code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff))
            return false;
    }
    return true;
}
} // namespace mousebox::protocol
