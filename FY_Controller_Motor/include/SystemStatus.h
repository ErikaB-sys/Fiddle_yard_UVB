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


struct FY_SystemStatus_t
{
    FY_SystemState_t state{FY_SystemState_t::INIT};
    FY_SystemInitStatus_t init;
};