# Motor – Muninn (Core 1)

**Stand: 24.10.2026.** Verbindliche Klarstellung: [#154](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/154), Referenz/Sensorik [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155), STEP/Profil [#153](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/153). Codebasis des Ist-Abgleichs: `342cdd35`.

## Fahrvertrag

**Verbindlicher Beschluss:** MOVE fährt relativ begrenzt auch ohne REF. SET_POSITION fährt absolut nur im kalibrierten Bewegungsraum mit gültiger REF; SET_TRACK benötigt ebenfalls beides. SET_POSITION ohne REF/POR-Mitte entfällt. [Hier beginnen](../README.md), Detailvertrag #154. MOVE mit Richtung ersetzt LEFT/RIGHT.

Muninn besitzt Motorzustand und Peripherie. Ein Fahrbefehl wird nach erfolgreicher Prepare-Prüfung ausgeführt. GO ist die Antriebsfreigabe, kein zusätzlicher Startbefehl je Job.

| Voraussetzung | SET_POSITION | SET_TRACK |
| --- | --- | --- |
| Keine gültige REF | Nicht zulässig; begrenzte relative Inbetriebnahmefahrt erfolgt über MOVE. | Nicht zulässig. |
| Kalibriert und mindestens kurz gültig referenziert | Ziel innerhalb des kalibrierten Bewegungsraums. | Gleisnummer 1..5 → gespeicherte Position → gleicher Prepare-/Profilpfad. |

Kurze REF kann erst mit gültiger Kalibrierung erfolgreich sein. **Kalibrierdaten, aktuelle Referenzgültigkeit und Positionszähler sind getrennte Informationen.** Ein virtueller Zählerstart ist keine Referenz.

## Auftrag und STOP

- STOP stoppt die Ausführung, verwirft den Auftrag samt vorgemerkter Fahrjobs und deaktiviert den Antrieb.
- Weil reale Bühne und Positionszähler danach voneinander abweichen können, wird die aktuelle REF ungültig. Kalibrierdaten bleiben erhalten.
- `STOP AA 55` entriegelt nur; `GO AA 55` aktiviert nur den Antrieb. Kein alter Auftrag, keine automatische Referenz und kein Wiederaufnehmen.
- Vor normaler kalibrierter Positions-/Gleisfahrt danach neu referenzieren, mindestens kurz. Für Inbetriebnahme ohne REF ist ausschließlich die begrenzte relative MOVE-Fahrt unter ihren Betriebsbedingungen vorgesehen.

## Zustandsübersicht – vereinbartes Verhalten

```mermaid
stateDiagram-v2
  [*] --> STOPPED : POR
  STOPPED --> STOPPED : STOP entriegeln / Antrieb bleibt aus
  STOPPED --> IDLE : GO EIN / Stop entriegelt
  IDLE --> MOVING : Fahrbefehl / Prepare OK
  MOVING --> IDLE : Auftrag fertig
  IDLE --> REFRUN : REFERENCE / Voraussetzungen OK
  REFRUN --> IDLE : Referenzfahrt erfolgreich
  IDLE --> CONF : Parameter / Stillstand
  CONF --> IDLE : Bearbeitung beendet
  IDLE --> STOPPED : STOP oder GO AUS
  MOVING --> STOPPED : STOP oder GO AUS
  REFRUN --> STOPPED : STOP oder GO AUS
  CONF --> STOPPED : STOP
  MOVING --> ERROR : Fehler
  REFRUN --> ERROR : Referenzfehler
  ERROR --> STOPPED : STOP
```

Die frühere abstrakte Darstellung ließ `STOPPED` aus und zeichnete POR/STOP direkt nach IDLE. Hier ist der tatsächlich vorhandene POR-/STOP-Zustand sichtbar. **STOPPED bedeutet Antrieb aus; die STOP-Verriegelung ist ein zusätzliches Flag.** Entriegeln bleibt deshalb in STOPPED; erst GO erreicht IDLE. Referenz-/Kalibrierflags sind keine zusätzlichen Motorzustände.

Dies ist das **Soll**, keine Behauptung, dass CONF/REFRUN/alle Fehlerpfade bereits vollständig implementiert seien. Im heutigen Code sind insbesondere STOPPED/IDLE/MOVING aktiv. Der volle Fehlerrückweg bleibt [#152](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/152)/[#158](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/158). „Auftrag fertig“ ist ein internes Ereignis; UART-Abschluss wird abgefragt, kein spontanes DONE-Telegramm.

## Iststand und verbleibendes Delta

- **Vorhanden:** [Motor.h/.cpp](../../lib/Motor/Motor.h), [STEP-Timer](../../lib/Motor/StepTimer.h), sechsphasige Profilberechnung/-ausführung, MOVE, SET_POSITION und SET_TRACK im gekennzeichneten Tischpfad; atomare Status-/Positionsabfragen.
- **Geprüft:** SET_POSITION-Fahrten/Positionsantworten 20.10.; HW3-MOVE, FAKE-REF und SET_TRACK/GET_TRACK 23.10. Diese Softwarefunktion bleibt im genannten Umfang fertig.
- **Aktuelle Testanbindung:** der Positionspfad verwendet explizite FAKE-`REFERENCE 00` mit Start 1000. SET_POSITION ohne diesen Anker wird abgewiesen. Der beschlossene Produktvertrag benötigt reale Kalibrierung und gültige REF; kein zusätzlicher unreferenzierter Absolutpfad wird ergänzt.
- **Offen:** echter Kalibriert-/Kurz-REF-Vertrag, reale Referenz-/Endlageneingänge und Bewegungsraum, SET_PARAM/CONF; STOP-Robustheit bei voller Pending-/Event-Verwaltung. Kein namensgleicher Nachbau von Nano-setJob/Update nötig.

Pinwerte ausschließlich im [zentralen Pinout](../pinouts/README.md). [Spickzettel](../../src/UART_Command_Spickzettel.md) nennt aktuelle Telegramme und Ampeln; [Testprotokoll](../MOTOR_TIMER_BENCH.md) die Beobachtungen. Alte Nano-Diagramme unter [archive/](archive/) sind Historie und keine zweite aktive Spezifikation.

## Daten- und Funktionsdiagramme (Release 2.0.0)

## Aktueller Steuer- und Datenfluss

```mermaid
flowchart TD
  Q["Normale / STOP-Prioritätsqueue"] --> M["Muninn Motor-State-Machine"]
  M --> P["Prepare und Profilplan"]
  P --> T["StepTimer: STEP/DIR"]
  T --> A["A4988 / Bühne"]
  S["G34/G35/G36/G39 Sensorik"] --> M
  M --> N["NVM-Kalibrierung und Parameter"]
  M --> U["Atomare STATUS-Snapshots"]
  M --> R["ACK / NACK nach Quelle"]
```

```mermaid
stateDiagram-v2
  [*] --> STOPPED
  STOPPED --> IDLE: STOP lösen, GO
  IDLE --> REFRUN: REFERENCE
  REFRUN --> MOVING: gültige REF, BG3 anfahren
  MOVING --> IDLE: Ziel erreicht
  IDLE --> MOVING: SET_TRACK / SET_POSITION / MOVE
  MOVING --> STOPPED: STOP
  REFRUN --> STOPPED: STOP
  IDLE --> STOPPED: STOP
  REFRUN --> ERROR: Sensor-/Referenzfehler
  MOVING --> ERROR: Bewegungsfehler
  ERROR --> STOPPED: STOP
```

**Zugehörige Issues:** [#150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150), [#171](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/171).
