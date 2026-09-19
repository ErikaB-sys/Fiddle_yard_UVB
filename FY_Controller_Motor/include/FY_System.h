#pragma once

#include <Arduino.h>
#include "Protokoll.h"

/**
 * @brief Represents the current operating state of the system controller.
 */
enum class FY_SystemState_t : uint8_t
{
    INIT,   ///< System has not completed initialization.
    REF,    /// refenziert 
    Idle,   ///< System is waiting for commands.
    Busy,   ///< System is processing a task.
    Error   ///< System is in an error state.
};

/**
 * @brief Stores the initialization status of hardware and subsystems.
 */
struct FY_SystemInitStatus_t
{
    bool displayConnected{false};      ///< True if the display is connected and detected.
    bool portExpanderConnected{false}; ///< True if the I/O expander is connected.
    bool uartConnected{false};         ///< True if the UART communication interface is active.

    bool motorInitialized{false};      ///< True if the motor controller has been initialized.
    bool buttonsInitialized{false};    ///< True if button inputs have been configured.
    bool switchesInitialized{false};   ///< True if switch inputs have been configured.

    bool systemError{false};           ///< True if a fatal initialization or runtime error occurred.
};

/**
 * @brief Stores error flags for major system modules.
 */
struct FY_System_Error_t
{
    bool uart{false};      ///< UART communication error.
    bool motor{false};     ///< Motor driver or movement error.
    bool switches{false};  ///< Switch detection or input error.
    bool reference {false}; // refernce detection error
};

//**Refernrun  class */

enum class REF_STATE : uint8_t
{
    REF_REQ,       // Referenzierung erforderlich / angefordert
    REFR_EXEC,     // Referenzlauf läuft
    REF_VALID,     // Referenzierung erfolgreich
    REF_ERROR      // Referenzierung fehlgeschlagen
};

REF_STATE FY_Refrun = REF_STATE::REF_REQ;


/**
 * @brief Represents the overall runtime status of the controller.
 */
struct FY_SystemStatus_t
{
    FY_SystemInitStatus_t init;                            ///< Initialization state of modules.
    FY_SystemState_t      state{FY_SystemState_t::INIT};   ///< Current controller state.
    FY_System_Error_t     error;                           ///< Current system error flags.
};

/**
 * @brief Defines movement limits and speed settings for a motorized axis.
 */
struct FY_Movement
{
    uint16_t POS_Min;      ///< Minimum valid position.
    uint16_t POS_Max;      ///< Maximum valid position.
    uint8_t  Speed_min;    ///< Minimum movement speed.
    uint8_t  Speed_max;    ///< Maximum movement speed.
    bool     Reference;    ///< Reference or calibration flag.
};

/**
 * @brief Stores the current and target track information.
 */
/*
    Index gesehen vom Zufahrtsgleis

    ------- AB1 -------|
    ------- AB2 -------|
    ------- AB3 -------| ------ BG1 -----|
    ------- AB4 -------| ------ BG2 -----|
    ------- AB5 -------| ------ BG3 -----|--------- Zufahrt
    ------- AB6 -------| ------ BG4 -----|
    ------- AB7 -------| ------ BG5 -----|
    ------- AB8 -------|                 --- BB1 ---
    ------- AB9 -------|                 --- BB2 ---

    BG1 bedient AB1 bis AB5
    BG5 bedient AB5 bis AB9

    BG2, BG3 und BG4 sind Abstellgleise
    für lange Garnituren (max. 48 cm)
In der  Abbildung wäre akt_track BG3!

*/

enum class FY_Track : uint8_t
{
    BG1 = 1,
    BG2 = 2,
    BG3 = 3,
    BG4 = 4,
    BG5 = 5
};

struct FY_Track_t
{
    FY_Track target_track{FY_Track::BG1}; ///< Requested destination track.
    FY_Track akt_track   {FY_Track::BG1};    ///< Currently active track.
};  
struct FY_Command_t
{

    uint8_t id;
    uint8_t data[MAX_COMMAND_LENGTH];
    bool pending;
};


// Die brücke zwischen den Modulen  
// forwar declaration 
class Motor;
class UART;


struct FY_SystemStatus_t
{
    FY_SystemInitStatus_t init;
    FY_SystemState_t      state{FY_SystemState_t::INIT};
    FY_System_Error_t     error;
};


// Verbindungen zwischen den Modulen
struct FY_ModuleContext_t
{
    Motor* motor;
    UART*  uart;
    // später:
    // Display* display;
    // Buttons* buttons;
    // Switches* switches;
};


