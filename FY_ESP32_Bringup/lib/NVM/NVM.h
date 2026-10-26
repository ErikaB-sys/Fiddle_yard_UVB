#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @file NVM.h
 * @brief Dauerhafte Speicherung kleiner, versionierter Datensaetze im ESP32-NVS.
 *
 * Modulvertrag:
 * - Einziger Zweck: Lesen, Vergleichen, Speichern und Invalidieren von Binaerdaten.
 * - Unabhaengig von Motor, UART, Huginn und Muninn; keine eigenen Tasks/Queues.
 * - Jeder Datensatz besitzt Kennung, Version, Nutzdatenlaenge und CRC32.
 * - Schreiben nur bei tatsaechlich veraendertem Inhalt; keine EEPROM-Emulation.
 * - Aufrufer serialisieren schreibende FY-Aktionen und lassen sie nur im
 *   sicheren Stillstand zu. NVM uebernimmt keine Betriebszustandspruefung.
 * - Nutzdaten sind kanonisch kodierte Bytes: keine ungeprueften C++-Strukturen
 *   mit Padding/ABI-abhaengigem Layout direkt persistent ablegen.
 * - CRC32 erkennt zufaellige Defekte, bietet aber keine Verschluesselung.
 *
 * Basis: Arduino-ESP32 Preferences (ESP-IDF NVS). Ein Namespace pro
 * Fachdomaene ist moeglich; einzelne Datensaetze werden ueber Keys adressiert.
 */
namespace fy {

enum class NvmResult : uint8_t {
    OK,             ///< Erfolgreich gelesen oder geschrieben.
    UNCHANGED,      ///< Bereits identisch gespeichert; kein Flash-Schreiben.
    NOT_FOUND,      ///< Noch kein Datensatz fuer diesen Schluessel vorhanden.
    INVALID_DATA,   ///< Datensatzformat, Version, Laenge oder CRC ungueltig.
    INVALID_ARG,    ///< Schluessel, Puffer, Version oder Laenge unzulaessig.
    IO_ERROR        ///< NVS konnte nicht geoeffnet, gelesen oder geschrieben werden.
};

/** @brief Schlanker synchroner NVS-Speicherdienst ohne eigene Task/Queue. */
class NVM final {
public:
    /// Maximaler Nutzdatenumfang eines Datensatzes in Bytes.
    static constexpr size_t MAX_PAYLOAD = 256U;

    /** @brief Namespace festlegen (ESP32-NVS: maximal 15 ASCII-Zeichen). */
    explicit NVM(const char* nameSpace) : _nameSpace(nameSpace) {}

    /** @brief Datensatz laden und Kennung/Version/Laenge/CRC32 pruefen.
     * @param key NVS-Schluessel (maximal 15 Zeichen).
     * @param version Erwartete Formatversion (ungleich 0).
     * @param data Ausgabepuffer; bei Fehler unveraendert.
     * @param length Exakte Nutzdatenlaenge.
     */
    NvmResult load(const char* key, uint16_t version,
                   void* data, size_t length) const;

    /** @brief Datensatz nur speichern, falls die gespeicherten Bytes abweichen.
     * @return OK bei Schreibvorgang, UNCHANGED bei identischem Inhalt,
     *         sonst Fehlerstatus.
     */
    NvmResult save(const char* key, uint16_t version,
                   const void* data, size_t length) const;

    /** @brief Vorhandene gueltige oder bereits invalidierte CRC erneut aus
     *         den gespeicherten Daten berechnen und invertiert ablegen.
     * @return OK bei Aenderung, UNCHANGED bei bereits invalidiertem Eintrag.
     *         Ein fehlender oder strukturell defekter Eintrag wird nicht erzeugt.
     */
    NvmResult invalidate(const char* key) const;

private:
    const char* _nameSpace;
};

} // namespace fy
