# FY – ESP32-WROOM / Huginn und Muninn

## Hier beginnen

**[Gemeinsamer Konzept-Einstieg und Lesepfad](docs/README.md)**

Dort gehen wir von den Bedienfällen über das System zum Motorverhalten und erst danach zu Befehlen und Implementierungsresten. Für die aktuelle Konzeptprüfung ist das die erste Seite.

**Aktiver Branch:** `main` · **Projekt:** `FY_ESP32/`.

## Zum Nachschlagen

- [UART-Spickzettel](src/UART_Command_Spickzettel.md): aktuelle Telegramme, Umsetzungsgrad und Prüfnachweise.
- [Kurze Roadmap](../Documents/FY_Productivsoftware_Roadmap.md): verbleibende kleine Arbeiten nach Klärung des Konzepts.
- [System](docs/System/README.md): Huginn/Core 0 für Kommunikation/UI, Muninn/Core 1 als alleiniger Besitzer von Motor und Sensorik.
- [Motor](docs/Motor/README.md): Fahrt, Kalibrierung/Referenz und STOP/GO.
- [Zentrales Pinout](docs/pinouts/README.md): aktuelle Hardwarezuordnung.
- [Head #150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150): Koordination, keine zweite Fachspezifikation.

## Einordnung der vorhandenen Arbeit

UART/BabelFish, Queues, STEP-Profile und Positions-/Gleisfahrten sind im benannten Tisch-Funktionsumfang geprüft. Der Spickzettel unterscheidet fertige Funktionen, offene Fälle und spätere Bühnenintegration.

Relative Fahrt erfolgt über **MOVE mit Richtungsangabe**. LEFT/RIGHT entfallen. SET_POSITION und SET_TRACK sind nur kalibriert und mit gültiger REF zulässig. SET_POSITION ohne REF und ein zusätzlicher POR-Mittenanker entfallen; [Befehlsvertrag #154](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/154).

Historische HW2-Build- und Testabläufe stehen in den [Berichten](docs/reports/) und im [Testprotokoll](docs/MOTOR_TIMER_BENCH.md). Die frühere lange Projekt-README bleibt über die Git-Historie zugänglich; ihre Startwerte und damaligen nächsten Schritte sind kein aktueller Arbeitsplan.
