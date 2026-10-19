#include <Arduino.h>
#include <freertos/task.h>
#include "FY_Bringup.h"
#include "UART.h"
#include "Motor.h"

namespace {
fy::BringupContext bridge{nullptr, nullptr};
}

void setup() {
    Serial.begin(115200);
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
}

void loop() {
    // Huginn and Muninn own their work loops. Arduino loop is idle.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
