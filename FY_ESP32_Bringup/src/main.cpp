#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// ESP32-WROOM dual-core bring-up. No FY motor outputs are driven.
#ifndef FY_STATUS_LED_PIN
#define FY_STATUS_LED_PIN 2  // Typical on-board LED, depends on dev board. -1 disables.
#endif
#ifndef FY_STATUS_LED_ACTIVE_HIGH
#define FY_STATUS_LED_ACTIVE_HIGH 1
#endif

constexpr bool statusLedEnabled = FY_STATUS_LED_PIN >= 0;
bool ledOn = false;
void writeStatusLed(bool on) {
    if (statusLedEnabled) {
        digitalWrite(FY_STATUS_LED_PIN, (on == (FY_STATUS_LED_ACTIVE_HIGH != 0)) ? HIGH : LOW);
    }
}

namespace {
enum class Op : uint8_t { Heartbeat };
struct Command { Op op; uint32_t sequence; };
struct Response { uint32_t sequence; uint8_t motorCore; };
QueueHandle_t commandQueue = nullptr;
QueueHandle_t responseQueue = nullptr;

void motorTask(void *) {
    Command cmd{};
    for (;;) {
        if (xQueueReceive(commandQueue, &cmd, portMAX_DELAY) == pdTRUE) {
            // In the real port: the sole owner of motion state, sensors and motor GPIO.
            Response result{cmd.sequence, static_cast<uint8_t>(xPortGetCoreID())};
            (void)xQueueSend(responseQueue, &result, 0);
        }
    }
}

void communicationTask(void *) {
    uint32_t seq = 0;
    TickType_t lastWake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(1000));
        Command cmd{Op::Heartbeat, ++seq};
        if (xQueueSend(commandQueue, &cmd, 0) != pdTRUE) {
            Serial.println("Bridge: command queue full");
            continue;
        }
        Response response{};
        if (xQueueReceive(responseQueue, &response, pdMS_TO_TICKS(100)) == pdTRUE) {
            ledOn = !ledOn;
            writeStatusLed(ledOn);
            Serial.printf("Bridge seq=%lu: comm core=%d, motor core=%u\n",
                          static_cast<unsigned long>(response.sequence),
                          xPortGetCoreID(), response.motorCore);
        } else {
            Serial.println("Bridge: response timeout");
        }
    }
}
}

void setup() {
    Serial.begin(115200);
    if (statusLedEnabled) {
        // GPIO 2 is a boot strapping pin: only use the board's own LED.
        pinMode(FY_STATUS_LED_PIN, OUTPUT);
        writeStatusLed(false);
    }
    commandQueue = xQueueCreate(8, sizeof(Command));
    responseQueue = xQueueCreate(8, sizeof(Response));
    if (!commandQueue || !responseQueue) {
        Serial.println("Bridge: queue allocation failed");
        return;
    }
    const BaseType_t motorOk = xTaskCreatePinnedToCore(
        motorTask, "FY_Motor", 4096, nullptr, 3, nullptr, 1);
    const BaseType_t commOk = xTaskCreatePinnedToCore(
        communicationTask, "FY_Comm", 4096, nullptr, 2, nullptr, 0);
    if (motorOk != pdPASS || commOk != pdPASS) {
        Serial.println("Bridge: task creation failed");
    }
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
