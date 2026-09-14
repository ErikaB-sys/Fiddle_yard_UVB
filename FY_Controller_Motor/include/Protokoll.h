#pragma once


/**
 * @file Protokoll.h
 * @brief Definitions for the controller's UART communication protocol.
 *
 * Contains command and response identifiers, message metadata, and the
 * command/response tables used by the controller. This protocol applies only
 * to UART communication; it is not used for I2C or SPI.
 */


/** Maximum length of a command received from UART, in bytes. */
#define MAX_COMMAND_LENGTH  5

constexpr uint8_t CMD_GET_STATUS   =   0x19   ;    // Command to get the status of the motor
constexpr uint8_t CMD_GET_ERROR    =   0x1A   ;    // Command to get the current error of the FY-Controller

constexpr uint8_t CMD_REFERENCE    =   0x16   ;    // Command to reference the motor position
constexpr uint8_t CMD_SET_SPEED    =   0x2B   ;    // Command to set the speed of the motor


constexpr uint8_t CMD_STOPP        =   0x01   ;    // Command to stop the motor
constexpr uint8_t CMD_GO           =   0x07   ;    // Command to start the motor   
constexpr uint8_t CMD_LEFT         =   0x0B   ;    // Command to move the motor left
constexpr uint8_t CMD_RIGHT        =   0x0D   ;    // Command to move the motor right

constexpr uint8_t CMD_SET_POSITION =   0x13   ;    // Command to set the target position of the motor
constexpr uint8_t CMD_GET_POSITION =   0x15   ;    // Command to get the current position of the motor

constexpr uint8_t CMD_SET_TRACK    =   0x1C   ;    // Command to set the track number of the motor
constexpr uint8_t CMD_GET_TRACK    =   0x23   ;    // Command to get the current track number of the motor
constexpr uint8_t CMD_HELP         =   0x25   ;    // Command to get the list of available commands

constexpr uint8_t CMD_SET_LOCAL    =   0x26   ;    // Command to set the controller to local mode
constexpr uint8_t CMD_SET_REMOTE   =   0x2A   ;    // Command to set the controller to remote mode



/** Maximum length of a response sent to UART, in bytes. */
#define MAX_RESPONSE_LENGTH        4

// General response codes.
/** Periodic status indicating that the controller is alive. */
constexpr uint8_t STATUS_Alive                  =  0x03  ;     // Status indicating that the controller is alive Cyclically send this status to Uart every ???ms
/** Error response containing the system status and three error bytes. */
constexpr uint8_t STATUS_Error                  =  0xF0  ;     // Sending system Status byte and 3 Byte indicating that an error has occurred. The 3 Byte following up  will tell the Modul error Byte
/** Response containing the controller's system status. */
constexpr uint8_t  STATUS_System                =  0x10;
/** Response to local/remote mode commands. */
constexpr uint8_t  STATUS_CMD                   =  0x15;


/** Response containing the current motor position. */
constexpr uint8_t STATUS_Position               =  0x20    ;   // Status ID Position Information
/** Response containing reference-operation information. */
constexpr uint8_t STATUS_Reference              =  0x30   ;    // Status ID  Reference Information
/** Response containing track information. */
constexpr uint8_t STATUS_Track                  =  0x40    ;   // Status ID  Track informatiom
/** Response containing motor information. */
constexpr uint8_t STATUS_Motor                  =  0x50;    //Status ID Motor information
/** Acknowledgement response for movement commands. */
constexpr uint8_t STATUS_ACK                    =  0x60;      // Answer on moving Commands  if accepted
constexpr uint8_t STATUS_NACK                    =  0x60;     // Answer on moving Commands if Busy
/** Response indicating that the help text has been sent. */
constexpr uint8_t STATUS_Help                   =  0x18   ;    // Status indicating that the list of available commands has been sent to Uart



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
/** Metadata describing a supported command and its response. */
// ---------------------------------------------------------

struct CommandDefinition
{
    uint8_t id;                 ///< Command identifier byte.
    CommandType type;           ///< Command scheduling type.
    uint8_t telegramLength;     ///< Total telegram length including command byte
    uint8_t response;           ///< Response status identifier.
  
};

// ---------------------------------------------------------
/** Metadata describing a response status and its payload length. */
// ---------------------------------------------------------

struct ResponseDefinition
{
    uint8_t id;       ///< Response status identifier byte.
    uint8_t length;   ///< Response length in bytes.
};
// ---------------------------------------------------------
// Command IDs
// ---------------------------------------------------------

// ---------------------------------------------------------
/** Definitions of all commands supported by the UART protocol. */
// ---------------------------------------------------------


const CommandDefinition commandDefinitions[] =
{    // Command,CommandType,Length CMD, Response
    { CMD_GET_STATUS    ,  CommandType::IMMEDIATE, 1, STATUS_System },
    { CMD_GET_ERROR     , CommandType:: IMMEDIATE, 1, STATUS_Error },
    { CMD_GET_POSITION  , CommandType:: IMMEDIATE, 1, STATUS_Position }, 
    { CMD_GET_TRACK     , CommandType:: IMMEDIATE, 1, STATUS_Track},
    { CMD_HELP          , CommandType:: IMMEDIATE, 1, STATUS_Help }, 
    { CMD_REFERENCE     , CommandType:: EXECUTE,   1, STATUS_Reference },
    { CMD_SET_SPEED     , CommandType:: EXECUTE,   3, STATUS_Motor }, 
    { CMD_GO            , CommandType:: EXECUTE,   1, STATUS_ACK   },
    { CMD_LEFT          , CommandType:: EXECUTE,   1, STATUS_ACK   },   
    { CMD_RIGHT         , CommandType:: EXECUTE,   1, STATUS_ACK   }, 
    { CMD_SET_POSITION  , CommandType:: EXECUTE,   1, STATUS_ACK   }, 
    { CMD_SET_TRACK     , CommandType:: EXECUTE,   1, STATUS_ACK   }, 
    { CMD_SET_REMOTE    , CommandType:: EXECUTE,   1, STATUS_CMD   }, 
    { CMD_SET_LOCAL     , CommandType:: EXECUTE,   1, STATUS_CMD   },   
    { CMD_STOPP         , CommandType:: PRIORITY,  1, STATUS_System}
  
};
constexpr size_t COMMAND_COUNT = sizeof(commandDefinitions) / sizeof(commandDefinitions[0]);

/** Definitions of all response statuses supported by the UART protocol. */
const ResponseDefinition ResponseDefinitions[] =
{
{ STATUS_Alive  , 0   },
{ STATUS_Error ,  4  },
{ STATUS_System ,  2  },
{ STATUS_CMD ,   1 },
{ STATUS_Position ,   4 },
{ STATUS_Reference , 1   },
{ STATUS_Track,   3 },
{ STATUS_Motor, 3   },
{ STATUS_ACK,  1  },
{ STATUS_Help, 1  }  // ! help send a long String 
};

