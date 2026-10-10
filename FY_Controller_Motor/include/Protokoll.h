#pragma once

#include <Arduino.h>

/**
 * @file Protokoll.h
 * @brief Definitions for the controller's UART communication protocol.
 *
 * Contains command and response identifiers, message metadata, and the
 * command/response tables used by the controller. This protocol applies only
 * to UART communication; it is not used for I2C or SPI.
 */


/**
 * @brief UART payload byte order.
 *
 * Multi-byte values are transmitted least-significant byte first.
 * Byte 0 is always the first transmitted byte and contains the least
 * significant 8 bits of a multi-byte value. Higher-order bytes follow
 * at increasing byte indices.
 *
 * Example:
 * uint16_t 0x1234 -> byte 0 = 0x34, byte 1 = 0x12
 *
 * The TransportContainer carries raw bytes. The individual command handler
 * is responsible for interpreting those bytes according to the command
 * definition.
 */

// Protocol options
//#define UART_USE_CRC_TX
//#define UART_USE_CRC_RX

/** Maximum length of a command received from UART, in bytes. */
#define MAX_COMMAND_LENGTH  5

constexpr uint8_t CMD_GET_STATUS   =   0x19   ;    // Command to get the status of the motor
constexpr uint8_t CMD_GET_ERROR    =   0x1A   ;    // Command to get the current error of the FY-Controller

constexpr uint8_t CMD_REFERENCE    =   0x16   ;    // Command to reference the motor position
constexpr uint8_t CMD_SET_PARAM    =   0x2B   ;    // Issue: #145 | Set a motor/controller parameter

constexpr uint8_t CMD_STOPP        =   0x01   ;    // Command to stop the motor
constexpr uint8_t CMD_GO           =   0x07   ;    // Command to start the motor
constexpr uint8_t CMD_LEFT         =   0x0B   ;    // Command to move the motor left
constexpr uint8_t CMD_RIGHT        =   0x0D   ;    // Command to move the motor right

constexpr uint8_t CMD_SET_POSITION =   0x13   ;    // Command to set the target position of the motor
constexpr uint8_t CMD_GET_POSITION =   0x15   ;    // Command to get the current position of the motor

constexpr uint8_t CMD_SET_TRACK    =   0x1C   ;    // Command to set the track number of the motor
constexpr uint8_t CMD_GET_TRACK    =   0x23   ;    // Query track and movement state (Issue #58)

// GET_TRACK request: one option byte, 0 = default query.
constexpr uint8_t GET_TRACK_OPTION_DEFAULT = 0x00;

// STATUS_Track DATA[2] values.
constexpr uint8_t TRACK_STATUS_REACHED = 0x00;
constexpr uint8_t TRACK_STATUS_MOVING  = 0x01;
constexpr uint8_t TRACK_STATUS_ERROR   = 0x02;
constexpr uint8_t CMD_HELP         =   0x25   ;    // Command to get the list of available commands
constexpr uint8_t CMD_GET_FIRMWARE =   0x27   ;    // Command to get firmware identity information

constexpr uint8_t CMD_SET_MODE     =   0x26   ;    // Command to select LOCAL/REMOTE mode (see Issue #58)


/**
 * @brief Maximum length of a binary response payload.
 *
 * The payload length describes the DATA container only. A complete UART
 * telegram consists of three parts:
 *
 *   ID | DATA | CRC
 *
 * Therefore, the total telegram length is calculated from the DATA length
 * plus the ID and CRC fields.
 */
#define MAX_RESPONSE_LENGTH        4

/** Container type of a response payload. */
enum class ResponseContainer : uint8_t
{
    Data,
    String
};

// General response codes.
/** Periodic status indicating that the controller is alive. */
constexpr uint8_t STATUS_Alive                  =  0x03  ;     // Status indicating that the controller is alive Cyclically send this status to Uart every ???ms
/** Error response containing the system status and three error bytes. */
constexpr uint8_t STATUS_Error                  =  0xF0  ;     // Sending system Status byte and 3 Byte indicating that an error has occurred. The 3 Byte following up  will tell the Modul error Byte
/** Response containing the controller's system status. */
constexpr uint8_t  STATUS_System                = 0x10;
/** Response to local/remote mode commands. */
constexpr uint8_t  STATUS_CMD                   = 0x15;


/** Response containing the current motor position. */
constexpr uint8_t STATUS_Position               = 0x20    ;   // Status ID Position Information
/** Response containing reference-operation information. */
constexpr uint8_t STATUS_Reference              = 0x30   ;    // Status ID  Reference Information
/** Response containing track information. */
constexpr uint8_t STATUS_Track                  = 0x40    ;   // Status ID  Track informatiom
/** Response containing motor information. */
constexpr uint8_t STATUS_Motor                  = 0x50;    //Status ID Motor information
constexpr uint8_t STATUS_Switches               = 0x41;
constexpr uint8_t STATUS_Keyboard               = 0x51;
constexpr uint8_t STATUS_SELECTOR_SYSTEM        = 0x10;
constexpr uint8_t STATUS_SELECTOR_MOTOR         = 0x20;
constexpr uint8_t STATUS_SELECTOR_SWITCHES      = 0x40;
constexpr uint8_t STATUS_SELECTOR_KEYBOARD      = 0x50;
/**
 * Reserved development response. This response reports an unexpected
 * implementation state; it is not a normal operational response.
 * DATA[0] = module code, DATA[1] = development error code.
 */
constexpr uint8_t DEV_RESPONSE = 0xFF;
constexpr uint8_t DEV_MODULE_UART = 0x01;
constexpr uint8_t DEV_ERROR_NULL_POINTER = 0x01;

