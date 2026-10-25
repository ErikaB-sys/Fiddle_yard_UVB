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
// Sensor-aware, fixed-speed movement used ONLY by the reference state machine.
// Old Nano direction: awayMotor=true means increasing position (G39 side).
enum class ReferenceEnd : uint8_t { NONE, ENDSTOP, REF_EXIT, STEP_LIMIT };
struct ReferenceTimerSample {
    uint32_t position = 0U;
    uint32_t coarse = 0U;
    uint32_t entry = 0U;
    uint32_t exit = 0U;
    ReferenceEnd reason = ReferenceEnd::NONE;
    bool coarseSeen = false;
    bool entrySeen = false;
    bool exitSeen = false;
};
void stepTimerReferenceSetPosition(uint32_t position);
bool stepTimerStartReference(bool awayMotor, uint16_t stepHz,
                             bool stopOnRefExit, bool captureCoarse);
bool stepTimerTakeReferenceDone(ReferenceTimerSample& sample);
ReferenceTimerSample stepTimerReferenceSnapshot();

void stepTimerStop();
bool stepTimerRunning();
uint32_t stepTimerCompletedSteps();
bool stepTimerTakeDone(uint32_t& completedSteps);

} // namespace fy
