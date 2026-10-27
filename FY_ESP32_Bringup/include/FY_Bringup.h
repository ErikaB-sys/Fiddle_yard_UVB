#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "Protokoll.h"

namespace fy {
constexpr BaseType_t CORE_HUGINN = 0;
constexpr BaseType_t CORE_MUNINN = 1;
struct CommandMessage {
    uint8_t id;
    uint8_t data[MAX_COMMAND_LENGTH - 1];
    uint8_t length;
    uint32_t sequence;
};
struct MotorEvent {
    uint32_t sequence;
    uint8_t commandId;
    uint8_t core;
    uint8_t responseId;    // STATUS_ACK or STATUS_NACK
    uint8_t responseData;  // command ID (ACK) or location|reason (NACK)
};
struct BringupContext {
    QueueHandle_t commands;
    QueueHandle_t events;
    QueueHandle_t priorityCommands; // independent PRIORITY mailbox (STOP)
    QueueHandle_t keyboardEvents;
    QueueHandle_t sensorSnapshots; // independent 1-byte, length-1 mailbox: Muninn -> Huginn
};
}
