#pragma once

// HW_Config.h
// Fixed hardware configuration of the FY controller.

#include <Arduino.h>

// I2C hardware addresses
constexpr uint8_t DISPLAY_ADDRESS = 0x3C;
constexpr uint8_t PORT_EXPANDER_ADDRESS = 0x27;

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

// Physical sensor definitions: Arduino pin and fast ISR access are kept together.
// Normal code uses .pin; the Timer1 ISR uses .inputRegister and .mask.
struct FastInputPin
{
    uint8_t pin;
    volatile uint8_t* inputRegister;
    uint8_t mask;
};

// Arduino Nano ATmega328P: D9=PB1, D12=PB4, D11=PB3, D10=PB2.
constexpr FastInputPin LSR   = {9,  &PINB, _BV(PB1)};
constexpr FastInputPin LSL   = {12, &PINB, _BV(PB4)};
constexpr FastInputPin LSREF = {11, &PINB, _BV(PB3)};
constexpr FastInputPin LS4   = {10, &PINB, _BV(PB2)};

// Motor control pins
constexpr uint8_t MOTOR_PWM_PIN    = PD3; // white wire
constexpr uint8_t MOTOR_DIR_PIN    = PD4; // yellow wire
constexpr uint8_t MOTOR_ENABLE_PIN = PD2; // blue wire

// Physical PCF8574 pin numbers (P0..P7), validated against FY_HW_TEST wiring.
constexpr uint8_t BUTTON_STOP_PIN  = 7;
constexpr uint8_t BUTTON_LEFT_PIN  = 6;
constexpr uint8_t BUTTON_GO_PIN    = 5;
constexpr uint8_t BUTTON_RIGHT_PIN = 4;

constexpr uint8_t LED_STOP_PIN  = 3;
constexpr uint8_t LED_LEFT_PIN  = 2;
constexpr uint8_t LED_GO_PIN    = 1;
constexpr uint8_t LED_RIGHT_PIN = 0;

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
