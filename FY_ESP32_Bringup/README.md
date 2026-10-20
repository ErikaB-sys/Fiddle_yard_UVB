# FY – ESP32-WROOM / Huginn und Muninn

**Branch:** `feature/FY_ESP32_Port` · **Stand:** 20.10.2026  
**Status:** Funktionsfaehiger Dual-Core- und UART-Port, begrenzter
Schreibtisch-Motortest in Vorbereitung; **keine produktiv sichere FY-Bewegung**.

## Wo arbeitet was?

| Bereich | Core | Code | Aufgabe |
|---|---|---|---|
| Huginn | 0 | `lib/UART/UART.cpp` | BabelFish, Nano-Protokoll, ACK/NACK, Status |
| Transport | FreeRTOS | `include/FY_Bringup.h` | Kommandos, STOP-Prioritaet, Antworten mit Sequenznummern |
| Muninn | 1 | `lib/Motor/Motor.cpp` | GO/STOP, ENABLE, Mikroschritt-Aufloesung, begrenzte Bench-Auftraege |
| Motor-Timer | ISR | `lib/Motor/StepTimer.cpp` | Hardwaretimer -> STEP HIGH/LOW, Zaehlen auf fallender Flanke |
| Hauptinitialisierung | Arduino | `src/main.cpp` | GPIO-Defaults, Boot-GPIO-Test, Queues, Tasks |
| Hardware | HW2 | `include/HW_Config.h` | Konkrete ESP32-Pinbelegung |

Huginn und Muninn kommunizieren ueber FreeRTOS-Queues. Muninn ist
alleiniger Besitzer von Motorfreigabe und STEP-Ausgabe. Die Timer-ISR
veraendert ausschliesslich private Motor-HAL-Werte und den STEP-Pin.

## Bisher geprueft

- PlatformIO auf echter ESP32-WROOM-HW2 gebaut und geflasht (UART/GO/STOP).
- Nano-`Protokoll.h`, `BabelFish` und `Error` inhaltlich unveraendert uebernommen.
- `HELP`, `FIRMWARE`, mehrere `GET`-Responses und NACKs binär getestet.
- STOP-Sperre / GO-Treiberfreigabe und GPIO27-Pegel auf HW2 erfolgreich gemessen.
- GPIO13/14/4 als A4988-MS1/MS2/MS3 im bisherigen 1/16-Stand auf HIGH
  gemessen; mit der aktuellen **neuen Bench-Firmware** gehen MS2/MS3 auf LOW.
- Core-Dump-Fehlermeldung nach Flash-Erase aus dem Bootprotokoll verschwunden.

**Noch nicht auf HW2 gebaut/geprueft:** der neue `StepTimer.cpp` und
der voruebergehende Halb-Schritt-Bench-Modus. STEP-Pulse und Motorbewegung
sind daher noch kein bestaetigter Funktionsnachweis.

## Motor-Bench-Test (ausdruecklich aktiviert)

`platformio.ini` enthaelt `-D FY_MOTOR_BENCH_TEST=1`.

- Nur fuer **losen, mechanisch nicht an FY angeschlossenen NEMA11**.
- Mikroschritt-Leitungen: HIGH/LOW/LOW = 1/2 Schritt.
- `STOP AA 55` hebt die STOP-Sperre auf.
- `GO AA 55` schaltet den A4988 ein, ohne Bewegung.
- `LEFT` / `RIGHT`: jeweils **64 Halbschritte bei 120 Hz** (ca. 0,53 s).
- `GO 55 AA` bzw. `STOP 55 AA`: Treiber AUS, laufende STEP-Pulse abbrechen.
- Andere Fahrbefehle, Referenzierung und Gleispositionierung bleiben abgewiesen.
- Keine frei erfundene Referenzposition; `GET_POSITION` meldet UNKNOWN.

**Vor dem Einsatz an der FY-Buehne Bench-Flag deaktivieren und Motor-,
Sensor-, Grenz- sowie Referenzlogik vollstaendig portieren.**

Wichtig: Motorversorgung beim Upload/Reset/Umstecken AUS,
Treiber-Hardware-ENABLE-Failsafe noch pruefen, vor Strom einschalten
A4988-VREF, Versorgungspegel, VMOT-Kondensator und Motorverkabelung pruefen.
Ein STOP ueber FreeRTOS ist hier noch **kein nachgewiesener Hardware-Not-Halt**.

## Dateien zur Orientierung

- [Erster Motor-Timer, ISR und Schritt-fuer-Schritt-Test](docs/MOTOR_TIMER_BENCH.md)
- [UART-Protokoll und Hex-Testfolgen](docs/UART_PORT_STEP2.md)
- [WROOM-Pinout / HW2](docs/PINOUT_ESP32.md)
- [Systemarchitektur und Task-Modell](docs/SYSTEM_ARCHITECTURE.md)
- [Coding-Regeln](docs/CODING_RULES.md)
- [UART-Spickzettel](src/UART_Command_Spickzettel.md)

## Naechste Schritte

1. Build und GPIO-/Schritt-Timing pruefen, zunaechst mit VMOT AUS.
2. Kurze, strombegrenzte Drehtests am losen NEMA11.
3. Nano-`Motor::Update()`, `setJob()`, `prepare...()`, Rampenprofile
   sowie asynchrone Fahrabschluss-Ereignisse in Muninn uebernehmen.
4. Endschalter/Lichtschranken und Referenzlauf samt unmittelbarer
   Motor-Abschaltung integrieren.
5. Keyboard/OLED, Parameter, MISRA/Cppcheck und Gesamt-HW-Abnahme.

Verbindliche Koordinierung: [ESP32 Roadmap #150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150),
[Motor #153](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/153),
[Motorlogik #154](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/154).
