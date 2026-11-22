#pragma once
#include <stdint.h>
#include <string.h>

namespace mousebox_wire
{
const uint8_t Version = 1;
const uint8_t FrameSize = 24;
enum Type : uint8_t
{
    Start = 1,
    Motion = 2,
    Stop = 3,
    Probe = 4,
    ButtonPulse = 5
};
struct Command
{
    Type type;
    uint32_t session, sequence;
    int16_t dx, dy;
    uint16_t intervalMs, leaseMs, budget;
};
inline uint16_t read16(const uint8_t *p)
{
    return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}
inline uint32_t read32(const uint8_t *p)
{
    return uint32_t(read16(p)) | (uint32_t(read16(p + 2)) << 16);
}
inline int16_t readSigned16(const uint8_t *p)
{
    uint16_t bits = read16(p);
    return bits <= 32767 ? int16_t(bits) : int16_t(int32_t(bits) - 65536L);
}
inline void write16(uint8_t *p, uint16_t n)
{
    p[0] = uint8_t(n);
    p[1] = uint8_t(n >> 8);
}
inline void write32(uint8_t *p, uint32_t n)
{
    write16(p, uint16_t(n));
    write16(p + 2, uint16_t(n >> 16));
}
inline uint16_t crc16(const uint8_t *p, uint8_t n)
{
    uint16_t crc = 0xFFFF;
    while (n--)
    {
        crc ^= uint16_t(*p++) << 8;
        for (uint8_t bit = 0; bit < 8; ++bit)
            crc = (crc & 0x8000) ? uint16_t((crc << 1) ^ 0x1021) : uint16_t(crc << 1);
    }
    return crc;
}
inline void encode(const Command &c, uint8_t *frame)
{
    frame[0] = 0xA5;
    frame[1] = 0x5A;
    frame[2] = Version;
    frame[3] = c.type;
    write32(frame + 4, c.session);
    write32(frame + 8, c.sequence);
    write16(frame + 12, uint16_t(c.dx));
    write16(frame + 14, uint16_t(c.dy));
    write16(frame + 16, c.intervalMs);
    write16(frame + 18, c.leaseMs);
    write16(frame + 20, c.budget);
    write16(frame + 22, crc16(frame, 22));
}
inline bool decode(const uint8_t *frame, Command &c)
{
    if (frame[0] != 0xA5 || frame[1] != 0x5A || frame[2] != Version || frame[3] < Start || frame[3] > ButtonPulse ||
        read16(frame + 22) != crc16(frame, 22))
        return false;
    c.type = Type(frame[3]);
    c.session = read32(frame + 4);
    c.sequence = read32(frame + 8);
    c.dx = readSigned16(frame + 12);
    c.dy = readSigned16(frame + 14);
    c.intervalMs = read16(frame + 16);
    c.leaseMs = read16(frame + 18);
    c.budget = read16(frame + 20);
    return true;
}
class Parser
{
    uint8_t bytes[FrameSize] = {}, used = 0;

  public:
    uint16_t errors = 0;
    void reset()
    {
        used = 0;
    }
    bool feed(uint8_t byte, Command &c)
    {
        if (!used && byte != 0xA5)
            return false;
        if (used == 1 && byte != 0x5A)
        {
            used = byte == 0xA5 ? 1 : 0;
            return false;
        }
        bytes[used++] = byte;
        if (used < FrameSize)
            return false;
        if (decode(bytes, c))
        {
            used = 0;
            return true;
        }
        ++errors;
        // Keep a possible next frame after loss/corruption, including a split sync word.
        for (uint8_t i = 1; i < FrameSize; ++i)
        {
            if (bytes[i] == 0xA5 && (i == FrameSize - 1 || bytes[i + 1] == 0x5A))
            {
                used = FrameSize - i;
                memmove(bytes, bytes + i, used);
                return false;
            }
        }
        used = 0;
        return false;
    }
};
} // namespace mousebox_wire
