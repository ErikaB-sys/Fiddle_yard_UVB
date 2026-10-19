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
    bool accepted;
};
struct BringupContext {
    QueueHandle_t commands;
    QueueHandle_t events;
};
}
