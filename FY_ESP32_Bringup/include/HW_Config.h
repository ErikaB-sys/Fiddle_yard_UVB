#pragma once
// HW2: ESP32-WROOM pin allocation and electrical purpose.
// GPIO numbers refer to the ESP32 board marking Gxx.

#include <Arduino.h>

// A4988 driver logic; STEP is intended for hardware-timer-driven pulses.
constexpr uint8_t MOTOR_PWM_PIN    = 25; // G25 -> PULS / STEP (output)
constexpr uint8_t MOTOR_DIR_PIN    = 26; // G26 -> DIR (output)
constexpr uint8_t MOTOR_ENABLE_PIN = 27; // G27 -> ENA (output, LOW enables)

// A4988 microstep resolution selection (HW2 wired 20 Oct 2026).
// HW2 M0/M1/M2 correspond to A4988 MS1/MS2/MS3 respectively.
// All HIGH = 1/16 microstep; these are owned by the Muninn motor module.
constexpr uint8_t MOTOR_MS0_PIN = 13; // G13 -> M0 / A4988 MS1
constexpr uint8_t MOTOR_MS1_PIN = 14; // G14 -> M1 / A4988 MS2
constexpr uint8_t MOTOR_MS2_PIN =  4; // G4  -> M2 / A4988 MS3 (ESP32 strap-sampled GPIO)

// Planned light barriers / switches (inputs, not yet wired on HW2).
// Confirm every sensor has a 0..3.3 V output BEFORE connecting.
constexpr uint8_t LSREF_PIN = 32; // G32 -> reference light barrier 1
constexpr uint8_t LS4_PIN   = 33; // G33 -> reference light barrier 2 / belt
constexpr uint8_t LSL_PIN   = 16; // G16 -> left end stop
constexpr uint8_t LSR_PIN   = 17; // G17 -> right end stop

// Reserved for optional local I2C keyboard / display (not connected).
constexpr uint8_t I2C_SDA_PIN = 21; // G21 -> I2C SDA
constexpr uint8_t I2C_SCL_PIN = 22; // G22 -> I2C SCL

// Optional SPI building/display extension: RESERVED, NOT IMPLEMENTED.
// constexpr uint8_t SPI_SCLK_PIN = 18; // G18 -> SPI clock
// constexpr uint8_t SPI_MISO_PIN = 19; // G19 -> SPI input
// constexpr uint8_t SPI_MOSI_PIN = 23; // G23 -> SPI output
// Display-CS, MCP23S17-CS, and DC GPIOs are not assigned yet.
// Proposed DE-9: GND, 3V3, SCLK, MOSI, MISO, two CS, DC, reserve.
// 12 V light power separately wired; common GND at a defined point.
