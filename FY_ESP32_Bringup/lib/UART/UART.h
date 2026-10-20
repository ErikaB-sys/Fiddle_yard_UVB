#pragma once
#include "FY_Bringup.h"
#include "BabelFish.h"
#include "Error.h"

namespace fy {

// Protocol owner on core 0. Only this class reads or writes Serial.
// Source behavior: FY_Controller_Motor/lib/UART/UART.cpp (Nano).
class UART {
public:
    void begin(BringupContext* context);
    void update();

private:
    struct ResponseBuffer_t {
        uint8_t id = 0;
        uint8_t data[MAX_RESPONSE_LENGTH]{};
        const char* string = nullptr; // Flash is memory-mapped on ESP32.
        uint8_t length = 0;
        ResponseContainer container = ResponseContainer::Data;
        bool responsePending = false;
    };

    BringupContext* _context = nullptr;
    BabelFish _babelFish;
    BabelFishCommand_t _command{};
    ResponseBuffer_t _response{};
    Error _errors;
    uint32_t _nextSequence = 0;

    void receive();
    bool validateCommand(const CommandDefinition*& definition);
    void decodeCommand(const CommandDefinition& definition);
    void decodeImmediate();
    void decodeExecute();
    void decodePriority();
    void collectMotorResponses();
    bool queueMotorCommand(bool priority);

    void handleGetStatus();
    void handleGetError();
    void handleGetPosition();
    void handleGetTrack();
    void handleHelp();
    void handleGetFirmware();
    void handleSetMode();

    void sendResponse();
    void setResponse(uint8_t id, const uint8_t* data, uint8_t length);
    void setResponse(uint8_t id, const char* text);
    void ack(uint8_t cmd);
    void nack(uint8_t location, NackReason reason);
};

BaseType_t Huginn_startUART(BringupContext* context);
}
