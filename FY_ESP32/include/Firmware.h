#pragma once
// ESP32 version of the FY firmware response text. No AVR PROGMEM dependency.
// The on-wire STATUS_Firmware format stays ID | ASCII payload | CRC8.
#ifndef FW_GIT_COMMIT
#define FW_GIT_COMMIT "unknown"
#endif
#define FY_FW_NAME "FY_ESP32"
#define FY_FW_VERSION "2.0.0"
static constexpr char FW_RESPONSE[] =
    "FW: " FY_FW_NAME "\n"
    "VER: " FY_FW_VERSION "\n"
    "BUILD: " __DATE__ " " __TIME__ "\n"
    "GIT: " FW_GIT_COMMIT;
static constexpr char HELP_RESPONSE[] =
    "Ta det med ro, hjelpen er p\xC3\xA5 vei! Spickzettel: "
    "https://github.com/ErikaB-sys/Fiddle_yard_UVB/blob/FY_Command_and_Responses/"
    "FY_Controller_Motor/src/UART_Command_Spickzettel.md";
