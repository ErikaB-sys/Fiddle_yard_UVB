#pragma once
// Library includes
#include <Arduino.h>
// Config.h
// magig switche

///@brief uncommen if CRC check is activ 
//#define UART_USE_CRC_TX
//#define UART_USE_CRC_RX
///@brief debug componets 
//#define DebugSwitches


// Project includes
#include <Arduino.h>
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
constexpr uint8_t SWITCH_REF_ID         =   0x01;
constexpr uint8_t SWITCH_TRIM_LEFT_ID   =   0x02;
constexpr uint8_t SWITCH_TRIM_RIGHT_ID  =   0x03;
constexpr uint8_t SWITCH_TIMING_BELT_ID =   0x04;
// Analog channels for switches and buttons on the Arduino board
constexpr uint8_t SWITCH_REF_A         =   A0 ;
constexpr uint8_t SWITCH_TRIM_LEFT_A   =   A1;
constexpr uint8_t SWITCH_TRIM_RIGHT_A  =   A2;
constexpr uint8_t SWITCH_TIMING_BELT_A =   A3;


// Digital pins for buttons and LEDs on the Arduino board
constexpr uint8_t SWITCH_REF_D         =  PB1; // D09
constexpr uint8_t SWITCH_TRIM_LEFT_D   =  PB2; // D10
constexpr uint8_t SWITCH_TRIM_RIGHT_D  =  PB3; // D11
constexpr uint8_t SWITCH_TIMING_BELT_D =  PB4; // D12

// Motor control pins on the Arduino board
constexpr uint8_t MOTOR_PWM_PIN      =     PD3;
constexpr uint8_t MOTOR_DIR_PIN      =     PD4;
constexpr uint8_t MOTOR_ENABLE_PIN   =     PD2;

// Virtual pins for the expander
// All pin numbers are defined as >=100 to avoid conflicts with normal Arduino pins

constexpr uint8_t BUTTON_STOP =    100;
constexpr uint8_t BUTTON_GO   =    101;
constexpr uint8_t BUTTON_LEFT =    102;
constexpr uint8_t BUTTON_RIGHT=    103;
constexpr uint8_t LED1        =    104;
constexpr uint8_t LED2        =    105;
constexpr uint8_t LED3        =    106;
constexpr uint8_t LED4        =    107;




