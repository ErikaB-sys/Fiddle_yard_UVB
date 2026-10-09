#pragma once

// HW_Config.h
// Fixed hardware configuration of the FY controller.

#include <Arduino.h>

// I2C hardware addresses
constexpr uint8_t DISPLAY_ADDRESS = 0x3C;
constexpr uint8_t PORT_EXPANDER_ADDRESS = 0x20;

// DESIGN PROPOSAL — PCB variant detection (not implemented yet):
// Keep one firmware for both controller PCB variants by assigning the I2C
// port expander a different hardware address on each variant.
// - Existing PCB: keep the currently configured expander address (0x20).
// - Stepper-driver PCB: use a distinct address (for example 0x21), set by
//   the expander's address pins and verified against the actual wiring.
// At startup, probe the known variant addresses and select the PCB profile.
// Only the stepper-driver profile may configure/use the three Arduino pins
// connected to the driver's step-resolution inputs; those pins remain unused
// on the existing PCB. No address probing result must be treated as a valid
// variant unless the expected expander is actually detected.

// Arduino Nano hardware: I2C pins are fixed.
constexpr uint8_t I2C_SDA_PIN = A4;
constexpr uint8_t I2C_SCL_PIN = A5;

// Switch IDs
constexpr uint8_t SWITCH_REF_ID         = 0x01;
constexpr uint8_t SWITCH_TRIM_LEFT_ID   = 0x02;
constexpr uint8_t SWITCH_TRIM_RIGHT_ID  = 0x03;
constexpr uint8_t SWITCH_TIMING_BELT_ID = 0x04;

// Analog channels for switches
constexpr uint8_t NO_ANALOG = 0xFF;
constexpr uint8_t SWITCH_REF_A         = NO_ANALOG;
constexpr uint8_t SWITCH_TRIM_LEFT_A   = NO_ANALOG;
constexpr uint8_t SWITCH_TRIM_RIGHT_A  = NO_ANALOG;
constexpr uint8_t SWITCH_TIMING_BELT_A = NO_ANALOG;

// Digital pins for switches
constexpr uint8_t SWITCH_REF_D         = PB1; // D09
constexpr uint8_t SWITCH_TRIM_LEFT_D   = PB2; // D10
constexpr uint8_t SWITCH_TRIM_RIGHT_D  = PB3; // D11
constexpr uint8_t SWITCH_TIMING_BELT_D = PB4; // D12

// Motor control pins
constexpr uint8_t MOTOR_PWM_PIN    = PD3; // white wire
constexpr uint8_t MOTOR_DIR_PIN    = PD4; // yellow wire
constexpr uint8_t MOTOR_ENABLE_PIN = PD2; // blue wire

// Virtual pins for the I2C port expander.
// Values >= 100 avoid conflicts with normal Arduino pins.
constexpr uint8_t BUTTON_STOP  = 100;
constexpr uint8_t BUTTON_GO    = 101;
constexpr uint8_t BUTTON_LEFT  = 102;
constexpr uint8_t BUTTON_RIGHT = 103;
constexpr uint8_t LED1 = 104;
constexpr uint8_t LED2 = 105;
constexpr uint8_t LED3 = 106;
constexpr uint8_t LED4 = 107;
