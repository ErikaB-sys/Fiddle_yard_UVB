#pragma once

#include <Arduino.h>

/**
 * @file Error.h
 * @brief Fehlercodes und kleiner, fluechtiger Fehlerspeicher des FY-Controllers.
 *
 * Fehlercode (ein Byte): oberes Halbbyte = Fehlerort, unteres Halbbyte =
 * Fehlerart. 0x00 bedeutet "kein Fehler". Die derzeit definierten Fehlerorte
 * sind UART (0x10), Motor (0x20), Referenzierung (0x30), Sensorik (0x40)
 * und Keyboard (0x50).
 *
 * @note Ein NACK ist eine Befehlsantwort und wird nicht automatisch als
 *       Systemfehler gespeichert. Ein gemeinsamer Fehlerspeicher fuer alle
 *       Module ist ein gesonderter Change (#181).
 */

/** Kein gespeicherter Fehler; zugleich Kennung eines freien Speicherplatzes. */
constexpr uint8_t FY_ERROR_NONE = 0x00;

/** Bitmaske fuer den Fehlerort (oberes Halbbyte). */
constexpr uint8_t FY_ERROR_LOCATION_MASK = 0xF0;
/** Bitmaske fuer die Fehlerart (unteres Halbbyte). */
constexpr uint8_t FY_ERROR_TYPE_MASK     = 0x0F;

/** Kennungen der derzeit vorgesehenen Fehlerorte. */
constexpr uint8_t FY_ERROR_LOCATION_UART      = 0x10;
constexpr uint8_t FY_ERROR_LOCATION_MOTOR     = 0x20;
constexpr uint8_t FY_ERROR_LOCATION_REFERENCE = 0x30;
constexpr uint8_t FY_ERROR_LOCATION_SWITCHES  = 0x40;
constexpr uint8_t FY_ERROR_LOCATION_KEYBOARD  = 0x50;

/**
 * @brief Verwaltet drei gespeicherte Fehler, hoechstens einen je Fehlerort.
 *
 * Ein neuer Fehler ersetzt einen vorhandenen Eintrag desselben Ortes oder
 * belegt den ersten freien Platz. Sind alle Plaetze durch andere Orte
 * belegt, bleibt der neue Fehler ungespeichert. Beim Loeschen entstehen
 * freie Plaetze; die restlichen Eintraege werden nicht verschoben.
 *
 * @note Kein automatisches Loeschen und keine Synchronisation zwischen Tasks.
 *       Im aktuellen ESP32-Stand gehoert die einzige Instanz der UART auf
 *       Huginn/Core 0. Core-uebergreifende Meldungen folgen separat (#181).
 */
class Error
{
public:
    /** @brief Leert alle drei Fehlerplaetze, z. B. beim Systemstart. */
    void begin();

    /**
     * @brief Speichert oder ersetzt einen Fehler anhand seines Fehlerortes.
     * @param errorCode Fehlerort im oberen, Fehlerart im unteren Halbbyte.
     * @note Codes ohne Fehlerort und FY_ERROR_NONE werden ignoriert.
     *       Bei drei belegten Plaetzen unterschiedlicher Orte bleibt
     *       der neue Fehler ungespeichert.
     */
    void setError(uint8_t errorCode);

    /**
     * @brief Loescht den gespeicherten Fehler des angegebenen Ortes.
     * @param errorLocation Fehlerort im oberen Halbbyte; untere Bits
     *                      werden maskiert. Ort 0 bewirkt nichts.
     * @note Andere Fehler bleiben an ihren bisherigen Speicherplaetzen.
     */
    void clearError(uint8_t errorLocation);

    /**
     * @brief Liest einen Speicherplatz ohne Umordnung der Fehler.
     * @param index Index des Speicherplatzes (0 bis 2).
     * @return Fehlercode oder FY_ERROR_NONE fuer leere/ungueltige Plaetze.
     */
    uint8_t getError(uint8_t index) const;

    /**
     * @brief Ermittelt die Belegung der drei Fehlerplaetze.
     * @return Bit 0 bis 2: 1 fuer belegten Speicherplatz, sonst 0.
     * @note Die Bits bezeichnen Speicherplaetze, nicht Fehlerorte.
     */
    uint8_t getSystemByte() const;

private:
    static constexpr uint8_t MAX_ERRORS = 3;
    uint8_t _errors[MAX_ERRORS]{};
};

// Referenzierung: Fehlerort 0x30, Fehlerarten 0x01 bis 0x06.
constexpr uint8_t FY_ERROR_REF_START_FAILED      = 0x31;
constexpr uint8_t FY_ERROR_REF_SENSOR_NOT_FOUND  = 0x32;
constexpr uint8_t FY_ERROR_REF_POSITION_INVALID  = 0x33;
constexpr uint8_t FY_ERROR_REF_TIMEOUT           = 0x34;
constexpr uint8_t FY_ERROR_REF_ABORTED           = 0x35;
constexpr uint8_t FY_ERROR_REF_REQUIRED          = 0x36;
