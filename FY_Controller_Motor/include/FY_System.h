#pragma once

#include <Arduino.h>
#include "Protokoll.h"

/**
 * @brief Represents the current operating state of the system controller.
 */
enum class FY_SystemState_t : uint8_t
{
    INIT,   ///< System has not completed initialization.
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
};

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
    bool     Refrence;     ///< Reference or calibration flag.
};

/**
 * @brief Stores the current and target track information.
 */
struct FY_Trak_t
{
    uint8_t target_trak{1}; ///< Requested destination track.
    uint8_t akt_track{1};   ///< Currently active track.
};
struct FY_Command_t
{

    uint8_t id;
    uint8_t data[MAX_COMMAND_LENGTH];
    bool pending;
};


