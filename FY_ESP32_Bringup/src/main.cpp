#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "Protokoll.h"
#include "BabelFish.h"

// Safe phase-1 prototype: unchanged Nano BabelFish and Protokoll.h.
// No motor GPIO or actual movement is enabled.
#ifndef FY_STATUS_LED_PIN
#define FY_STATUS_LED_PIN 2
#endif
#ifndef FY_STATUS_LED_ACTIVE_HIGH
#define FY_STATUS_LED_ACTIVE_HIGH 1
#endif

namespace {
// Huginn (thought): communications and command interpretation on Core 0.
// Muninn (memory): motor state and motion handling on Core 1.
constexpr BaseType_t CORE_HUGINN = 0;
constexpr BaseType_t CORE_MUNINN = 1;
constexpr char TASK_HUGINN_UART[] = "Huginn_UART";
constexpr char TASK_MUNINN_MOTOR[] = "Muninn_Motor";

struct CommandMessage {
    uint8_t id;
    uint8_t data[MAX_COMMAND_LENGTH - 1];
    uint8_t length;   // command length includes ID, excludes CRC (Nano BabelFish semantics)
    uint32_t sequence;
};
struct MotorEvent {
    uint32_t sequence;
    uint8_t commandId;
    uint8_t core;
    bool accepted;
};
QueueHandle_t commands = nullptr;
QueueHandle_t events = nullptr;
BabelFish babelFish;
uint32_t nextSequence = 0;

void setLed(bool on) {
#if FY_STATUS_LED_PIN >= 0
    digitalWrite(FY_STATUS_LED_PIN,
                 (on == (FY_STATUS_LED_ACTIVE_HIGH != 0)) ? HIGH : LOW);
#else
    (void)on;
#endif
}

const CommandDefinition* lookup(uint8_t cmd) {
    for (size_t i = 0; i < COMMAND_COUNT; ++i) {
        if (commandDefinitions[i].id == cmd) return &commandDefinitions[i];
    }
    return nullptr;
}

uint8_t crc8(uint8_t id, const uint8_t* data, uint8_t count) {
    uint8_t crc = id;
    for (uint8_t i = 0; i < count; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x80U) ?
                static_cast<uint8_t>((crc << 1U) ^ 0x07U) :
                static_cast<uint8_t>(crc << 1U);
        }
    }
    return crc;
}

void motorTask(void*) {
    CommandMessage cmd{};
    bool ledState = false;
    for (;;) {
        if (xQueueReceive(commands, &cmd, portMAX_DELAY) == pdTRUE) {
            // Simulation only: no position, state machine, sensor or movement.
            ledState = !ledState;
            setLed(ledState);
            MotorEvent response{cmd.sequence, cmd.id,
                                static_cast<uint8_t>(xPortGetCoreID()), true};
            (void)xQueueSend(events, &response, pdMS_TO_TICKS(10));
        }
    }
}

void uartTask(void*) {
    babelFish.begin(115200); // Original Nano behavior: echo and ASCII translation.
    Serial.println("FY ESP32: Huginn (UART) <-> Muninn (motor mock), NO MOTION");
    for (;;) {
        while (Serial.available() > 0) {
            BabelFishCommand_t decoded{};
            const BabelFishResult result =
                babelFish.process(static_cast<char>(Serial.read()), decoded);
            if (result != BabelFishResult::COMMAND_READY) continue;

            const CommandDefinition* def = lookup(decoded.cmd);
            const bool valid =
                (def != nullptr) &&
                (decoded.length > 0) &&
                (static_cast<uint8_t>(decoded.length - 1) == def->dataLength) &&
                (decoded.crc == crc8(decoded.cmd, decoded.data,
                                     static_cast<uint8_t>(decoded.length - 1)));

            // During bring-up, status reports are diagnostic TEXT, not final
            // binary FY ACK/NACK frames. No invented protocol response format.
            if (!valid) {
                Serial.println("FY_TEST: Nano protocol table validation rejected command");
                continue;
            }

            CommandMessage request{};
            request.id = decoded.cmd;
            request.length = decoded.length;
            request.sequence = ++nextSequence;
            memcpy(request.data, decoded.data, sizeof(request.data));
            if (xQueueSend(commands, &request, 0) != pdTRUE) {
                Serial.println("FY_TEST: motor queue full");
            }
        }

        MotorEvent event{};
        while (xQueueReceive(events, &event, 0) == pdTRUE) {
            Serial.printf("FY_TEST: seq=%lu cmd=0x%02X Muninn_core=%u accepted=%u\n",
                static_cast<unsigned long>(event.sequence), event.commandId,
                event.core, event.accepted ? 1U : 0U);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
}

void setup() {
    Serial.begin(115200);
#if FY_STATUS_LED_PIN >= 0
    pinMode(FY_STATUS_LED_PIN, OUTPUT);
    setLed(false);
#endif
    commands = xQueueCreate(8, sizeof(CommandMessage));
    events = xQueueCreate(8, sizeof(MotorEvent));
    if (!commands || !events) {
        Serial.println("FY_TEST: failed to create queues");
        return;
    }
    if (xTaskCreatePinnedToCore(motorTask, TASK_MUNINN_MOTOR, 4096,
                                nullptr, 3, nullptr, CORE_MUNINN) != pdPASS ||
        xTaskCreatePinnedToCore(uartTask, TASK_HUGINN_UART, 6144,
                                nullptr, 2, nullptr, CORE_HUGINN) != pdPASS) {
        Serial.println("FY_TEST: failed to create tasks");
    }
}
void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }
