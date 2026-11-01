# FY – ESP32-WROOM / Huginn und Muninn

## Hier beginnen

**[Gemeinsamer Konzept-Einstieg und Lesepfad](docs/README.md)**

Dort stehen der aktuelle Iststand, der verbindliche Schnittstellenvertrag und die Abgrenzung zu historischen Entwicklungsunterlagen.

**Aktueller Stand:** `main`, Firmware **2.1.0** (01.11.2026) · **Projekt:** `FY_ESP32/`. Grundlage: risikobasierte HW3-Regression vom 01.11.2026 und abgeschlossenes Modul-Cleanup.

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

## Release 2.1.0

Gegenueber 2.0.0: bereinigte und dokumentierte ESP32-Module, aktualisierte UART- und NVM-Pfade, verbesserte Keyboard-/OLED-Bedienung und verfuegbarkeitsbasierte STATUS 10/50-Abfragen. Die lokale Motor-, Referenz- und STOP-Grundfunktion bleibt erhalten. `GET_FIRMWARE` meldet `VER: 2.1.0`; der UART-Startbanner zeigt die Version ebenfalls.

**Abnahme:** [HW3-Regression vom 01.11.2026](test/REGRESSION_HW3_2026-11-01.md) mit PASS; anschliessender Build nach den letzten Codekorrekturen erfolgreich. Fuer die neue Versionskennzeichnung selbst ist vor dem Tag ein erneuter PlatformIO-Build durchzufuehren. Das ESP32-Buildartefakt ist `firmware.bin`, nicht `.hex`.

**Geplantes Release-Tag:** `FY_Release_2.1.0` (ohne Leerzeichen). Alte Tags bleiben unveraendert; Release-Tag erst nach erfolgreichem Build auf den finalen Main-Commit setzen.
