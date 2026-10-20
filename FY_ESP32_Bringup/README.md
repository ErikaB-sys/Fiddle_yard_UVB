# FY – ESP32-WROOM / Huginn und Muninn

**Branch:** `feature/FY_ESP32_Port` · **Stand:** 20.10.2026  
**Status:** Funktionsfaehiger Dual-Core- und UART-Port; STEP-Tischmotor erstmals
auf HW2 gelaufen; MOVE/STOP an losem Motor getestet; profilvorbereitende Motor-State-Machine
integriert, sechsphasiger Profil-Executor in der ISR geschrieben (Build/HW-Test offen); **keine produktiv sichere FY-Bewegung**.

## Speicher-Baseline vor dem neuen SET_POSITION-Test
PlatformIO Release-Build, 20.10.2026 laut HW2-Log:
RAM 21544 / 327680 B (6,6 %), Flash 281981 / 1310720 B (21,5 %),
Build 4,05 s. **Nicht der Speicherverbrauch der jetzt weiterentwickelten
Bench-Positionierungsfirmware; dafuer erneuten Build abwarten.**

## Wo arbeitet was?

| Bereich | Core | Code | Aufgabe |
|---|---|---|---|
| Huginn | 0 | `lib/UART/UART.cpp` | BabelFish, Nano-Protokoll, ACK/NACK, Status |
| Transport | FreeRTOS | `include/FY_Bringup.h` | Kommandos, STOP-Prioritaet, Antworten mit Sequenznummern |
| Muninn | 1 | `lib/Motor/Motor.cpp` | GO/STOP, ENABLE, Mikroschritt-Aufloesung, begrenzte Bench-Auftraege |
| Motor-Timer | ISR | `lib/Motor/StepTimer.cpp` | STEP HIGH/LOW; fallende Flanke zaehlt und aktualisiert sechs Profilphasen, Timergruppe 1/Timer 0 |
| Hauptinitialisierung | Arduino | `src/main.cpp` | GPIO-Defaults, Boot-GPIO-Test, Queues, Tasks |
| Hardware | HW2 | `include/HW_Config.h` | Konkrete ESP32-Pinbelegung |

Huginn und Muninn kommunizieren ueber FreeRTOS-Queues. Muninn ist
alleiniger Besitzer von Motorfreigabe und STEP-Ausgabe. Die Timer-ISR
veraendert ausschliesslich private Motor-HAL-Werte und den STEP-Pin.

## Bisher geprueft

- PlatformIO auf echter ESP32-WROOM-HW2 gebaut und geflasht (UART/GO/STOP).
- Nano-`Protokoll.h` und `Error` weiterhin inhaltlich unveraendert; ESP32-BabelFish jetzt mit separatem MOVE-Kommando (ID 0x0F) erweitert.
- `HELP`, `FIRMWARE`, mehrere `GET`-Responses und NACKs binär getestet.
- STOP-Sperre / GO-Treiberfreigabe und GPIO27-Pegel auf HW2 erfolgreich gemessen.
- GPIO13/14/4 als A4988-MS1/MS2/MS3 zuvor am 1/16-Stand gemessen;
  neuer **Bench-Default 1/4: LOW/HIGH/LOW** muss noch auf HW2 nachgemessen werden.
- Core-Dump-Fehlermeldung nach Flash-Erase aus dem Bootprotokoll verschwunden.

**HW2-Meilenstein:** Der lose NEMA11 dreht ueber den neuen STEP-Timer und
GO/LEFT/RIGHT, Richtungsumkehr bestaetigt. Die Nachfolge-Firmware mit
MOVE/prepareMove sowie die 10000-Schritte-Langfahrt mit STOP wurden
am losen NEMA11 bestaetigt: Motor steht nach STOP sofort, gemessener
Versorgungsstrom faellt auf 0. POR-STOP-Sperre wurde am HW2
anschliessend ebenfalls ueber GO/NACK, STOP-Freigabe und MOVE bestaetigt.

## Motor-Bench-Test (ausdruecklich aktiviert)

`platformio.ini` enthaelt `-D FY_MOTOR_BENCH_TEST=1`.

