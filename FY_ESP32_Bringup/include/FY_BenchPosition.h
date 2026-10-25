#pragma once
#include <stdint.h>

namespace fy {

// BENCH ONLY: coordinates copied from FY_HW_TEST/FY_HW_TEST/src/main.cpp,
// trackPosition[] after physical measurement. Unit: HW_TEST eighth steps.
// A fake short reference has no sensor movement and sets coordinate 1000.
// Never use the following values as proof of a real reference.
constexpr uint16_t FY_BENCH_REFERENCE_POSITION = 1000U;
constexpr uint8_t FY_BENCH_TRACK_COUNT = 5U;
constexpr uint16_t FY_BENCH_TRACK_POSITIONS[FY_BENCH_TRACK_COUNT] = {
    1557U, 3157U, 4757U, 6357U, 7957U
};

// Bounds and single-job limit remain BENCH aids, not physical end stops.
constexpr uint16_t FY_BENCH_MAX_ABSOLUTE_MOVE = 10600U;
constexpr uint16_t FY_BENCH_MIN_POSITION = 1U;
constexpr uint16_t FY_BENCH_MAX_POSITION = UINT16_MAX - 1U;

inline bool benchTrackPosition(uint8_t number, uint16_t& out) {
    if (number < 1U || number > FY_BENCH_TRACK_COUNT) return false;
    out = FY_BENCH_TRACK_POSITIONS[number - 1U];
    return true;
}

// Guard uint16 position against overflow/underflow (BENCH only).
inline bool benchCanMove(uint16_t start, uint16_t steps, bool right) {
    if (steps == 0U || start < FY_BENCH_MIN_POSITION ||
        start > FY_BENCH_MAX_POSITION) return false;
    const int32_t destination = static_cast<int32_t>(start) +
        (right ? static_cast<int32_t>(steps)
               : -static_cast<int32_t>(steps));
    return destination >= FY_BENCH_MIN_POSITION &&
           destination <= FY_BENCH_MAX_POSITION;
}

inline uint16_t benchAbsoluteDistance(uint16_t from, uint16_t to) {
    return to >= from ? static_cast<uint16_t>(to - from)
                      : static_cast<uint16_t>(from - to);
}

} // namespace fy
