# FY ESP32 – offene Soll-Ist-Abweichungen / Review-Checkliste

**Erfasst:** 21.10.2026 · Branch `feature/FY_ESP32_Port`. **Morgen gemeinsam durchgehen.** Fachlicher Sollstand: [#150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150) mit Teil-Issues; Code-Snapshot: Commit `df69ef79c3256d68237c60ac0fb7021daafa0895` (vor dieser Dokumentationsumstellung). **[ ] bedeutet offen / ungeklärt, NICHT „Code ist fehlerhaft“.**

## Kritisch vor Betrieb an der FY-Mechanik

- [ ] **D01 – Boot/Flash-Failsafe:** #153/#155/#161 verlangen Schutz gegen ungewollte A4988-ENABLE-/STEP-Pegel **vor** `setup()`. `src/main.cpp` schaltet GPIO erst nach Firmwarestart auf sicheren Pegel; hardwareseitiger VMOT-Interlock/Pull-up nicht nachgewiesen. **Entscheidung/Nachweis:** Schaltung, Messung beim Reset/Flash, sichere Freigabe; bis dahin VMOT AUS beim Flashen.
- [ ] **D02 – STOP-Priorität ≠ Hardware-Not-Halt:** #152/#155 fordern Stopreaktion unabhängig von überlasteten Queues/Tasks; aktuell `BringupContext.priorityCommands` + Muninn-Timerpfad, aber harte STOP-Latenz/ISR-Schutz unter Last nicht nachgewiesen. **Entscheidung/Nachweis:** definierter Stoppweg + Messung.
- [ ] **D03 – Referenzierung und Grenzschutz:** #155 verlangt echte Lichtschranken-/Endlagenbehandlung und Referenzverlust; `lib/Motor/Motor.h` setzt `_ReferenceValid=false`; `lib/Switches/` enthält keine aktive ESP32-Implementierung. **Entscheidung/Nachweis:** Sensorik, Referenzfahrt, Timeout/Fehlerfall und FY-HW-Test.
- [ ] **D04 – Prüfungsumfang bis main-Merge:** #159/#150 definieren HW-Abnahme einschließlich Bewegung, STOP, Reset, Fehlerszenarien. Bisheriges Bench-Testprotokoll belegt nur losen Motor/virtuelle Position. **Entscheidung:** pragmatische konkrete HW-Nachweise ohne Pflicht zu 100 % Coverage; dokumentierter Release-Gate.

## Fachliche / Software-Verträge

- [ ] **D05 – Produktive Kommandoparität:** #156 verlangt Nano-Kompatibilität, #154 dokumentiert bewusstes ESP32-Bench-`MOVE 0x0F` mit neuer DATA-Belegung; Nano `LEFT/RIGHT` werden im ESP-Bench-Pfad nicht ausgeführt. **Entscheidung:** endgültige Kommandoliste, Übergangs-/Kompatibilitätsregeln, ggf. Issue-Anpassung; nicht stillschweigend beides als identisch beschreiben.
- [ ] **D06 – Motorjob-/State-Machine-Vollständigkeit:** #154 fordert Prepare-/Job-Lifecycle und reale Gleispositionierung. `Motor.h/.cpp` hat Zustände und Profil/BENCH, aber Nano-`setJob()/Update()` und realer FY-Track-Ablauf nicht vollständig abgenommen. **Entscheidung/Nachweis:** Sollpfade mit Code und später Mechanik vergleichen.
- [ ] **D07 – Systemzustände vs. Übersicht:** Überblick verzichtet auf `STOPPED`; echter `MotorState_t` enthält `STOPPED` und startet dort. **Dokumentiert als absichtliche Abstraktion**, aber #154 verlangt klare POR/GO/STOP-Transitionen. **Review:** technische Zustandstabelle und abstraktes Diagramm gemeinsam gegen Code prüfen.
- [ ] **D08 – UART-Abschlusssemantik:** In #154 verbindlich **keine** spontane DONE-Antwort, sondern Status-Polling. Älterer Architekturentwurf hatte Push-DONE-Sequence; in der aktiven Dokumentation korrigiert. **Review:** Handler/Event-Queue darauf prüfen; #156 erwähnt DONE im allgemeinen Zieltext.
- [ ] **D09 – Queue-/Fehlertransport:** #152/#162 planen zentralen Router, Request-ID, Fehler-/Status-Snapshots, definierte Overflow-Fälle. `include/FY_Bringup.h` hat drei Queues und Sequenz, `Error` liegt derzeit im UART-Objekt, `SwitchesSnapshot` noch nicht angeschlossen. **Entscheidung:** Verantwortlichkeit/Fehlertransport und Queue-Full-Verhalten.
- [ ] **D10 – Parameter und echtes Teach-in:** #158 und #167 verlangen verlässliche Track-Daten/Teach-in, Änderungen nur im Stillstand, Diagnose/Firmware-ID. Bench-`SET_POSITION` verwendet synthetische Position. **Entscheidung/Nachweis:** operative Parameter-/Kalibrierungs-API, Speicherkonzept nur nach Beschluss.
- [ ] **D11 – Keyboard / Display:** #157 will PCF8574, vier Tasten, STOP, Local/Remote, 4-Zeilen-OLED; `lib/Keyboard/` hat noch keinen ESP-C++-Code. **Entscheidung/Nachweis:** I²C-Pinout und tatsächliche Platine, OLED-Init aus Fahrpult, Polling ohne INT, Bedien-/Sperrtests.
- [ ] **D12 – Fehler-API und Cores:** `lib/Error/Error.h/.cpp` sind vorhanden, `UART` hat `_errors`. Die geplante aggregierte Huginn-Fehlerverwaltung aus #162/#158 mit Muninn-Meldungen fehlt als nachgewiesener End-to-End-Pfad. **Entscheidung/Nachweis:** Ownership, Reset/Quittierung und Test.

## Qualität und Dokumentation

- [ ] **D13 – MISRA-orientierte ESP-Prüfung:** [#163](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/163) verlangt Nano-Ansatz portieren. Ein separates Script/PlatformIO-Ziel ist als Teil dieser Dokumentationsarbeit angelegt; **noch kein lokaler Prüflauf oder Baseline-Befundbericht**. **Morgen:** mit installiertem Cppcheck auf Entwicklerrechner ausführen, Ergebnis/Ausnahmen bewerten.
- [ ] **D14 – HW-Pinout vs. aktueller Bench-Code:** `docs/pinouts/01_PINOUT_ESP32.md` hatte noch „keine STEP-ISR“ und alten Mikroschritt-Bench-Default; `StepTimer.cpp` und `README.md` belegen neueren Stand. Text hier aktualisiert, **physische Pegelmessung mit aktuellem Firmwarestand weiter offen**. Quelle: `include/HW_Config.h`.
- [ ] **D15 – Referenzierung alter Nano-Diagramme:** Arbeitsdiagramme aus `lib/Motor/` und `lib/UART/` nach `docs/<Modul>/archive/` verschoben. **Review:** welche fachlich übernommen/angepasst werden; keine verbatim Nano-ISR als ESP-Soll ausgeben.
- [ ] **D16 – Dokumentationsfreigabe:** neue Modulübersichten sind aus Issues und dem sichtbaren Quellstand abgeleitet, noch **nicht gemeinsam fachlich bestätigt**. Erst nach Review Checklisten in Modul-READMEs final abhaken.

## Später, nicht mit der Portierung verwechseln

- [ ] **D17 – WLAN/JMRI/Betriebsarten:** #160/#166 sind überwiegend Enhancement/Konzept. Umschaltung nur nach STOP/Stillstand als Ziel festhalten; nicht als implementiert behandeln.
- [ ] **D18 – Optionales SPI-Schaufensterdisplay/Licht:** #165 ist Backlog, ausdrücklich **nicht** Keyboard-Funktionsumfang #157. Keine Vermischung.

## Bei unserem Review

Zu jedem Punkt festhalten: **Entscheidung (Soll), konkrete Code-/Dokuänderung, Nachweisart, Datum/Commit oder bewusst „nicht geprüft“**. Die Checkliste dient der Abweichungsbearbeitung, nicht als zweite Issue-Verwaltung. Beim Schließen auf betroffene Fach-Issues verweisen und einmalig aktualisieren.

## HW3-Pinplanung – Ergänzung 22.10.2026

- [ ] **D19 – HW3-Zielbelegung in Firmware überführen:** Der [zentrale Pinplan](pinouts/01_PINOUT_ESP32.md) enthält jetzt die vollständige HW3-Zielbelegung mit separatem A4988-SLEEP und den SPI-Steuersignalen aus #165. `HW_Config.h` ist am 22.10.2026 gezielt auf HW3 umgestellt: Sensor-Konstanten aktualisiert, SLEEP/SPI/CS/DC definiert und nach Muninn/Kern 1 sowie Huginn/Kern 0 gegliedert. Offen bleiben SLEEP-Ansteuerung und Aufwachzeit sowie unmittelbare Endlagen-Pegelprüfung auf Muninn und SPI-Geräteansteuerung. Mit SLEEP-Pulldown bleibt der A4988 ohne diese Laufzeitergänzung im Schlafzustand. G36/G39 werden ohne GPIO-Flankeninterrupts geplant. Display-Reset lokal, keine ungeprüfte Brücke zu ESP32-EN. GPIO-Konflikte und bestehendes Task-Pinning geprüft; isolierte C++11-Header-Syntaxprüfung mit Arduino-Typstub bestanden; **kein vollständiger ESP32-Build, kein MISRA-/Cppcheck- oder HW-Nachweis**, vorhandene D01/D03/D18 bleiben offen. Details und Pinzahlen ausschließlich im zentralen Pinplan pflegen.