- Nur fuer **losen, mechanisch nicht an FY angeschlossenen NEMA11**.
- Standard-Mikroschritt-Leitungen: LOW/HIGH/LOW = 1/4 Schritt (A4988 MS1/MS2/MS3).
- **POR/Reset startet mit verriegeltem STOP** und Treiber AUS; keine STEP/DIR-Start-Lichtorgel.
- `GO AA 55` vor `STOP AA 55` liefert Motor/NOT_READY.
- `STOP AA 55` hebt die STOP-Sperre auf, laesst den Treiber aber AUS.
- `GO AA 55` schaltet den A4988 ein, ohne Bewegung.
- `MOVE 21 32 00`: 50 Viertelschritte rechts bei 120 Hz (ca. 0,42 s).
- `MOVE 20 32 00`: 50 Viertelschritte links.
- `MOVE 21 10 27`: 10000 Viertelschritte rechts; neue Nano-Rampe rechnerisch ca. 14,6 s, auf HW2 noch nicht getestet.
- `MOVE`-Format: DATA0 oberes Nibble Modus (0=voll,1=halb,2=viertel,3=achtel,4=sechzehntel), unterstes Bit Richtung 0=links/1=rechts; DATA1/2 Schrittzahl uint16 little endian; Restbits reserviert.
- Die alten Nano-Befehle `LEFT` / `RIGHT` werden von ESP32 nicht mehr ausgefuehrt.
- `GO 55 AA` bzw. `STOP 55 AA`: Treiber AUS, laufende STEP-Pulse abbrechen.
- **Bench-SET_POSITION** erlaubt absolute Testfahrten in virtuellen Viertelschritten; Start = 32767, Gleisabstand 1600, maximal 10600 je SET_POSITION-Auftrag. Physische Referenz bleibt ungueltig.
- `GET_POSITION` liefert im Bench die synthetische Position **mit Marker-Bit 16**, z. B. `0x00017FFF` fuer BG3/32767. Keine echte Positionierung der FY-Buehne; REFERENCE/SET_TRACK bleiben abgewiesen.
- MOVE in anderer Aufloesung als 1/4 invalidiert die synthetische Positionsfuehrung bis zur erneuten Bench-Verankerung (STOP/GO-AUS).

**Vor dem Einsatz an der FY-Buehne Bench-Flag deaktivieren und Motor-,
Sensor-, Grenz- sowie Referenzlogik vollstaendig portieren.**

Wichtig: Motorversorgung beim Upload/Reset/Umstecken AUS,
Treiber-Hardware-ENABLE-Failsafe noch pruefen, vor Strom einschalten
A4988-VREF, Versorgungspegel, VMOT-Kondensator und Motorverkabelung pruefen.
Ein STOP ueber FreeRTOS ist hier noch **kein nachgewiesener Hardware-Not-Halt**.

## Dateien zur Orientierung

- [Portierungsstatus UART/Motor 20.10.2026](docs/MOTOR_PORT_STATUS_2026-10-20.md)
- [Erster Motor-Timer, ISR und Schritt-fuer-Schritt-Test](docs/MOTOR_TIMER_BENCH.md)
- [UART-Protokoll und Hex-Testfolgen](docs/UART_PORT_STEP2.md)
- [WROOM-Pinout / HW2](docs/PINOUT_ESP32.md)
- [Systemarchitektur und Task-Modell](docs/SYSTEM_ARCHITECTURE.md)
- [Coding-Regeln](docs/CODING_RULES.md)
- [Original Nano UART-Spickzettel](../FY_Controller_Motor/src/UART_Command_Spickzettel.md) (MOVE-Erweiterung siehe Motor-Timer-Doku)

## Naechste Schritte

1. Neuen sechsphasigen ISR-Executor mit Viertelschritt/virtuellem SET_POSITION auf HW2 bauen; Speicherverbrauch vergleichen; mit VMOT AUS flashen.
2. Bei losem Motor BG3 -> BG1 (`SET_POSITION 7F 73`) -> BG5 (`SET_POSITION 7F 8C`) fahren, Ramping/STOP/STATUS 20 und GET_POSITION pruefen.
3. Anschliessend Nano-`Motor::Update()` und `setJob()` vervollstaendigen;
   GPIO-Endschalter/Referenzlauf gesondert abnehmen.
   **Keine spontanen DONE-UART-Antworten**; Request/Response bleibt verbindlich.
4. Endschalter/Lichtschranken und Referenzlauf samt unmittelbarer
   Motor-Abschaltung integrieren.
5. Keyboard/OLED, Parameter, MISRA/Cppcheck und Gesamt-HW-Abnahme.

Verbindliche Koordinierung: [ESP32 Roadmap #150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150),
[Motor #153](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/153),
[Motorlogik #154](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/154).
