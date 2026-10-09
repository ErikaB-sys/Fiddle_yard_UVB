#pragma once

#include <Arduino.h>
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

constexpr uint8_t FY_ERROR_LOCATION_MASK = 0xF0;
constexpr uint8_t FY_ERROR_TYPE_MASK     = 0x0F;

constexpr uint8_t FY_ERROR_LOCATION_UART      = 0x10;
constexpr uint8_t FY_ERROR_LOCATION_MOTOR     = 0x20;
constexpr uint8_t FY_ERROR_LOCATION_REFERENCE = 0x30;
constexpr uint8_t FY_ERROR_LOCATION_SWITCHES  = 0x40;
constexpr uint8_t FY_ERROR_LOCATION_KEYBOARD  = 0x50;

/*
 * Zentrale Fehlerverwaltung.
 *
 * Aktueller Stand:
 * - Die Module definieren ihre eigenen Fehlercodes.
 * - Error verwaltet zentral die aktuell aktiven Fehler.
 * - Pro Fehlerort wird ein aktueller Fehler gespeichert.
 */
class Error
{
public:
    void begin();
    void update();

    void setError(uint8_t errorCode);
    void clearError(uint8_t errorLocation);
    uint8_t getError(uint8_t index) const;
    uint8_t getSystemByte() const;

private:
    static constexpr uint8_t MAX_ERRORS = 3;
    uint8_t _errors[MAX_ERRORS]{};
};

// -----------------------------------------------------------------------------
// Alte freie Fehler-API
// -----------------------------------------------------------------------------
//
// Vorläufig auskommentiert. Die zentrale Fehlerverwaltung ist jetzt Bestandteil
// der Error-Klasse. Die alte API bleibt sichtbar, bis alle Aufrufer umgestellt sind.
//
// void setError(uint8_t errorCode);
// uint8_t getError();
// bool hasError();
// void clearError();

// -----------------------------------------------------------------------------
// Fehler 0x03 – Referenzierung (Modul: Referenzierungslogik)
// -----------------------------------------------------------------------------
//
// Fehlerart 0x01 .. 0x06
//
constexpr uint8_t FY_ERROR_REF_START_FAILED      = 0x31;
constexpr uint8_t FY_ERROR_REF_SENSOR_NOT_FOUND  = 0x32;
constexpr uint8_t FY_ERROR_REF_POSITION_INVALID  = 0x33;
constexpr uint8_t FY_ERROR_REF_TIMEOUT           = 0x34;
constexpr uint8_t FY_ERROR_REF_ABORTED           = 0x35;
constexpr uint8_t FY_ERROR_REF_REQUIRED          = 0x36;

// -----------------------------------------------------------------------------
// Fehler 0x04 – Endschalter / Positionssensorik
// -----------------------------------------------------------------------------
//
// Noch offen – wird mit der konkreten Sensorlogik definiert.
//

// -----------------------------------------------------------------------------
// Fehler 0x05 .. 0x10
// -----------------------------------------------------------------------------
//
// Noch offen – werden mit den konkreten Modulen definiert.
//
