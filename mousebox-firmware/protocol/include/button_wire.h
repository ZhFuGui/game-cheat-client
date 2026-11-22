#pragma once
#include "control_wire.h"
namespace mousebox_buttons
{
const uint8_t FrameSize = 14;
const uint32_t EnableToken = 0x53494445UL;
const uint16_t Back = 1u << 3, Forward = 1u << 4;
const uint8_t Ready = 1, Known = 2, ExtendedMotion = 4, HighRateMotion = 8, FiniteButtonPulse = 16;
struct State
{
    uint32_t sequence;
    uint16_t buttons, releases;
    uint8_t flags;
};
inline void encode(const State &s, uint8_t *p)
{
    p[0] = 0xB6;
    p[1] = 0x4D;
    p[2] = 1;
    p[3] = s.flags;
    mousebox_wire::write32(p + 4, s.sequence);
    mousebox_wire::write16(p + 8, s.buttons);
    mousebox_wire::write16(p + 10, s.releases);
    mousebox_wire::write16(p + 12, mousebox_wire::crc16(p, 12));
}
inline bool decode(const uint8_t *p, State &s)
{
    if (p[0] != 0xB6 || p[1] != 0x4D || p[2] != 1 ||
        (p[3] & ~uint8_t(Ready | Known | ExtendedMotion | HighRateMotion | FiniteButtonPulse)) ||
        ((p[3] & HighRateMotion) && !(p[3] & ExtendedMotion)) ||
        mousebox_wire::read16(p + 12) != mousebox_wire::crc16(p, 12))
        return false;
    s = {mousebox_wire::read32(p + 4), mousebox_wire::read16(p + 8), mousebox_wire::read16(p + 10), p[3]};
    return s.sequence != 0;
}
class Parser
{
    uint8_t bytes[FrameSize] = {}, used = 0;

  public:
    bool feed(uint8_t b, State &s)
    {
        if (!used && b != 0xB6)
            return false;
        if (used == 1 && b != 0x4D)
        {
            used = b == 0xB6 ? 1 : 0;
            return false;
        }
        bytes[used++] = b;
        if (used < FrameSize)
            return false;
        if (decode(bytes, s))
        {
            used = 0;
            return true;
        }
        for (uint8_t i = 1; i < FrameSize; ++i)
            if (bytes[i] == 0xB6 && (i == FrameSize - 1 || bytes[i + 1] == 0x4D))
            {
                used = FrameSize - i;
                memmove(bytes, bytes + i, used);
                return false;
            }
        used = 0;
        return false;
    }
};
} // namespace mousebox_buttons
