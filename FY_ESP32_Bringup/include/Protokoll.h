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
constexpr uint8_t CMD_SET_PARAM    =   0x2B   ;    // Issue #158: Parameter in RAM / SAVE via ID
constexpr uint8_t CMD_GET_PARAM    =   0x2C   ;    // Issue #158: einzelner Parameter per ID

/**
 * @brief Gemeinsame Parameter-IDs fuer GET_PARAM und SET_PARAM (Issue #158).
 *
 * Werte auf dem Draht: drei uint8 in Little Endian, unsigned (24 Bit).
 * A/B bezeichnen Beschleunigungs-/Bremssegmente; die Kennung bestimmt das
 * Vorzeichen im internen Profil. Kalibrierwerte sind ausschliesslich lesbar.
 * IDs sind ein stabiler Protokollvertrag; neue IDs nicht umnummerieren.
 *
 * Hinweis: Diese Definitionen bereiten nur das Protokoll vor. Die Handler
 * und die Anbindung der Motorparameter an den Timer folgen separat.
 */
enum class ParamId : uint8_t {
    V_MIN           = 0x01U,
    V_MAX           = 0x02U,
    V_POS           = 0x03U,
    V_REF_MIN       = 0x04U,
    V_REF           = 0x05U,
    A1              = 0x06U,
    A2              = 0x07U,
    B1              = 0x08U,
    B2              = 0x09U,
    A1_ST           = 0x0AU,
    A2_ST           = 0x0BU,
    B1_ST           = 0x0CU,
    B2_ST           = 0x0DU,

    POS_LSR         = 0x20U, // Nur GET: Messwert der langen Referenzfahrt
    POS_REF_A       = 0x21U, // Nur GET: erste raeumliche Fahnenflanke
    POS_REF_B       = 0x22U, // Nur GET: zweite raeumliche Fahnenflanke

    TRACK_BG1       = 0x30U,
    TRACK_BG2       = 0x31U,
    TRACK_BG3       = 0x32U,
    TRACK_BG4       = 0x33U,
    TRACK_BG5       = 0x34U,

    RESTORE_DEFAULT = 0x7EU, // Nur SET: DATA[1] = Bereich, DATA[2..3] = AA 55
    SAVE            = 0x7FU  // Nur SET: DATA[1] = Bereich, DATA[2..3] = AA 55
};

enum class ParamSaveArea : uint8_t {
    MOTOR_PARAMETERS = 0x02U,
    TRACK_POSITIONS  = 0x01U
    // CALIBRATION = 0x00: bewusst nicht per SAVE beschreibbar.
};
constexpr uint8_t STATUS_SELECTOR_NVM = 0x60U; // GET_STATUS 60: NVM-Snapshot
constexpr uint8_t PARAM_SAVE_PATTERN_1 = 0xAAU;
constexpr uint8_t PARAM_SAVE_PATTERN_2 = 0x55U;

// GET_PARAM: DATA[0] = ParamId; Antwort: STATUS_Parameter mit 4 DATA-Bytes.
// SET_PARAM: DATA[0] = ParamId, DATA[1..3] = uint24 LE.
// SAVE / RESTORE: DATA[0] = 0x7F / 0x7E, DATA[1] = ParamSaveArea,
//       DATA[2] = 0xAA, DATA[3] = 0x55 (nur ein Segment je Auftrag).

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
constexpr uint8_t STATUS_Motor                  = 0x50;
constexpr uint8_t STATUS_Parameter              = 0x2DU;
constexpr uint8_t STATUS_NVM                    = 0x61U; // error, faultMask, validMask // DATA: ParamId, value[0..2] LE    //Status ID Motor information
constexpr uint8_t STATUS_Switches               = 0x41;
constexpr uint8_t STATUS_Keyboard               = 0x51;
/** Response containing automatically generated firmware identity information. */
constexpr uint8_t STATUS_Firmware               = 0x28;
constexpr uint8_t STATUS_SELECTOR_SYSTEM        = 0x10;
constexpr uint8_t STATUS_SELECTOR_MOTOR         = 0x20;
// ESP32 extension: STATUS 30 queries reference truth (not a command).
constexpr uint8_t STATUS_SELECTOR_REFERENCE     = 0x30;
constexpr uint8_t REFERENCE_UNKNOWN             = 0x00;
constexpr uint8_t REFERENCE_REAL                = 0x01;
constexpr uint8_t REFERENCE_BENCH_FAKE          = 0x02;
constexpr uint8_t STATUS_SELECTOR_SWITCHES      = 0x40;
constexpr uint8_t STATUS_SELECTOR_KEYBOARD      = 0x50;
/**
 * Reserved development response for unexpected implementation states.
 * DATA[0]: upper nibble = module/location, lower nibble = development error.
 * DATA[1]: reserved for temporary local debugging; release value is 0x00.
 */
constexpr uint8_t DEV_RESPONSE = 0xFF;
constexpr uint8_t DEV_MODULE_UART = 0x10;       // Error-location nibble: UART
constexpr uint8_t DEV_ERROR_NULL_POINTER = 0x01; // Error-type nibble
constexpr uint8_t DEV_DEBUG_DATA_NONE = 0x00;    // Mandatory release value for DATA[1]

/** Acknowledgement response for movement commands. */
constexpr uint8_t STATUS_ACK                    = 0x60;      // Answer on moving Commands  if accepted
constexpr uint8_t STATUS_NACK                    = 0x70;     // Issue: #143 | Negative acknowledgement with one-byte reason code

