#pragma once
// Library includes
#include <Arduino.h>
// Config.h
// magig switche

///@brief uncommen if CRC check is activ 
//#define UART_USE_CRC_TX
//#define UART_USE_CRC_RX



// Project includes
#include "Display.h"
#include "UART.h" // also includes "Protokoll.h"
#include "Buttons.h"
#include "Switches.h"
#include "Motor.h"

// Configuration constants
// Modul configuration see  Modul header !
// Main configuration of the FY controller
// I2C address definitions (default)
#define DISPLAY_ADDRESS         0x3C
#define PORT_EXPANDER_ADDRESS   0x20 
/* A4 & A5  I²C Bus*/

// Pin definitions for switches, buttons, and LEDs on the Arduino board



// define of Switchnames 
#define SWITCH_REF_NAME          "SWITCH_REF"
#define SWITCH_TRIM_LEFT_NAME    "SWITCH_TRIM_LEFT"
#define SWITCH_TRIM_RIGHT_NAME   "SWITCH_TRIM_RIGHT"
#define SWITCH_TIMING_BELT_NAME  "SWITCH_TIMING_BELT"
// define of Switch IDs
#define SWITCH_REF_ID             0x01
#define SWITCH_TRIM_LEFT_ID      0x02
#define SWITCH_TRIM_RIGHT_ID     0x03
#define SWITCH_TIMING_BELT_ID    0x04
// Analog channels for switches and buttons on the Arduino board
#define SWITCH_REF_A            A0
#define SWITCH_TRIM_LEFT_A      A1
#define SWITCH_TRIM_RIGHT_A     A2
#define SWITCH_TIMING_BELT_A    A3


// Digital pins for buttons and LEDs on the Arduino board
#define SWITCH_REF_D            D10
#define SWITCH_TRIM_LEFT_D      D11
#define SWITCH_TRIM_RIGHT_D     D12
#define SWITCH_TIMING_BELT_D    D13

// Motor control pins on the Arduino board
#define MOTOR_PWM_PIN           D3
#define MOTOR_DIR_PIN           D4
#define MOTOR_ENABLE_PIN        D5

// Virtual pins for the expander
// All pin numbers are defined as >=100 to avoid conflicts with normal Arduino pins

#define BUTTON_STOP     100
#define BUTTON_GO       101
#define BUTTON_LEFT     102
#define BUTTON_RIGHT    103
#define LED1            104
#define LED2            105
#define LED3            106
#define LED4            107




