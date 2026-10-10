#pragma once

#include <avr/pgmspace.h>
#include "Protokoll.h"
#include "BabelFish.h"
#include "Error.h"

/**
 * @brief References to controller data exchanged through the UART interface.
 */
struct UART_Context_t
{
    FY_SystemStatus_t* systemStatus;
    Error*      error;
    int32_t*    motorPosition;
    FY_Track_t* Track_INFO;
    uint16_t*   motorSpeed;
};

enum class UART_CommandStatus_t : uint8_t
{
    VALID,
    CMD_INVALID,
    DATA_INVALID,
    CRC_INVALID
};

// UART error definitions belong to the UART module. The central Error class
// stores these module-local codes and manages the active error list.
constexpr uint8_t UART_ERROR_NO_CONTENT =
    FY_ERROR_LOCATION_UART | 0x01U;
constexpr uint8_t UART_ERROR_NO_CONNECTION =
    FY_ERROR_LOCATION_UART | 0x02U;
constexpr uint8_t UART_ERROR_UNKNOWN_COMMAND =
    FY_ERROR_LOCATION_UART | 0x03U;
constexpr uint8_t UART_ERROR_INVALID_LENGTH =
    FY_ERROR_LOCATION_UART | 0x04U;
constexpr uint8_t UART_ERROR_INVALID_DATA =
    FY_ERROR_LOCATION_UART | 0x05U;
constexpr uint8_t UART_ERROR_INVALID_TELEGRAM =
    FY_ERROR_LOCATION_UART | 0x06U;
constexpr uint8_t UART_ERROR_CRC =
    FY_ERROR_LOCATION_UART | 0x07U;
constexpr uint8_t UART_ERROR_INVALID_STATE =
    FY_ERROR_LOCATION_UART | 0x08U;

// UART communication speed: 115200 baud, 8 data bits, no parity, 1 stop bit (8N1).
#define UART_BAUD_RATE 115200

/**
 * @brief UART communication handler.
 *
 * UART owns the serial input and the completed command buffer.
 * BabelFish is used as the ASCII decoder and writes directly into
 * that command buffer.
 */
class UART
{
public:
    UART();

    void begin(UART_Context_t& context, FY_ModuleContext_t& modules);

    /** @brief Processes incoming commands and outgoing responses. */
    void update();

    UART_CommandStatus_t getCommandStatus();
    void clearCommandStatus();

private:
    bool commandReady = false;

    UART_Context_t* UART_context;
    FY_ModuleContext_t* FY_ModuleContext;

    struct CommandBuffer_t
    {
        BabelFishCommand_t command;
        CommandType type;
        UART_CommandStatus_t status;
    };

    CommandBuffer_t CommandBuffer;

    struct ResponseBuffer_t
    {
        uint8_t id;
        uint8_t data[MAX_RESPONSE_LENGTH];
        PGM_P string;
        uint8_t length;
        ResponseContainer container;
        bool responsePending;
    };

    ResponseBuffer_t ResponseBuffer;

    BabelFish _babelFish;

    /** @brief Receives and decodes one or more available serial characters. */
    void receive();

    /** @brief Validates the decoded command against the command table. */
    bool validateCommand();

    /** @brief Decodes the received command. */
    void decodeCommand();

    /** @brief Sends the pending response. */
    void sendResponse();

    /** @brief Queues a response for transmission. */
    void setResponse(uint8_t id, const uint8_t* data, uint8_t length);
    void setResponse(uint8_t id, PGM_P data);

    /** @brief Sends a temporary NACK carrying the receive status. */
    void sendNack(UART_CommandStatus_t status);

    bool CheckError();
    bool CheckBusy();
    bool SetCommand();

    uint8_t Calc_CRC(uint8_t id, const uint8_t* data, uint8_t length);
    uint8_t Calc_CRC_PGM(uint8_t id, PGM_P data, uint8_t length);

    void sendHello();

    void setError(uint8_t errorCode);
    void clearError();

    void decodeImmediate();
    void decodeExecute();
    void decodePriority();

    void handleGetStatus();
    void handleGetError();
    void handleGetPosition();
    void handleGetTrack();
    void handleHelp();
    void handleGetFirmware();

    void handleReference();
    void handleSetSpeed();
    void handleGo();
    void handleLeft();
    void handleRight();
    void handleSetPosition();
    void handleSetTrack();
    void handleSetRemote();
    void handleSetLocal();

    void handleStop();
    void handle_Busy();
    void handle_ACK();
};
