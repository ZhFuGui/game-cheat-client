#pragma once
#include <stdint.h>

namespace mousebox_firmware
{
struct MotionStep
{
    int16_t x, y;
};

// USB setup fields are normalized before reaching the common runtime.
struct ControlRequest
{
    uint8_t bmRequestType, bRequest, wValueL, wValueH;
    uint16_t wIndex, wLength;
};
static_assert(sizeof(ControlRequest) == 8, "USB setup layout");
} // namespace mousebox_firmware
