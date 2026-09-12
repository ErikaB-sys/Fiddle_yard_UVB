#pragma once


// Protokoll.h
// This file contains the definitions for the communication protocol used in the project.
// It defines the structure of the messages exchanged between the controller and other components.
// Only for Uart communication, not for I2C or SPI.


// ---------------------------------------------------------
// Command types
// ---------------------------------------------------------

enum class CommandType : uint8_t
{
    IMMEDIATE,
    EXECUTE,
    PRIORITY
};

// ---------------------------------------------------------
// Command definition
// ---------------------------------------------------------

struct CommandDefinition
{
    uint8_t id;
    CommandType type;
    uint8_t telegramLength;  //  telegramLength = DataLength 
    uint8_t response;
  
};

// ---------------------------------------------------------
// Response Definition
// ---------------------------------------------------------

struct ResponseDefinition
{
    uint8_t id;
    uint8_t length;
};
// ---------------------------------------------------------
// Command IDs
// ---------------------------------------------------------

// ---------------------------------------------------------
//define the commands and responses for the communication protocol
// ---------------------------------------------------------


const CommandDefinition commandDefinitions[] =
{    // Command,CommandType,Length CMD, Response,length Response
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

const ResponseDefinition ResponseDefinitions[] =
{
{ STATUS_Alive  , 0   },
{ STATUS_Error ,  4  },
{ STATUS_System ,  2  },
{ STATUS_CMD ,   1 },
{ STATUS_Position ,   3 },
{ STATUS_Reference , 1   },
{ STATUS_Track,   3 },
{ STATUS_Motor, 3   },
{ STATUS_ACK,  1  },
{ STATUS_Help, 1  }  // ! help send a long String 
};



//received commands from Uart are stored in a struct and processed by the main loop
#define MAX_COMMAND_LENGTH  8         // Maximum length of a command received from Uart in Bytes

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



// Send Status to Uart
#define MAX_RESPONSE_LENGTH        4             // Maximum length of a response sent to Uart in Bytes
//general Response Codes
constexpr uint8_t STATUS_Alive                  =  0x03  ;     // Status indicating that the controller is alive Cyclically send this status to Uart every ???ms
//resonse  for CMD_GET_ERROR 
constexpr uint8_t STATUS_Error                  =  0xF0  ;     // Sending system Status byte and 3 Byte indicating that an error has occurred. The 3 Byte following up  will tell the Modul error Byte
//response for CMD_GET_STATUS;
constexpr uint8_t  STATUS_System                =  0x10;
//response for CMD_SET_LOCAL / CMD_SET_REMOTE
constexpr uint8_t  STATUS_CMD                   =  0x15;


//response for CMD_GET_POSITION in case ref=ence has not been done yet the response will be STATUS_ReferenceNotDONE
constexpr uint8_t STATUS_Position               =  0x20    ;   // Status ID Position Information
//response for CMD_REFERENCE
constexpr uint8_t STATUS_Reference              =  0x30   ;    // Status ID  Reference Information
//response for CMD_GET_TRACK /CMD_SET_TRACK=
constexpr uint8_t STATUS_Track                  =  0x40    ;   // Status ID  Track informatiom
constexpr uint8_t STATUS_Motor                  =  0x50;    //Status ID Motor information
constexpr uint8_t STATUS_ACK                    =  0x60;     // Answer on mosving Commands 

//response for CMD_HELP
constexpr uint8_t STATUS_Help                   =  0x18   ;    // Status indicating that the list of available commands has been sent to Uart

