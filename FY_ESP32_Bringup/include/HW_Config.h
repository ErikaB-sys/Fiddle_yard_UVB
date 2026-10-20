#pragma once
// FY HW2 / ESP32-WROOM wiring. No AVR port or timer-register definitions.
#include <Arduino.h>

// Motor driver A4988/TB6600 logic lines (HW2 verified LED sequence).
constexpr uint8_t MOTOR_PWM_PIN = 25;     // PULS / STEP
constexpr uint8_t MOTOR_DIR_PIN = 26;
constexpr uint8_t MOTOR_ENABLE_PIN = 27;  // A4988 active LOW; HIGH = off

// Planned FY sensor inputs; verify 3.3 V output levels before connection.
constexpr uint8_t LSREF_PIN = 32;
constexpr uint8_t LS4_PIN   = 33; // Timing belt / second reference gate
constexpr uint8_t LSL_PIN   = 16;
constexpr uint8_t LSR_PIN   = 17;

constexpr uint8_t SWITCH_REF_ID         = 0x01;
constexpr uint8_t SWITCH_TRIM_LEFT_ID   = 0x02;
constexpr uint8_t SWITCH_TRIM_RIGHT_ID  = 0x03;
constexpr uint8_t SWITCH_TIMING_BELT_ID = 0x04;

constexpr uint8_t DISPLAY_ADDRESS = 0x3C;
constexpr uint8_t PORT_EXPANDER_ADDRESS = 0x27;
constexpr uint8_t I2C_SDA_PIN = 21; // Reserved; not wired on HW2
constexpr uint8_t I2C_SCL_PIN = 22; // Reserved; not wired on HW2

// Nano-compatible PCF8574 assignments (not connected yet).
constexpr uint8_t BUTTON_STOP_PIN = 7;
constexpr uint8_t BUTTON_LEFT_PIN = 6;
constexpr uint8_t BUTTON_GO_PIN = 5;
constexpr uint8_t BUTTON_RIGHT_PIN = 4;
constexpr uint8_t LED_STOP_PIN = 3;
constexpr uint8_t LED_LEFT_PIN = 2;
constexpr uint8_t LED_GO_PIN = 1;
constexpr uint8_t LED_RIGHT_PIN = 0;

// OPTIONAL future SPI extension (Issue #165), deliberately not enabled:
// constexpr uint8_t SPI_SCLK_PIN = 18;
// constexpr uint8_t SPI_MISO_PIN = 19;
// constexpr uint8_t SPI_MOSI_PIN = 23;
// constexpr uint8_t SPI_DISPLAY_CS_PIN = ...; // assign after GPIO review
// constexpr uint8_t SPI_EXPANDER_CS_PIN = ...;
// constexpr uint8_t SPI_DISPLAY_DC_PIN = ...;
// DE-9 connector: 1 GND, 2 3V3, 3 SCLK, 4 MOSI, 5 MISO,
// 6 CS Display, 7 CS MCP23S17, 8 DC, 9 reserve/CS3.
// 12 V lighting power separately wired; common GND at defined point.
