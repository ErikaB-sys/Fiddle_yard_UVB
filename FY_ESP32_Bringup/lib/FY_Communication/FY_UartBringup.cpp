#include "FY_Bringup.h"
#include "BabelFish.h"
#include <freertos/task.h>
#include <cstring>

namespace fy {
namespace {
BabelFish babelFish;
uint32_t nextSequence = 0;
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


}
void uartBringupTask(void* arg) {
    auto* context = static_cast<BringupContext*>(arg);
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
            if (xQueueSend(context->commands, &request, 0) != pdTRUE) {
                Serial.println("FY_TEST: motor queue full");
            }
        }

        MotorEvent event{};
        while (xQueueReceive(context->events, &event, 0) == pdTRUE) {
            Serial.printf("FY_TEST: seq=%lu cmd=0x%02X Muninn_core=%u accepted=%u\n",
                static_cast<unsigned long>(event.sequence), event.commandId,
                event.core, event.accepted ? 1U : 0U);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
}
