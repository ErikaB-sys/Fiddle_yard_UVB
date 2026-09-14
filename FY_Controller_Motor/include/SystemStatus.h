#pragma once

#include <Arduino.h>



enum class FY_SystemState_t : uint8_t
{
    INIT,
    Idle,
    Busy,
    Error
};


struct FY_SystemInitStatus_t
{
    bool displayConnected{false};
    bool portExpanderConnected{false};
    bool uartConnected{false};

    bool motorInitialized{false};
    bool buttonsInitialized{false};
    bool switchesInitialized{false};

    bool systemError{false};
};

struct FY_System_Error_t
{
  bool uart {false};
  bool motor {false};
  bool switches{false};
};


struct FY_SystemStatus_t
{   FY_SystemInitStatus_t   init;
    FY_SystemState_t        state{FY_SystemState_t::INIT};
    FY_System_Error_t       error;
};

struct FY_Movement
{
    uint16_t  POS_Min;
    uint16_t  POS_Max;
    uint8_t  Speed_min;
    uint8_t  Speed_max;
    bool     Refrence;
};
/// @brief 
struct FY_Trak_t
{
  uint8_t target_trak{1};
  uint8_t akt_track{1} ; 
};
