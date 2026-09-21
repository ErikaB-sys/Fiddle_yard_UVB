#pragma once

/*
 * FY Controller – system-wide error code definition
 *
 * Reference: Issue #80
 *
 * Error code format: one byte
 *
 *   bit 7..4 = error location
 *   bit 3..0 = error type
 *
 * The sections below define the 16 available error locations.
 * The concrete error types are intentionally left open and will be
 * added as the flowcharts and implementation define the actual NOK cases.
 *
 * Location numbers are documented as 0x01 .. 0x10.
 * When combined into the final one-byte error code, the location occupies
 * the upper nibble:
 *
 *   final_code = (location << 4) | error_type
 *
 * 0x00 = no error
 * 0xF0 .. 0xFF = reserved
 */

// -----------------------------------------------------------------------------
// Fehlerstatus – zentrale API
// -----------------------------------------------------------------------------

constexpr uint8_t FY_ERROR_NONE = 0x00;

/*
 * Zentraler Systemfehler.
 *
 * Module setzen Fehler über setError().
 * Der aktuelle Fehler kann über getError() abgefragt werden.
 * clearError() setzt den Fehlerzustand wieder auf FY_ERROR_NONE.
 */
void setError(uint8_t errorCode);
uint8_t getError();
bool hasError();
void clearError();

// -----------------------------------------------------------------------------
// Fehler 0x01 – UART (Modul: UART/Protokoll)
// -----------------------------------------------------------------------------
//
// Fehlerart 0x01 .. 0x07
//
constexpr uint8_t FY_ERROR_UART_NOT_AVAILABLE   = 0x11;
constexpr uint8_t FY_ERROR_UART_INVALID_COMMAND = 0x12;
constexpr uint8_t FY_ERROR_UART_INVALID_LENGTH  = 0x13;
constexpr uint8_t FY_ERROR_UART_INVALID_DATA    = 0x14;
constexpr uint8_t FY_ERROR_UART_INVALID_TELEGRAM= 0x15;
constexpr uint8_t FY_ERROR_UART_TIMEOUT         = 0x16;
constexpr uint8_t FY_ERROR_UART_CRC             = 0x17; // falls CRC verwendet wird

// -----------------------------------------------------------------------------
// Fehler 0x02 – Motor (Modul: motor.cpp)
// -----------------------------------------------------------------------------
//
// Fehlerart 0x01 .. 0x07
//
constexpr uint8_t FY_ERROR_MOTOR_NOT_ACTIVE       = 0x21;
constexpr uint8_t FY_ERROR_MOTOR_TARGET_RANGE     = 0x22;
constexpr uint8_t FY_ERROR_MOTOR_INVALID_TRACK    = 0x23;
constexpr uint8_t FY_ERROR_MOTOR_INVALID_JOB      = 0x24;
constexpr uint8_t FY_ERROR_MOTOR_INVALID_PARAM    = 0x25;
constexpr uint8_t FY_ERROR_MOTOR_INVALID_PROFILE  = 0x26;
constexpr uint8_t FY_ERROR_MOTOR_PROFILE_FAILED   = 0x27;

// -----------------------------------------------------------------------------
// Fehler 0x03 – Referenzierung (Modul: Referenzierungslogik)
// -----------------------------------------------------------------------------
//
// Fehlerart 0x01 .. 0x06
//
constexpr uint8_t FY_ERROR_REF_START_FAILED       = 0x31;
constexpr uint8_t FY_ERROR_REF_SENSOR_NOT_FOUND   = 0x32;
constexpr uint8_t FY_ERROR_REF_POSITION_INVALID   = 0x33;
constexpr uint8_t FY_ERROR_REF_TIMEOUT            = 0x34;
constexpr uint8_t FY_ERROR_REF_ABORTED            = 0x35;
constexpr uint8_t FY_ERROR_REF_REQUIRED           = 0x36;

// -----------------------------------------------------------------------------
// Fehler 0x04 – Endschalter / Positionssensorik (Modul: Endschalter/Sensorik)
// -----------------------------------------------------------------------------
//
// Noch offen – wird mit der konkreten Sensorlogik definiert.
//
// -----------------------------------------------------------------------------
// Fehler 0x05
// -----------------------------------------------------------------------------
//
// Fehlerarten werden hier später ergänzt.
//

// -----------------------------------------------------------------------------
// Fehler 0x06
// -----------------------------------------------------------------------------
//
// Fehlerarten werden hier später ergänzt.
//

// -----------------------------------------------------------------------------
// Fehler 0x07
// -----------------------------------------------------------------------------
//
// Fehlerarten werden hier später ergänzt.
//

// -----------------------------------------------------------------------------
// Fehler 0x08
// -----------------------------------------------------------------------------
//
// Fehlerarten werden hier später ergänzt.
//

// -----------------------------------------------------------------------------
// Fehler 0x09
// -----------------------------------------------------------------------------
//
// Fehlerarten werden hier später ergänzt.
//

// -----------------------------------------------------------------------------
// Fehler 0x0A
// -----------------------------------------------------------------------------
//
// Fehlerarten werden hier später ergänzt.
//

// -----------------------------------------------------------------------------
// Fehler 0x0B
// -----------------------------------------------------------------------------
//
// Fehlerarten werden hier später ergänzt.
//

// -----------------------------------------------------------------------------
// Fehler 0x0C
// -----------------------------------------------------------------------------
//
// Fehlerarten werden hier später ergänzt.
//

// -----------------------------------------------------------------------------
// Fehler 0x0D
// -----------------------------------------------------------------------------
//
// Fehlerarten werden hier später ergänzt.
//

// -----------------------------------------------------------------------------
// Fehler 0x0E
// -----------------------------------------------------------------------------
//
// Fehlerarten werden hier später ergänzt.
//

// -----------------------------------------------------------------------------
// Fehler 0x0F
// -----------------------------------------------------------------------------
//
// Fehlerarten werden hier später ergänzt.
//

// -----------------------------------------------------------------------------
// Fehler 0x10
// -----------------------------------------------------------------------------
//
// Fehlerarten werden hier später ergänzt.
//
