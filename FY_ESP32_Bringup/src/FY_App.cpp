#include "FY_Bringup.h"
#include <freertos/task.h>

namespace fy {
namespace {
BringupContext context{nullptr, nullptr};
}
bool startBringup() {
    Serial.begin(115200);
    motorMockInit();
    context.commands = xQueueCreate(8, sizeof(CommandMessage));
    context.events = xQueueCreate(8, sizeof(MotorEvent));
    if (!context.commands || !context.events) {
        Serial.println("FY_TEST: failed to create queues");
        return false;
    }
    const BaseType_t motorStarted = xTaskCreatePinnedToCore(
        motorMockTask, TASK_MUNINN_MOTOR, 4096,
        &context, 3, nullptr, CORE_MUNINN);
    const BaseType_t uartStarted = xTaskCreatePinnedToCore(
        uartBringupTask, TASK_HUGINN_UART, 6144,
        &context, 2, nullptr, CORE_HUGINN);
    if (motorStarted != pdPASS || uartStarted != pdPASS) {
        Serial.println("FY_TEST: failed to create tasks");
        return false;
    }
    return true;
}
}
