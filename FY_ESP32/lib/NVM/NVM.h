#pragma once

#include <stddef.h>
#include <stdint.h>
#include "NvmAreas.h"

/**
 * @file NVM.h
 * @brief Synchroner NVS-Speicherdienst mit statischen FY-Segmenten.
 *
 * Modulvertrag:
 * - Verwaltet ausschliesslich Nutzdatenbytes, Formatversion und CRC32.
 * - Statische Segment-IDs; NVS-Schluessel, Versionen und Laengen liegen
 *   zentral im Descriptor in NVM.cpp. Neue Segmente nur hinten anfuegen;
 *   bestehende Segment-IDs/Keys und Datenformate nicht stillschweigend aendern.
 * - Ein gemeinsamer Namespace "fy_uvb"; physische Flash-Adressen verwaltet
 *   Espressif NVS (Preferences), nicht diese Anwendung.
 * - load() prueft Format, Laenge, Version und CRC, veraendert bei Fehler
 *   den Ausgabepuffer nicht. save() schreibt nur bei geaenderten Bytes.
 * - invalidate() hinterlegt das Komplement der frisch berechneten CRC.
 *   Wiederholtes Invalidieren darf keine Gueltigkeit herstellen.
 * - NOT_FOUND erlaubt dem jeweiligen Fachmodul die Erstinitialisierung
 *   seiner Defaults; INVALID_DATA darf nicht automatisch ueberschrieben werden.
 *   CALIBRATION hat niemals synthetische Defaults (nur Messfahrt!).
 * - NVM kennt weder Motor noch Huginn/Muninn, besitzt keine Task, Queue,
 *   eigene Sperre, Betriebszustandspruefung oder Default-/Migrationslogik.
 *   Alle schreibenden Aktionen werden von der FY-Steuerung exklusiv und
 *   ausschliesslich im sicheren Stillstand zugelassen.
 * - Fachmodule kodieren ihre festen Payloads kanonisch als Bytes.
 *   Ungepruefte C++-Structs mit Padding nicht direkt persistent speichern.
 * - CRC32 ist Fehlererkennung, keine Verschluesselung.
 */
namespace fy {

/** @brief Stabile IDs der im NVM-Descriptor beschriebenen Segmente. */
/** @brief Gleiche Bereichs-IDs wie im UART-Protokoll. */
using NvmRecord = NvmArea;
// Der NVM-Descriptor bestimmt, welche Bereiche bereits implementiert sind.

enum class NvmResult : uint8_t {
    OK,            ///< Erfolgreich gelesen oder geschrieben.
    UNCHANGED,     ///< Bereits identisch gespeichert / schon invalidiert.
    NOT_FOUND,     ///< Segment wurde noch nicht gespeichert.
    INVALID_DATA,  ///< Format, Version, Laenge oder CRC ungueltig.
    INVALID_ARG,   ///< Unbekannte Segment-ID oder ungueltiger Datenpuffer.
    IO_ERROR       ///< NVS konnte nicht geoeffnet/gelesen/geschrieben werden.
};

/** @brief Minimaler synchroner NVS-Dienst, der nur feste IDs akzeptiert. */
class NVM final {
public:
    /** @brief Datensatz in den vom Descriptor festgelegten Puffer laden.
     * @param record Statische Segment-ID.
     * @param data Ausgabepuffer mit exakt der im Descriptor angegebenen Groesse.
     * @return OK oder Fehlerstatus; bei Fehler bleibt data unveraendert.
     */
    NvmResult load(NvmRecord record, void* data) const;

    /** @brief Datensatz mit gueltiger CRC nur bei geaenderten Bytes speichern.
     * @param record Statische Segment-ID.
     * @param data Eingabepuffer in festgelegter Segmentgroesse.
     * @return OK bei Schreiben, UNCHANGED ohne Flash-Schreibvorgang.
     */
    NvmResult save(NvmRecord record, const void* data) const;

    /** @brief Gespeicherten Datensatz durch invertierte CRC invalidieren.
     * @return OK beim Schreiben, UNCHANGED bei erneuter Invalidierung.
     */
    NvmResult invalidate(NvmRecord record) const;
};

} // namespace fy
