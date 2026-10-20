#include "StepTimer.h"
#include "HW_Config.h"

#include <Arduino.h>
#include <esp_arduino_version.h>
#include <soc/gpio_struct.h>

namespace fy {
namespace {

// Nano Timer1 convention: one rising STEP edge, then one falling edge;
// completed steps are counted on the FALLING edge.
// ESP32 general-purpose hardware timer: 1 MHz counter -> 1 tick = 1 us.
// The alarm fires every half-period. No motor calculations run in the ISR.
constexpr uint32_t kStepBit = uint32_t{1} << MOTOR_PWM_PIN;
constexpr uint16_t kMinStepHz = 40U;
constexpr uint16_t kMaxStepHz = 800U;

hw_timer_t* stepTimer = nullptr;
portMUX_TYPE stepMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool isRunning = false;
volatile bool stepIsHigh = false;
volatile bool isFinished = false;
volatile uint32_t completed = 0U;
volatile uint32_t target = 0U;

void IRAM_ATTR stepTimerIsr() {
    portENTER_CRITICAL_ISR(&stepMux);
    if (!isRunning) {
        // No pulses between moves. Keep STEP deterministically LOW.
        GPIO.out_w1tc = kStepBit;
        stepIsHigh = false;
    } else if (!stepIsHigh) {
        // A4988 advances on the rising edge; no Arduino digitalWrite in ISR.
        GPIO.out_w1ts = kStepBit;
        stepIsHigh = true;
    } else {
        // Count on falling edge just as in FY Nano Timer1_ISR().
        GPIO.out_w1tc = kStepBit;
        stepIsHigh = false;
        ++completed;
        if (completed >= target) {
            isRunning = false;
            isFinished = true;
        }
    }
    portEXIT_CRITICAL_ISR(&stepMux);
}

void haltAlarm() {
    if (stepTimer == nullptr) return;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    timerStop(stepTimer);
#else
    timerAlarmDisable(stepTimer);
#endif
}

void armAlarm(uint32_t halfPeriodUs) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    timerStop(stepTimer);
    timerWrite(stepTimer, 0U);
    timerAlarm(stepTimer, halfPeriodUs, true, 0U);
    timerStart(stepTimer);
#else
    timerAlarmDisable(stepTimer);
    timerWrite(stepTimer, 0U);
    timerAlarmWrite(stepTimer, halfPeriodUs, true);
    timerAlarmEnable(stepTimer);
#endif
}

} // anonymous namespace

bool stepTimerBegin() {
    if (stepTimer != nullptr) return true;
    // ENABLE is active LOW. A fresh timer never enables the motor driver.
    digitalWrite(MOTOR_PWM_PIN, LOW);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    stepTimer = timerBegin(1000000U);
#else
    // Arduino-ESP32 2.x: timer #1, 80 MHz APB / 80 = 1 MHz.
    stepTimer = timerBegin(1U, 80U, true);
#endif
    if (stepTimer == nullptr) return false;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    timerAttachInterrupt(stepTimer, &stepTimerIsr);
    timerStop(stepTimer);
#else
    timerAttachInterrupt(stepTimer, &stepTimerIsr, true);
    timerAlarmWrite(stepTimer, 4167U, true);
    timerAlarmDisable(stepTimer);
#endif
    return true;
}

bool stepTimerStart(uint16_t steps, bool directionRight, uint16_t stepHz) {
    if (stepTimer == nullptr || steps == 0U ||
        stepHz < kMinStepHz || stepHz > kMaxStepHz)
        return false;

    portENTER_CRITICAL(&stepMux);
    const bool busy = isRunning || isFinished;
    portEXIT_CRITICAL(&stepMux);
    if (busy) return false;

    // Only Muninn calls this, while STOP is released and ENABLE active.
    haltAlarm();
    digitalWrite(MOTOR_PWM_PIN, LOW);
    digitalWrite(MOTOR_DIR_PIN, directionRight ? HIGH : LOW);
    // DIR setup before first STEP >= half period (at 800 Hz >= 625 us).
    const uint32_t halfPeriodUs =
        (500000UL + static_cast<uint32_t>(stepHz / 2U)) / stepHz;

    portENTER_CRITICAL(&stepMux);
    completed = 0U;
    target = steps;
    stepIsHigh = false;
    isFinished = false;
    isRunning = true;
    portEXIT_CRITICAL(&stepMux);

    armAlarm(halfPeriodUs);
    return true;
}

void stepTimerStop() {
    haltAlarm(); // No new STEP interrupt after alarm is disabled.
    portENTER_CRITICAL(&stepMux);
    isRunning = false;
    stepIsHigh = false;
    isFinished = false;
    portEXIT_CRITICAL(&stepMux);
    digitalWrite(MOTOR_PWM_PIN, LOW);
}

bool stepTimerRunning() {
    portENTER_CRITICAL(&stepMux);
    const bool result = isRunning;
    portEXIT_CRITICAL(&stepMux);
    return result;
}

bool stepTimerTakeDone(uint32_t& completedSteps) {
    portENTER_CRITICAL(&stepMux);
    const bool done = isFinished;
    if (done) {
        completedSteps = completed;
        isFinished = false;
    }
    portEXIT_CRITICAL(&stepMux);
    if (done) haltAlarm();
    return done;
}

} // namespace fy
