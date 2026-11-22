#pragma once
#include <stdint.h>
#include <string.h>

namespace mousebox_firmware
{
template <uint16_t Depth, uint16_t MaxReport> class ReportQueue
{
    static_assert(Depth > 0 && Depth <= 255 && MaxReport > 0, "Invalid queue limits");
    struct Packet
    {
        uint16_t length;
        uint8_t bytes[MaxReport];
    } packets[Depth];
    uint8_t head = 0, tail = 0, count = 0;

  public:
    bool full() const
    {
        return count == Depth;
    }
    uint8_t size() const
    {
        return count;
    }
    const uint8_t *front() const
    {
        return count ? packets[head].bytes : nullptr;
    }
    void pop()
    {
        if (count)
        {
            head = (head + 1) % Depth;
            --count;
        }
    }
    bool push(const uint8_t *bytes, uint16_t length)
    {
        if (full() || !length || length > MaxReport)
            return false;
        packets[tail].length = length;
        memcpy(packets[tail].bytes, bytes, length);
        tail = (tail + 1) % Depth;
        ++count;
        return true;
    }
    template <class Device> int submit(uint8_t endpoint)
    {
        if (!count)
            return 0;
        const int result = Device::trySend(endpoint, packets[head].bytes, packets[head].length);
        if (result > 0)
            pop();
        return result;
    }
    uint8_t clear()
    {
        const uint8_t removed = count;
        head = tail = count = 0;
        return removed;
    }
};
} // namespace mousebox_firmware
