#include "FY_Bringup.h"
#include <freertos/task.h>

#ifndef FY_STATUS_LED_PIN
#define FY_STATUS_LED_PIN 2
#endif
#ifndef FY_STATUS_LED_ACTIVE_HIGH
#define FY_STATUS_LED_ACTIVE_HIGH 1
#endif

namespace fy {
namespace {
void setLed(bool on) {
#if FY_STATUS_LED_PIN >= 0
    digitalWrite(FY_STATUS_LED_PIN,
                 (on == (FY_STATUS_LED_ACTIVE_HIGH != 0)) ? HIGH : LOW);
#else
    (void)on;
#endif
}


}
void motorMockInit() {
#if FY_STATUS_LED_PIN >= 0
    pinMode(FY_STATUS_LED_PIN, OUTPUT);
    setLed(false);
#endif
}
void motorMockTask(void* arg) {
    auto* context = static_cast<BringupContext*>(arg);
    CommandMessage cmd{};
    bool ledState = false;
    for (;;) {
        if (xQueueReceive(context->commands, &cmd, portMAX_DELAY) == pdTRUE) {
            // Simulation only: no position, state machine, sensor or movement.
            ledState = !ledState;
            setLed(ledState);
            MotorEvent response{cmd.sequence, cmd.id,
                                static_cast<uint8_t>(xPortGetCoreID()), true};
            (void)xQueueSend(context->events, &response, pdMS_TO_TICKS(10));
        }
    }
}


}
