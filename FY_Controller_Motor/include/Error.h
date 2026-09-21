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
// Fehlerarten werden hier später ergänzt.
//

// -----------------------------------------------------------------------------
// Fehler 0x02 – Motor (Modul: motor.cpp)
// -----------------------------------------------------------------------------
//
// Fehlerarten werden hier später ergänzt.
//

// -----------------------------------------------------------------------------
// Fehler 0x03 – Referenzierung (Modul: Referenzierungslogik)
// -----------------------------------------------------------------------------
//
// Fehlerarten werden hier später ergänzt.
//

// -----------------------------------------------------------------------------
// Fehler 0x04 – Endschalter / Positionssensorik (Modul: Endschalter/Sensorik)
// -----------------------------------------------------------------------------
//
// Fehlerarten werden hier später ergänzt.
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
