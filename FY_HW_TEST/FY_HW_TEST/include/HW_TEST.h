#pragma once

#include <Arduino.h>

// ------------------------------------------------------------
// Timing
// ------------------------------------------------------------

constexpr uint32_t BUTTON_INTERVAL_MS  = 20;
constexpr uint32_t SWITCH_INTERVAL_MS  = 50;
constexpr uint32_t DISPLAY_INTERVAL_MS = 500;
constexpr uint32_t SERIAL_INTERVAL_MS  = 500;


// ------------------------------------------------------------
// Button events
// ------------------------------------------------------------

enum ButtonEvent
{
    BUTTON_NONE,
    BUTTON_STOP,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_GO
};


// ------------------------------------------------------------
// HW-Test state
// ------------------------------------------------------------

struct HWTestState
{
    ButtonEvent button = BUTTON_NONE;

    bool ena  = false;
    bool dir  = false;
    bool puls = false;

    bool limitLeft  = false;
    bool limitRight = false;
    bool hall       = false;

    uint16_t stepReload = 0;

    bool displayDirty = true;
};