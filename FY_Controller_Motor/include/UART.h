#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include "SystemStatus.h"
#include "Config.h"
#include "Protokoll.h"

/**
 * @brief References to controller data exchanged through the UART interface.
 */
struct UART_Context_t
{
    FY_SystemStatus_t* systemStatus;
    uint8_t*  uartError;
    int32_t*  motorPosition;
    uint8_t*  motorTrack;
    uint16_t* motorSpeed;
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

enum class ReceiveState : uint8_t
{
    FindCommand,
    ReadData,
    CheckCRC
};


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
    // Internal Variable 
 
    uint8_t UART_Error;

    UART_Context_t* UART_context; ///< Controller data used by the UART handler.
    
    uint8_t commandLength;
    uint8_t expectedLength;
    struct CommandBuffer_t
    {
        uint8_t      cmd;
        uint8_t      data[MAX_COMMAND_LENGTH - 1];
        uint8_t      CMD_CRC;
        CommandType type;
        uint8_t     response;
        bool CMD_valid;
    };
     CommandBuffer_t CommandBuffer; 

      struct ResponseBuffer_t
     {
         uint8_t  id;
         uint8_t* data;
         uint8_t  length;
         bool     responsePending;
     };
     ResponseBuffer_t ResponseBuffer;

    uint8_t dataIndex;
     ReceiveState receiveState = ReceiveState::FindCommand;
    /** @brief Receives data from the UART interface. */
    void receive();

    /** @brief Decodes the received command. */
    void decodeCommand();

    /** @brief Sends a response to the remote device. */
    void sendResponse();
    void setResponse(uint8_t id, uint8_t* data, uint8_t length);

  /** @brief übertragung der Fahrbefehle  */
    bool CheckError(); // wenn störung motor dann auch keine Bewegung 
    bool CheckBusy();    // wenn bewegung  dann kein anderer Befehl ausser Stop 
    bool SetCommand();   // eintragen der daten in Movment struct und valid flag setzen wenn gültig rückgabe  ok / nok für antwort 

    /**@brief Check and create CRC  */
    bool Check_CRC();
    uint8_t Calc_CRC(uint8_t id,const uint8_t* data,uint8_t length); 


    /** @brief Sends the UART handshake message. */
    void sendHello();
    /**@brief Decode funktionen  für jeden CMD  type */
    void decodeImmediate(); 
    void decodeExecute();
    void decodePriority();





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
    void Handle_Busy();
};