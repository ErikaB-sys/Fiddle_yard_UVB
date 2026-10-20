#pragma once
#include <Arduino.h>

namespace fy {

// The Nano's six ordered stages, but without AVR timer-register coupling.
// accel: signed delta in *Nano timer ticks* (+ means slower).
// rampEvery: apply one delta after this many completed STEP pulses.
struct StepProfileSegment {
    uint16_t steps;
    int16_t accel;
    uint16_t rampEvery;
};
constexpr uint8_t STEP_PROFILE_PHASES = 6U;

// Muninn alone starts/stops movements. The ISR counts completed pulses
// on the falling edge and advances phases there (never in HIGH).
bool stepTimerBegin();

// Existing fixed-frequency diagnostics. Retained for compatibility.
bool stepTimerStart(uint16_t steps, bool directionRight, uint16_t stepHz);

// Six-phase Nano motion executor. Returns false if backend unsupported,
// total mismatch, timer busy, zero distance, or invalid ramp intervals.
// On Arduino-ESP32 2.x the ISR writes the TIMG1/T0 next alarm interval
// directly (IRAM-safe) without calling timerAlarmWrite inside the ISR.
bool stepTimerStartProfile(uint16_t steps, bool directionRight,
                           const StepProfileSegment (&segments)[STEP_PROFILE_PHASES]);
void stepTimerStop();
bool stepTimerRunning();
uint32_t stepTimerCompletedSteps();
bool stepTimerTakeDone(uint32_t& completedSteps);

} // namespace fy
