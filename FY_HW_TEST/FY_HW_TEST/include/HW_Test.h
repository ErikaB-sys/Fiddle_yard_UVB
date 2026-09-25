#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// FY_HW_TEST - central configuration and test state
// -----------------------------------------------------------------------------

// Timing
constexpr uint32_t BUTTON_INTERVAL_MS  = 20;
constexpr uint32_t SWITCH_INTERVAL_MS  = 50;
constexpr uint32_t DISPLAY_INTERVAL_MS = 500;
constexpr uint32_t SERIAL_INTERVAL_MS  = 500;

// Button events
enum ButtonEvent
{
    BUTTON_NONE,
    BUTTON_STOP,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_GO
};

// Hardware pins
constexpr uint8_t LED1 = 2;
constexpr uint8_t LED2 = 3;
constexpr uint8_t LED3 = 4;
constexpr uint8_t LED_BUILTIN_PIN = LED_BUILTIN;

constexpr uint8_t LSL  = 9;
constexpr uint8_t LSR  = 12;
constexpr uint8_t LSREF = 11;
constexpr uint8_t LS4  = 10;

// PCF8574
constexpr uint8_t PORT_EXPANDER_ADDRESS = 0x27;

// Button / LED mapping on PCF8574
constexpr uint8_t BUTTON_STOP_PIN  = P7;
constexpr uint8_t BUTTON_GO_PIN    = P6;
constexpr uint8_t BUTTON_LEFT_PIN  = P5;
constexpr uint8_t BUTTON_RIGHT_PIN = P4;

constexpr uint8_t LED_STOP_PIN  = P3;
constexpr uint8_t LED_GO_PIN    = P2;
constexpr uint8_t LED_LEFT_PIN  = P1;
constexpr uint8_t LED_RIGHT_PIN = P0;

// Central test state
struct HWTestState
{
    // One-shot button event
    ButtonEvent button = BUTTON_NONE;

    // Action currently in progress
    ButtonEvent activeButton = BUTTON_NONE;
    bool busy = false;

    bool ena  = false;
    bool dir  = false;
    bool puls = false;

    bool limitLeft  = false;
    bool limitRight = false;
    bool reference  = false;
    bool hall       = false;

    uint16_t stepReload = 0;
    uint16_t frequency  = 0;
    uint32_t steps      = 0;

    bool displayDirty = true;
};

// Function interface
void scanI2C();
void initOLED();
void init_expander();

ButtonEvent read_buttons();
void read_end_switches();

void update_outputs();
void update_display();
void update_serial();
void set_active_button(ButtonEvent event);
void clear_active_button();
