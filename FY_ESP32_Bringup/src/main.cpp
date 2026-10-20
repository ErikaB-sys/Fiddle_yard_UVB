#include <Arduino.h>
#include <freertos/task.h>
#include "FY_Bringup.h"
#include "UART.h"
#include "Motor.h"
#include "HW_Config.h"

namespace {
fy::BringupContext bridge{nullptr, nullptr, nullptr};

void blinkHw2Outputs() {
    // Setup-only visual wiring check; never used for motor stepping.
    const uint8_t pins[] = {MOTOR_PWM_PIN, MOTOR_DIR_PIN, MOTOR_ENABLE_PIN};
    for (const uint8_t pin : pins) {
        digitalWrite(pin, pin == MOTOR_ENABLE_PIN ? LOW : HIGH);
        delay(650);  // Deliberately visible to the human eye.
        digitalWrite(pin, pin == MOTOR_ENABLE_PIN ? HIGH : LOW);
        delay(350);
    }
}
}

void setup() {
    Serial.begin(115200);
    // Safe output levels before configuring the pins as outputs.
    digitalWrite(MOTOR_PWM_PIN, LOW);
    digitalWrite(MOTOR_DIR_PIN, LOW);
    digitalWrite(MOTOR_ENABLE_PIN, HIGH); // Driver disabled.
    pinMode(MOTOR_PWM_PIN, OUTPUT);
    pinMode(MOTOR_DIR_PIN, OUTPUT);
    pinMode(MOTOR_ENABLE_PIN, OUTPUT);
    bridge.commands = xQueueCreate(8, sizeof(fy::CommandMessage));
    bridge.events = xQueueCreate(12, sizeof(fy::MotorEvent));
    bridge.priorityCommands = xQueueCreate(4, sizeof(fy::CommandMessage));
    if (!bridge.commands || !bridge.events || !bridge.priorityCommands) {
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
