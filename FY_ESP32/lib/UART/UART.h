#pragma once
#include "FY_Bringup.h"
#include "BabelFish.h"
#include "Error.h"

namespace fy {

/**
 * @brief UART-Protokollverarbeitung auf Huginn (Core 0).
 * @details Nur dieses Modul liest und schreibt Serial. Das Nano-Protokoll
 *          dient als Grundlage; ESP32-Erweiterungen sind separat definiert.
 */
class UART {
public:
    /** @brief Initialisiert Protokollparser und UART-Ausgabe.
     * @param context Gemeinsamer Kontext mit den FreeRTOS-Queues.
     */
    void begin(BringupContext* context);
    /** @brief Verarbeitet Sensor-Snapshots, Motorantworten und UART-Eingaben. */
    void update();

private:
    struct ResponseBuffer_t {
        uint8_t id = 0;
        uint8_t data[MAX_RESPONSE_LENGTH]{};
        const char* string = nullptr; // Flash ist auf dem ESP32 direkt adressierbar.
        uint8_t length = 0;
        ResponseContainer container = ResponseContainer::Data;
        bool responsePending = false;
    };

    BringupContext* _context = nullptr;
    BabelFish _babelFish;
    BabelFishCommand_t _command{};
    ResponseBuffer_t _response{};
    Error _errors;
    uint32_t _nextSequence = 0;
    uint8_t _sensorSnapshot = 0U;
    bool _sensorSnapshotValid = false;
    // Interne Zuordnung von Motorantworten; keine Sequenznummer im Drahtprotokoll.
    struct PendingCommand { uint32_t sequence = 0; uint8_t id = 0; bool active = false; };
    PendingCommand _pending[16]{};

    /** @brief Liest serielle Zeichen und verarbeitet vollstaendige Telegramme. */
    void receive();
    /** @brief Prueft Befehlskennung, Nutzdatenlaenge und CRC.
     * @param definition Erhaelt bei bekannter Kennung die Befehlsdefinition.
     * @return true bei gueltigem Telegramm. */
    bool validateCommand(const CommandDefinition*& definition);
    /** @brief Verteilt einen geprueften Befehl nach Befehlstyp.
     * @param definition Zugehoerige Befehlsdefinition. */
    void decodeCommand(const CommandDefinition& definition);
    /** @brief Bearbeitet Befehle ohne Motor-Queue. */
    void decodeImmediate();
    /** @brief Prueft auszufuehrende Befehle und leitet sie an Muninn weiter. */
    void decodeExecute();
    /** @brief Prueft STOP und leitet ihn an die Prioritaetsqueue weiter. */
    void decodePriority();
    /** @brief Ordnet Motorereignisse aus der Event-Queue ausstehenden Befehlen zu. */
    void collectMotorResponses();
    /** @brief Uebernimmt den letzten Sensorstatus aus der Snapshot-Queue. */
    void collectSensorSnapshot();
    /** @brief Uebergibt einen Befehl an die passende Motor-Queue.
     * @param priority true fuer die STOP-Prioritaetsqueue.
     * @return true bei erfolgreicher Uebergabe. */
    bool queueMotorCommand(bool priority);

    /** @brief Erzeugt die Antwort auf GET_STATUS. */
    void handleGetStatus();
    /** @brief Erzeugt die Antwort auf GET_PARAM. */
    void handleGetParam();
    /** @brief Erzeugt die Antwort auf GET_ERROR. */
    void handleGetError();
    /** @brief Erzeugt die Antwort auf GET_POSITION. */
    void handleGetPosition();
    /** @brief Erzeugt die Antwort auf GET_TRACK. */
    void handleGetTrack();
    /** @brief Erzeugt die Antwort auf HELP. */
    void handleHelp();
    /** @brief Erzeugt die Antwort auf GET_FIRMWARE. */
    void handleGetFirmware();
    /** @brief Prueft SET_MODE; Betriebsartenwechsel derzeit nicht implementiert. */
    void handleSetMode();

    /** @brief Sendet die vorgemerkte Antwort einschliesslich CRC. */
    void sendResponse();
    /** @brief Merkt eine Datenantwort zur spaeteren Ausgabe vor.
     * @param id Antwortkennung.
     * @param data Nutzdatenpuffer.
     * @param length Nutzdatenlaenge. */
    void setResponse(uint8_t id, const uint8_t* data, uint8_t length);
    /** @brief Merkt eine Textantwort zur spaeteren Ausgabe vor.
     * @param id Antwortkennung.
     * @param text Nullterminierter Antworttext.
     */
    void setResponse(uint8_t id, const char* text);
    /** @brief Merkt eine positive Bestaetigung vor.
     * @param cmd Bestaetigte Befehlskennung. */
    void ack(uint8_t cmd);
    /** @brief Merkt eine negative Bestaetigung vor.
     * @param location Fehlerort.
     * @param reason Fehlergrund. */
    void nack(uint8_t location, NackReason reason);
};

/** @brief Startet die UART-Task auf Huginn.
 * @param context Gemeinsamer Kontext mit den FreeRTOS-Queues.
 * @return Ergebnis von xTaskCreatePinnedToCore().
 */
BaseType_t Huginn_startUART(BringupContext* context);
}
