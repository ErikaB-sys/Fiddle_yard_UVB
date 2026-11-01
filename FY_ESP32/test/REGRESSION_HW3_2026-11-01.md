# FY-UVB – HW3-Regressionstest vor Merge

**Datum:** 01.11.2026  
**Branch:** `feature/FY_ESP32_Cleanup`  
**Hardware:** ESP32 / HW3 / A4988 / PCF8574 (0x27)  
**Testergebnis:** OFFEN – erst nach realem Test eintragen.

> Anleitung: [ ] erst nach erfolgreicher Prüfung zu [x] ändern. Fehler und UART-Ausgaben unten notieren. Für einen fehlgeschlagenen Test [ ] belassen und Befund dokumentieren.

## 1. Vorbereitung und Start

- [ ] Cleanup-Branch erfolgreich mit PlatformIO gebaut.
- [ ] Firmware auf HW3 geflasht.
- [ ] Serielles Terminal mit 115200 Baud geöffnet; Startbanner plausibel.
- [ ] Keine unerwarteten Neustarts oder Fehlermeldungen.
- [ ] Keyboard und Motorversorgung geprüft; Fahrweg frei.

## 2. Neue Statusabfragen (UART)

- [ ] `GET_STATUS 10`: SYSTEM-Antwort mit zwei Datenbytes; bei erreichbarem PCF8574 Bit 0 in DATA[0] gesetzt, DATA[1] = 0.
- [ ] `GET_STATUS 50`: KEYBOARD-Antwort mit einem Datenbyte; Bit 0 bei erreichbarem PCF8574 gesetzt.
- [ ] Ohne Keyboard (nur spannungsfrei trennen, neu starten): `GET_STATUS 10` und `50` melden Bit 0 = 0.
- [ ] Keyboard spannungsfrei wieder verbinden, neu starten; beide Abfragen melden wieder Bit 0 = 1.
- [ ] `SET_MODE` meldet erwartungsgemäß NOT_READY (Betriebsartenwechsel noch nicht implementiert).

**Hinweis:** Das I²C-ACK bestätigt nur die Erreichbarkeit des PCF8574, nicht die Funktion aller Tasten.

## 3. Lokale Bedienung und STOP

- [ ] Linksfahrt über Keyboard funktioniert.
- [ ] Rechtsfahrt über Keyboard funktioniert.
- [ ] GO/OK-Taste funktioniert.
- [ ] Rote STOP-Taste hält eine laufende Bewegung an.
- [ ] Nach STOP ist ein kontrollierter neuer Fahrbefehl möglich.

## 4. Referenzierung und Gleise

- [ ] Kurze Referenzierung mit vorhandener gültiger Kalibrierung erfolgreich.
- [ ] Lange Referenzierung bei freiem Fahrweg erfolgreich.
- [ ] BG1 korrekt angefahren.
- [ ] BG3 korrekt angefahren.
- [ ] BG5 korrekt angefahren.
- [ ] Rückfahrt BG5 → BG1; Position/Wiederholgenauigkeit plausibel.

## 5. UART / Fehlerbehandlung

- [ ] `HELP` liefert eine gültige Antwort; enthaltenen Dokumentationslink notieren (Umstellung auf `main` erst **nach** Merge).
- [ ] `GET_PARAM` liefert einen bekannten Parameter korrekt.
- [ ] `GET_POSITION` liefert eine plausible Position.
- [ ] `GET_STATUS 20` und `GET_STATUS 30` liefern plausible Zustände.
- [ ] Ungültige Gleisnummer (z. B. 7) wird abgewiesen.
- [ ] Fahrbefehl während BUSY wird abgewiesen, nicht zur späteren Ausführung vorgemerkt.
- [ ] UART-STOP während einer Bewegung hält die Bühne an.

## 6. NVM und Abschluss

- [ ] Gespeicherte Parameter/Gleispositionen vor Neustart gelesen.
- [ ] Nach Neustart sind die gespeicherten Werte unverändert.
- [ ] Nach Neustart keine unbeabsichtigte Motorbewegung.
- [ ] Abschließende Referenz- und Gleisfahrt erfolgreich.
- [ ] Testergebnis und offene Punkte dokumentiert.

## Testprotokoll

| Feld | Eintrag |
|---|---|
| Testerin | |
| Build / Commit-SHA | |
| Datum / Uhrzeit | |
| UART-Terminal / Port | |
| Auffälligkeiten / Befehle / Antworten | |
| Gesamtergebnis (PASS / FAIL / TEILWEISE) | |
| Freigabe zum Merge (JA / NEIN) | |

## Nach erfolgreichem Regressionstest

- [ ] Cleanup-Branch nach `main` mergen.
- [ ] **Danach** Dokumentationslinks auf `main` prüfen/anpassen, besonders URL im `HELP`-Antworttext.
- [ ] `HELP` auf `main` erneut prüfen und Linkkorrekturen committen.

**Nicht Bestandteil dieses Tests:** JMRI-Anbindung, Binärdecoder, zweite PlatformIO-Firmwarevariante. Diese Arbeiten beginnen erst nach dem Merge.
