#pragma once
#include <stdint.h>

namespace fy {

// ONLY the loose NEMA11 bench; never a substitute for a measured reference.
// Coordinates count A4988 QUARTER steps as used by the FY Nano hardware test.
constexpr uint16_t FY_BENCH_POSITION_START = UINT16_MAX / 2U; // 32767
constexpr uint16_t FY_BENCH_NANO_TRACK_PITCH = 1600U;
constexpr uint16_t FY_BENCH_NANO_EDGE_OFFSET = 1000U;
constexpr uint16_t FY_BENCH_NOMINAL_TRACK_INTERVALS = 6U;
constexpr uint16_t FY_BENCH_NOMINAL_TRAVEL = FY_BENCH_NANO_EDGE_OFFSET +
    FY_BENCH_NOMINAL_TRACK_INTERVALS * FY_BENCH_NANO_TRACK_PITCH; // 10600
constexpr uint16_t FY_BENCH_MAX_ABSOLUTE_MOVE = FY_BENCH_NOMINAL_TRAVEL;

// Virtual BG3 is aligned with the start coordinate, BG1..BG5 spaced
// using the measured Nano interval. The nominal travel is a *test scale*
// and per-command upper bound, NOT a mechanical or absolute end switch.
constexpr uint16_t FY_BENCH_BG1 = FY_BENCH_POSITION_START - 2U * FY_BENCH_NANO_TRACK_PITCH;
constexpr uint16_t FY_BENCH_BG2 = FY_BENCH_POSITION_START - FY_BENCH_NANO_TRACK_PITCH;
constexpr uint16_t FY_BENCH_BG3 = FY_BENCH_POSITION_START;
constexpr uint16_t FY_BENCH_BG4 = FY_BENCH_POSITION_START + FY_BENCH_NANO_TRACK_PITCH;
constexpr uint16_t FY_BENCH_BG5 = FY_BENCH_POSITION_START + 2U * FY_BENCH_NANO_TRACK_PITCH;

constexpr uint16_t FY_BENCH_MIN_POSITION = 1U;
constexpr uint16_t FY_BENCH_MAX_POSITION = UINT16_MAX - 1U;

// Prevent integer wrapping: moving to 0 or UINT16_MAX is not permitted.
inline bool benchCanMove(uint16_t start, uint16_t steps, bool right) {
    if (steps == 0U || start < FY_BENCH_MIN_POSITION ||
        start > FY_BENCH_MAX_POSITION) return false;
    const int32_t destination = static_cast<int32_t>(start) +
        (right ? static_cast<int32_t>(steps)
               : -static_cast<int32_t>(steps));
    return destination >= FY_BENCH_MIN_POSITION &&
           destination <= FY_BENCH_MAX_POSITION;
}

// Compute signed difference in wider type to avoid uint16 underflow.
inline uint16_t benchAbsoluteDistance(uint16_t from, uint16_t to) {
    return to >= from ? static_cast<uint16_t>(to - from)
                      : static_cast<uint16_t>(from - to);
}

} // namespace fy
