#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// FY_HW_TEST - central configuration and test state
// -----------------------------------------------------------------------------

// Timing
constexpr uint32_t BUTTON_INTERVAL_MS  = 20;

// Adjustable STEP frequency for the hardware test.
// BLUE 1 / BLUE 2 select the next lower / higher test frequency while ENA is OFF.
constexpr uint16_t STEP_FREQUENCIES[] = {10, 25, 50, 100, 200, 500, 800};
constexpr uint8_t STEP_FREQUENCY_COUNT = sizeof(STEP_FREQUENCIES) / sizeof(STEP_FREQUENCIES[0]);
constexpr uint32_t SWITCH_INTERVAL_MS  = 50;
constexpr uint32_t DISPLAY_INTERVAL_MS = 200;
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

// Motor driver outputs
constexpr uint8_t MOTOR_ENA  = 2;
constexpr uint8_t MOTOR_PULS = 3;
constexpr uint8_t MOTOR_DIR  = 4;

constexpr uint8_t LED_BUILTIN_PIN = LED_BUILTIN;

// Physical sensor definition: Arduino pin and its fast ISR access live together.
// Normal code uses .pin; the Timer1 ISR uses .inputRegister/.mask.
struct FastInputPin
{
    uint8_t pin;
    volatile uint8_t* inputRegister;
    uint8_t mask;
};

constexpr FastInputPin LSR   = {9,  &PIND, _BV(PD1)};
constexpr FastInputPin LSL   = {12, &PINB, _BV(PB4)};
constexpr FastInputPin LSREF = {11, &PINB, _BV(PB3)};
constexpr FastInputPin LS4   = {10, &PINB, _BV(PB2)};

// PCF8574
constexpr uint8_t PORT_EXPANDER_ADDRESS = 0x27;

// Button / LED mapping on PCF8574
// Physical mapping, verified on the hardware:
//   STOP   -> button P7 / LED P1
//   BLUE 1 -> button P6 / LED P2
//   GREEN  -> button P5 / LED P3
//   BLUE 2 -> button P4 / LED P0
constexpr uint8_t BUTTON_STOP_PIN  = P7;
constexpr uint8_t BUTTON_LEFT_PIN  = P6;
constexpr uint8_t BUTTON_GO_PIN    = P5;
constexpr uint8_t BUTTON_RIGHT_PIN = P4;

constexpr uint8_t LED_STOP_PIN  = P3;
constexpr uint8_t LED_LEFT_PIN  = P2;
constexpr uint8_t LED_GO_PIN    = P1;
constexpr uint8_t LED_RIGHT_PIN = P0;

// Central hardware IO image
// All physical inputs are read in read_IO(). All other code works only
// with this image. Hardware outputs are applied in update_outputs().
struct HWIOState
{
    // Inputs
    bool buttonStop  = false;
    bool buttonLeft  = false;
    bool buttonRight = false;
    bool buttonGo    = false;

    bool limitLeft  = false;
    bool limitRight = false;
    bool reference  = false;
    bool hall       = false;

    // Outputs
    bool ledStop  = false;
    bool ledLeft  = false;
    bool ledRight = false;
    bool ledGo    = false;

    bool ena = false;
    bool dir = false;
};

extern HWIOState io;

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


// -----------------------------------------------------------------------------
// Mechanik-Messwerte
// -----------------------------------------------------------------------------

struct HWMeasurement
{
    uint16_t position = 0;
    uint16_t refStart = 0;
    uint16_t refEnd = 0;
    uint16_t refLength = 0;
    uint16_t totalLength = 0;
    bool refValid = false;
};

extern volatile HWMeasurement measurement;

void get_measurement(HWMeasurement& result);

// Function interface
void scanI2C();
void initOLED();
void init_expander();

void read_IO();
ButtonEvent read_buttons();
void read_end_switches();

void update_outputs();
void update_display();
void update_serial();
void set_active_button(ButtonEvent event);
void clear_active_button();
void init_step_timer();
void set_step_frequency(uint16_t frequency);
