#pragma once
#include <stdint.h>

namespace fy {

// ESP32-only commissioning extension. Nano command IDs in Protokoll.h
// remain untouched: legacy LEFT=0x0B and RIGHT=0x0D are NOT MOVE.
constexpr uint8_t CMD_MOVE = 0x0FU;

// Maximum displacement PER command without a valid reference.
// BENCH ONLY: no cumulative limit, end-stop protection or FY mechanics.
constexpr uint16_t FY_MOVE_MAX_UNREFERENCED_STEPS = 10000U;

// Byte 0: high nibble = microstep mode, bit 0 = direction,
//         bits 3..1 reserved and must be zero.
// Byte 1: step count low byte; byte 2: step count high byte.
enum class MoveMicrostep : uint8_t {
    FULL = 0U, HALF = 1U, QUARTER = 2U, EIGHTH = 3U, SIXTEENTH = 4U
};

// Fixed hardware resolution: MS pins are configured once at motor startup.
constexpr MoveMicrostep FY_MOTOR_MICROSTEP_MODE = MoveMicrostep::EIGHTH;

struct MoveRequest {
    bool directionRight = false; // 0=LEFT, 1=RIGHT
    MoveMicrostep mode = FY_MOTOR_MICROSTEP_MODE;
    uint16_t steps = 0U;
};

// Pure payload validation. Command length and CRC are checked by Huginn.
inline bool decodeMovePayload(const uint8_t* data, MoveRequest& result) {
    if (data == nullptr) return false;
    const uint8_t flags = data[0];
    const uint8_t mode = static_cast<uint8_t>(flags >> 4U);
    if ((flags & 0x0EU) != 0U ||
        mode > static_cast<uint8_t>(MoveMicrostep::SIXTEENTH))
        return false;

    const uint16_t steps = static_cast<uint16_t>(
        static_cast<uint16_t>(data[1]) |
        (static_cast<uint16_t>(data[2]) << 8U));
    if (steps == 0U || steps > FY_MOVE_MAX_UNREFERENCED_STEPS)
        return false;

    result.directionRight = (flags & 0x01U) != 0U;
    result.mode = static_cast<MoveMicrostep>(mode);
    result.steps = steps;
    return true;
}

} // namespace fy
