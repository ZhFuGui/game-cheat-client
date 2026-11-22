#pragma once
#include "types.hpp"

// Compile-time port contracts (no virtual dispatch or allocation):
// Clock: static uint32_t nowUs().
// Device: static bool configured(); static uint8_t epoch();
//         static int trySend(uint8_t endpoint, const void*, uint16_t length);
//         static void serviceControl().
// Channel: begin(), enableFeedback(), available(), read(), availableForWrite(),
//          write(const uint8_t*, uint16_t). Writes must not block.
// Host: begin(), service(), poll(), ready(), needsRecovery(), recover(),
//       cancelControl(), beginControl(ControlRequest,...), finishControl(...).
//
// A positive trySend result means one complete report entered the USB bank.
// It is NOT proof of an ACK, OS delivery, or cursor movement. A busy bank returns
// zero without consuming the queued report. Buffers stay owned by the caller
// until submission. A different backend must preserve these semantics.
