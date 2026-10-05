#pragma once
// Library includes
#include <Arduino.h>

// Config.h
// Main hardware configuration of the FY controller.

// UART options
//#define UART_USE_CRC_TX
//#define UART_USE_CRC_RX

// Debug options
//#define DebugSwitches

// I2C hardware addresses
constexpr uint8_t DISPLAY_ADDRESS = 0x3C;
constexpr uint8_t PORT_EXPANDER_ADDRESS = 0x20;

// Arduino Nano hardware: I2C pins are fixed.
// Keep Arduino pin macros outside initializer lists for MISRA 12.3.
constexpr uint8_t I2C_SDA_PIN = A4;
constexpr uint8_t I2C_SCL_PIN = A5;

// Switch names
#define SWITCH_REF_NAME          "SWITCH_REF"
#define SWITCH_TRIM_LEFT_NAME    "SWITCH_TRIM_LEFT"
#define SWITCH_TRIM_RIGHT_NAME   "SWITCH_TRIM_RIGHT"
#define SWITCH_TIMING_BELT_NAME  "SWITCH_TIMING_BELT"

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
