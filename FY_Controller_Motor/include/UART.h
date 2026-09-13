#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include "Protokoll.h"

/**
 * @brief References to controller data exchanged through the UART interface.
 */
struct UART_Context_t
{
    uint8_t*  System_status;   ///< Pointer to the current controller status.
    uint8_t*  System_error;    ///< Pointer to the current controller error.
    uint8_t*  UART_error;
    int32_t*  Motor_position; ///< Pointer to the current position.
    uint8_t*  Motor_track;    ///< Pointer to the current track.
    uint16_t* Motor_speed;    ///< Pointer to the current speed.

};
// Speed of Uart communication is 115200 baud, 8 data bits, no parity, 1 stop bit (8N1).
#define UART_BAUD_RATE 115200
/**
 * @brief Refence for Uart Modul Error 
 */
/// @brief Error of the Uart modul
constexpr uint8_t UART_ERROR_NO_CONTENT      = 0x01;
constexpr uint8_t UART_ERROR_NO_CONNECTION   = 0x02;
constexpr uint8_t UART_ERROR_UNKNOWN_COMMAND = 0x04;
constexpr uint8_t UART_ERROR_INVALID_STATE   = 0x08;




/**
 * @brief Manages UART communication with the controller.
 */
class UART
{
public:
    /** @brief Constructs a UART communication handler. */
    UART();

    /**
     * @brief Initializes the handler with controller data.
     * @param context References to the controller state.
     */
    void begin(UART_Context_t& context);

    /** @brief Processes incoming commands and outgoing responses. */
    void update();

private:
    
    uint8_t UART_Error;

    UART_Context_t* UART_context; ///< Controller data used by the UART handler.
    uint8_t commandBuffer[MAX_COMMAND_LENGTH];
    uint8_t commandLength;
    uint8_t expectedLength;

    /** @brief Receives data from the UART interface. */
    void receive();

    /** @brief Decodes the received command. */
    void decodeCommand();

    /** @brief Sends a response to the remote device. */
    void sendResponse();

    /** @brief Sends the UART handshake message. */
    void sendHello();
    /** @brief Command Handler  */
    void handleGetStatus();
    void handleGetError();
    void handleGetPosition();
    void handleGetTrack();
    void handleHelp();

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
};