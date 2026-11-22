#pragma once
namespace mousebox_avr
{
struct LeonardoHostBoard
{
    // USB Host Shield 2.0: SPI uses ICSP; SS=D10, INT=D9 in the pinned library.
    // UART control: D0=RX, D1=TX. Neither the TTL VCC pin nor an external supply
    // is enabled by firmware. No experimental power/reset/idle keepalive policy.
    enum
    {
        MouseQueueDepth = 32,
        AuxiliaryQueueDepth = 4,
        VendorQueueDepth = 8
    };
};
} // namespace mousebox_avr