/** Acknowledgement response for movement commands. */
constexpr uint8_t STATUS_ACK                    = 0x60;      // Answer on moving Commands  if accepted
constexpr uint8_t STATUS_NACK                    = 0x70;     // Issue: #143 | Negative acknowledgement with one-byte reason code
/** Response indicating that the help text has been sent. */
constexpr uint8_t STATUS_Help                   = 0x18   ;    // Status indicating that the list of available commands has been sent to Uart



// ---------------------------------------------------------
/** How a received command is scheduled for execution. */
// ---------------------------------------------------------

enum class CommandType : uint8_t
{
    IMMEDIATE,  ///< Execute as soon as the command is received.
    EXECUTE,    ///< Queue for normal command execution.
    PRIORITY    ///< Execute with priority over queued commands.
};

// ---------------------------------------------------------
/** Metadata describing a supported command. */
// ---------------------------------------------------------

struct CommandDefinition
{
    uint8_t id;                 ///< Command identifier byte.
    CommandType type;           ///< Command scheduling type.
    uint8_t dataLength;          ///< DATA length only; ID and CRC are not included
     bool requiresReference;      /// need reference run

};

// ---------------------------------------------------------
/** Metadata describing a response status and its payload length. */
// ---------------------------------------------------------

struct ResponseDefinition
{
    uint8_t id;                     ///< Response status identifier byte.
    uint8_t length;                 ///< Response length in bytes for binary data.
    ResponseContainer container;    ///< Response payload container type.
};
// ---------------------------------------------------------
// Command IDs
// ---------------------------------------------------------

// ---------------------------------------------------------
/** Definitions of all commands supported by the UART protocol. */
// ---------------------------------------------------------


const CommandDefinition commandDefinitions[] =
{
    // Command,          CommandType,          DATA length, Requires reference
    { CMD_GET_STATUS,    CommandType::IMMEDIATE, 1,false }, // Issue: #114 | Status: Coded constructed
    { CMD_GET_ERROR,     CommandType::IMMEDIATE, 0,false }, // Issue: #115 | Status: Coded constructed
    { CMD_GET_POSITION,  CommandType::IMMEDIATE, 0,false }, // Issue: #116 | Status: Coded constructed
    { CMD_GET_TRACK,     CommandType::IMMEDIATE, 1,false }, // Issue: #117 | Status: Coded constructed
    { CMD_HELP,          CommandType::IMMEDIATE, 0,false }, // Issue: #118 | Status: Coded constructed
    { CMD_GET_FIRMWARE,  CommandType::IMMEDIATE, 0,false }, // Issue: #119 | Status: Coded constructed
    { CMD_REFERENCE,     CommandType::EXECUTE,   1,false }, // Issue: #120 | Status: Coded constructed
    { CMD_SET_PARAM,     CommandType::EXECUTE,   4,false }, // Issue: #145 | Status: Coded constructed
    { CMD_GO,            CommandType::EXECUTE,   2, true }, // Issue: #122 | Status: Coded constructed
    { CMD_LEFT,          CommandType::EXECUTE,   3, true }, // Issue: #123 | Status: Coded constructed
    { CMD_RIGHT,         CommandType::EXECUTE,   3, true }, // Issue: #124 | Status: Coded constructed
    { CMD_SET_POSITION,  CommandType::EXECUTE,   3, true }, // Issue: #125 | Status: Coded constructed
    { CMD_SET_TRACK,     CommandType::EXECUTE,   1, true }, // Issue: #126 | Status: Coded constructed
    { CMD_SET_MODE,      CommandType::EXECUTE,   2,false }, // Issue: #58 | Status: Coded constructed
    { CMD_STOPP,         CommandType::PRIORITY,  0,false } // Issue: #129 | Status: Coded constructed
};
constexpr size_t COMMAND_COUNT = sizeof(commandDefinitions) / sizeof(commandDefinitions[0]);

/** Definitions of all response statuses supported by the UART protocol. */
const ResponseDefinition ResponseDefinitions[] =
{
{ STATUS_Alive,     0, ResponseContainer::Data }, // Issue: #134 | Status: Defined
{ STATUS_Error,     4, ResponseContainer::Data }, // Issue: #135 | Status: Coded constructed
{ STATUS_System,    2, ResponseContainer::Data }, // Issue: #136 | Status: Coded constructed
{ STATUS_CMD,       1, ResponseContainer::Data }, // Issue: #137 | Status: Defined
{ STATUS_Position,  4, ResponseContainer::Data }, // Issue: #138 | Status: Coded constructed
{ STATUS_Reference, 1, ResponseContainer::Data }, // Issue: #139 | Status: Defined
{ STATUS_Track,     3, ResponseContainer::Data }, // DATA[0]=target, DATA[1]=last confirmed track, DATA[2]=movement status | Issue: #140 | Status: Coded constructed
{ STATUS_Motor,     3, ResponseContainer::Data }, // Issue: #141 | Status: Coded constructed
{ STATUS_Switches,  1, ResponseContainer::Data }, // Issue: #114 | Status: Coded constructed
{ STATUS_Keyboard,  1, ResponseContainer::Data }, // Issue: #114 | Status: Coded constructed
{ STATUS_ACK,       1, ResponseContainer::Data }, // Issue: #142 | Status: Defined
{ STATUS_NACK,      1, ResponseContainer::Data }, // Issue: #143 | Status: Coded constructed
{ DEV_RESPONSE,     2, ResponseContainer::Data } // Reserved development response: module code + error code
{ STATUS_Help,      0, ResponseContainer::String } // Issue: #144 | Status: Coded constructed
};


// Command data exchanged through the UART protocol.
struct FY_Command_t
{
    uint8_t id;
    uint8_t data[MAX_COMMAND_LENGTH];
    bool pending;
};
