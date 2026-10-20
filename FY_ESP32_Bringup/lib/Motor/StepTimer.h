#pragma once
#include <Arduino.h>

namespace fy {

// ESP32 hardware timer for the initial, constant-speed, bounded STEP test.
// This is a motor HAL, not a replacement for Nano Motor::Update() / profiles.
// Only Muninn may call these functions; ISR touches STEP and private counters.
bool stepTimerBegin();
bool stepTimerStart(uint16_t steps, bool directionRight, uint16_t stepHz);
void stepTimerStop();
bool stepTimerRunning();
// Snapshot of STEP pulses completed (counted on falling edge).
uint32_t stepTimerCompletedSteps();
bool stepTimerTakeDone(uint32_t& completedSteps);

} // namespace fy
