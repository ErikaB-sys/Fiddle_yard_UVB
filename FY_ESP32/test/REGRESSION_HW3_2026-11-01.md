# FY-UVB – HW3-Regressionstest vor Merge

**Datum:** 01.11.2026  
**Branch:** `feature/FY_ESP32_Cleanup`  
**Hardware:** ESP32 / HW3 / A4988 / PCF8574 (0x27)  
**Testergebnis:** PASSED – reale HW3-Regression mit Freigabe zum Merge dokumentiert; Build/Commit unten beziehen sich auf den getesteten Cleanup-Stand.

> Anleitung: [ ] erst nach erfolgreicher Prüfung zu [x] ändern. Fehler und UART-Ausgaben unten notieren. Für einen fehlgeschlagenen Test [ ] belassen und Befund dokumentieren.

## 1. Vorbereitung und Start

- [x] Cleanup-Branch erfolgreich mit PlatformIO gebaut.
- [x] Firmware auf HW3 geflasht.
- [x] Serielles Terminal mit 115200 Baud geöffnet; Startbanner plausibel.
- [x] Keine unerwarteten Neustarts oder Fehlermeldungen.
- [x] Keyboard und Motorversorgung geprüft; Fahrweg frei.

## 2. Neue Statusabfragen (UART)

- [x] `GET_STATUS 10`: SYSTEM-Antwort mit zwei Datenbytes; bei erreichbarem PCF8574 Bit 0 in DATA[0] gesetzt, DATA[1] = 0.
- [x] `GET_STATUS 50`: KEYBOARD-Antwort mit einem Datenbyte; Bit 0 bei erreichbarem PCF8574 gesetzt.
- [x] Ohne Keyboard (nur spannungsfrei trennen, neu starten): `GET_STATUS 10` und `50` melden Bit 0 = 0.
- [x] Keyboard spannungsfrei wieder verbinden, neu starten; beide Abfragen melden wieder Bit 0 = 1.
- [x] `SET_MODE` meldet erwartungsgemäß NOT_READY (Betriebsartenwechsel noch nicht implementiert).

**Hinweis:** Das I²C-ACK bestätigt nur die Erreichbarkeit des PCF8574, nicht die Funktion aller Tasten.

## 3. Lokale Bedienung und STOP

- [X] Linksfahrt über Keyboard funktioniert.
- [X] Rechtsfahrt über Keyboard funktioniert.
- [x] GO/OK-Taste funktioniert.
- [x] Rote STOP-Taste hält eine laufende Bewegung an.
- [x] Nach STOP ist kein kontrollierter neuer Fahrbefehl möglich.(Veriegelung lösen  )
- [x] nach Freigabe kann wieder gefahren werden 

## 4. Referenzierung und Gleise

- [x] Kurze Referenzierung mit vorhandener gültiger Kalibrierung erfolgreich.
- [x] Lange Referenzierung bei freiem Fahrweg erfolgreich.
- [x] BG1 korrekt angefahren.
- [x] BG3 korrekt angefahren.
- [x] BG5 korrekt angefahren.
- [x] Rückfahrt BG5 → BG1; Position/Wiederholgenauigkeit plausibel.

## 5. UART / Fehlerbehandlung

- [X] `HELP` liefert eine gültige Antwort; enthaltenen Dokumentationslink notieren (Umstellung auf `main` erst **nach** Merge).
- [X] `GET_PARAM` liefert einen bekannten Parameter korrekt.
- [X] `GET_POSITION` liefert eine plausible Position.
- [X] `GET_STATUS 20` und `GET_STATUS 30` liefern plausible Zustände.
- [x] Ungültige Gleisnummer (z. B. 7) wird abgewiesen.
- [x] Fahrbefehl während BUSY wird abgewiesen, nicht zur späteren Ausführung vorgemerkt.
- [x] UART-STOP während einer Bewegung hält die Bühne an.

## 6. NVM und Abschluss

- [x] Gespeicherte Parameter/Gleispositionen vor Neustart gelesen.
- [x] Nach Neustart sind die gespeicherten Werte unverändert.
- [x] Nach Neustart keine unbeabsichtigte Motorbewegung.
- [x] Abschließende Referenz- und Gleisfahrt erfolgreich.
- [x] Testergebnis und offene Punkte dokumentiert.

## Testprotokoll

| Feld | Eintrag |
|---|---|
| Testerin | TEAM DINGSDA  |
| Build / Commit-SHA |78c920bde17fd0e68b55962cd56f84d461cd5a7e |
| Datum / Uhrzeit | |
| UART-Terminal / Port | |
| Auffälligkeiten / Befehle / Antworten | |
| Gesamtergebnis (PASS / FAIL / TEILWEISE) |Passed |
| Freigabe zum Merge (JA / NEIN) | ja |

## Nach erfolgreichem Regressionstest

- [x] Cleanup-Branch nach `main` gemergt: [PR #182](https://github.com/ErikaB-sys/Fiddle_yard_UVB/pull/182) am 01.11.2026.
- [ ] **Danach** Dokumentationslinks auf `main` prüfen/anpassen, besonders URL im `HELP`-Antworttext.
- [ ] `HELP` auf `main` erneut prüfen und Linkkorrekturen committen.

**Nicht Bestandteil dieses Tests:** JMRI-Anbindung, Binärdecoder, zweite PlatformIO-Firmwarevariante. Diese Arbeiten beginnen erst nach dem Merge.

## Nachtrag nach dem Merge (01.11.2026)

- **Cleanup ist bereits in `main`** (PR #182). Die ursprüngliche Bezeichnung „vor Merge“ dokumentiert den Zeitpunkt der HW-Abnahme.
- **Namensbereinigung** `Muninn_*MotorMock` → `Muninn_*Motor` / `Muninn_motorTask` wurde später separat in [PR #183](https://github.com/ErikaB-sys/Fiddle_yard_UVB/pull/183) nach `main` übernommen. Funktionale Motor-/Sensor-/STOP-Logik unverändert.
- **Prüfgrenze:** Der in diesem Protokoll genannte erfolgreiche Build und HW-Test wurden **vor PR #183** durchgeführt. Ein neuer PlatformIO-Build des umbenannten `main`-Heads ist noch nicht protokolliert; siehe [#171](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/171).