/**
 * @brief Central NACK reason codes stored in DATA[0]'s low nibble.
 *
 * The high nibble identifies the responsible module/location using the
 * FY_ERROR_LOCATION_* constants from Error.h. NACK payloads are queued via
 * setResponse() and transmitted through the normal sendResponse() path.
 * NONE is reserved and must not be used for an actual NACK.
 */
enum class NackReason : uint8_t
{
    NONE              = 0x00,
    BUSY              = 0x01,
    NOT_READY         = 0x02,
    INVALID_COMMAND   = 0x03,
    INVALID_PARAMETER = 0x04,
    EXECUTION_ERROR   = 0x05,
    INVALID_TELEGRAM  = 0x06
};
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
    { CMD_GET_STATUS,    CommandType::IMMEDIATE, 1,false },
    { CMD_GET_PARAM,     CommandType::IMMEDIATE, 1,false }, // #158 | Protokoll definiert; Handler folgt // Issue: #114 | Status: Code working
    { CMD_GET_ERROR,     CommandType::IMMEDIATE, 0,false }, // Issue: #115 | Status: Code working
    { CMD_GET_POSITION,  CommandType::IMMEDIATE, 0,false }, // Issue: #116 | Status: Code constructed (snapshot wired; runtime refresh depends on motor loop)
    { CMD_GET_TRACK,     CommandType::IMMEDIATE, 1,false }, // Issue: #117 | Status: Code constructed (track snapshot connection pending)
    { CMD_HELP,          CommandType::IMMEDIATE, 0,false }, // Issue: #118 | Status: Code working
    { CMD_GET_FIRMWARE,  CommandType::IMMEDIATE, 0,false }, // Issue: #119 | Status: Code constructed (STATUS_Firmware)
    { CMD_REFERENCE,     CommandType::EXECUTE,   1,false }, // Issue: #120 | Status: Code constructed
    { CMD_SET_PARAM,     CommandType::EXECUTE,   4,false }, // #158 | SAVE ueber ParamId::SAVE // Issue: #145 | Status: Code constructed
    { CMD_GO,            CommandType::EXECUTE,   2,false }, // Issue: #122 | Motor enable/disable does not require a reference
    { CMD_LEFT,          CommandType::EXECUTE,   0, true }, // Issue: #123 | No DATA payload
    { CMD_RIGHT,         CommandType::EXECUTE,   0, true }, // Issue: #124 | No DATA payload
    { CMD_SET_POSITION,  CommandType::EXECUTE,   2, true }, // Issue: #125 | uint16 target position, little-endian
    { CMD_SET_TRACK,     CommandType::EXECUTE,   1, true }, // Issue: #126 | Status: Code constructed
    { CMD_SET_MODE,      CommandType::EXECUTE,   2,false }, // Issue: #58 | Status: Code working
    { CMD_STOPP,         CommandType::PRIORITY,  2,false } // Issue: #129 | Status: Code constructed (safety behavior pending)
};
constexpr size_t COMMAND_COUNT = sizeof(commandDefinitions) / sizeof(commandDefinitions[0]);

/** Definitions of all response statuses supported by the UART protocol. */
const ResponseDefinition ResponseDefinitions[] =
{
{ STATUS_Alive,     0, ResponseContainer::Data }, // Issue: #134 | Status: Defined
{ STATUS_Error,     4, ResponseContainer::Data }, // Issue: #135 | Status: Code working
{ STATUS_System,    2, ResponseContainer::Data }, // Issue: #136 | Status: Code working
{ STATUS_CMD,       1, ResponseContainer::Data }, // Issue: #137 | Status: Code working
{ STATUS_Position,  4, ResponseContainer::Data }, // Issue: #138 | Status: Code constructed (snapshot wired; runtime refresh depends on motor loop)
{ STATUS_Reference, 1, ResponseContainer::Data }, // STATUS 30: 0 none, 1 physically referenced, 2 bench-only FAKE
{ STATUS_Track,     3, ResponseContainer::Data }, // DATA[0]=target, DATA[1]=last confirmed track, DATA[2]=movement status | Issue: #140 | Status: Code constructed (track snapshot connection pending)
{ STATUS_Motor,     3, ResponseContainer::Data },
{ STATUS_Parameter, 4, ResponseContainer::Data },
{ STATUS_NVM,       3, ResponseContainer::Data }, // flag, fault mask, valid mask // Parameter-ID + 24-bit-LE-Wert // Issue: #141 | Status: Code working
{ STATUS_Switches,  1, ResponseContainer::Data }, // Issue: #114 | Status: Code working
{ STATUS_Keyboard,  1, ResponseContainer::Data }, // Issue: #114 | Status: Code working
{ STATUS_Firmware,  0, ResponseContainer::String }, // Automatically generated firmware identity
{ STATUS_ACK,       1, ResponseContainer::Data }, // Issue: #142 | Status: Code working
{ STATUS_NACK,      1, ResponseContainer::Data }, // Issue: #143 | Status: Code working
{ DEV_RESPONSE,     2, ResponseContainer::Data }, // DATA[0]=location|error; DATA[1]=0x00 in release
{ STATUS_Help,      0, ResponseContainer::String } // Issue: #144 | Norwegian help text + GitHub Spickzettel
};


// Command data exchanged through the UART protocol.
struct FY_Command_t
{
    uint8_t id;
    uint8_t data[MAX_COMMAND_LENGTH];
    bool pending;
};
