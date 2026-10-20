#include <Arduino.h>
#include <freertos/task.h>
#include "FY_Bringup.h"
#include "UART.h"
#include "Motor.h"

namespace {
fy::BringupContext bridge{nullptr, nullptr};

// HW2: STEP=25, DIR=26, ENA=27 (A4988: ENABLE is active LOW).
constexpr uint8_t PIN_STEP = 25;
constexpr uint8_t PIN_DIR = 26;
constexpr uint8_t PIN_ENA = 27;

void blinkHw2Outputs() {
    // Setup-only visual wiring check; never used for motor stepping.
    const uint8_t pins[] = {PIN_STEP, PIN_DIR, PIN_ENA};
    for (const uint8_t pin : pins) {
        digitalWrite(pin, pin == PIN_ENA ? LOW : HIGH);
        delay(650);  // Deliberately visible to the human eye.
        digitalWrite(pin, pin == PIN_ENA ? HIGH : LOW);
        delay(350);
    }
}
}

void setup() {
    Serial.begin(115200);
    // Safe output levels before configuring the pins as outputs.
    digitalWrite(PIN_STEP, LOW);
    digitalWrite(PIN_DIR, LOW);
    digitalWrite(PIN_ENA, HIGH); // Driver disabled.
    pinMode(PIN_STEP, OUTPUT);
    pinMode(PIN_DIR, OUTPUT);
    pinMode(PIN_ENA, OUTPUT);
    bridge.commands = xQueueCreate(8, sizeof(fy::CommandMessage));
    bridge.events = xQueueCreate(8, sizeof(fy::MotorEvent));
    if (!bridge.commands || !bridge.events) {
        Serial.println("FY_TEST: failed to create queues");
        return;
    }

    // Module owners create their own FreeRTOS tasks on assigned cores.
    fy::Muninn_initMotorMock();
    const BaseType_t muninn = fy::Muninn_startMotorMock(&bridge);
    const BaseType_t huginn = fy::Huginn_startUART(&bridge);
    if (muninn != pdPASS || huginn != pdPASS) {
        Serial.println("FY_TEST: failed to create tasks");
    }
    blinkHw2Outputs();
}

void loop() {
    // Huginn and Muninn own their work loops. Arduino loop is idle.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
