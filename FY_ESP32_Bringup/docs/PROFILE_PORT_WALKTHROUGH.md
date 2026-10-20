# FY – Nano zu ESP32: Motorprofile verstehen

Stand: 20.10.2026 · `feature/FY_ESP32_Port` · **Build und Motor-Benchtest auf HW2 mit losem NEMA11 erfolgreich; reale FY-Mechanik/Sensorik weiterhin nicht freigegeben.**

## 1. Dieselbe Idee, andere Verantwortlichkeiten

| Ursprünglicher Nano | Jetzt ESP32 | Bedeutung |
|---|---|---|
| UART/BabelFish empfängt | `lib/UART/UART.cpp`, Huginn (Core 0) | Prüft Telegramm, ID, Laenge und CRC |
| `setJob()` | FreeRTOS `commands` Queue + Muninn-Prepare | Die Queue ist der Transport, **kein** vollstaendiger Ersatz der Motor-State-Machine |
| `prepareSetPosition()` | `Motor::prepareSetPosition()` | Berechnet Richtung, Distanz und prueft Bounds im reinen Bench-Modus |
| `calcProfile(distance)` | `Motor::calcProfile(distance)` | Gleiche sechs Nano-Profilphasen |
| `Timer1_Start()` | `stepTimerStartProfile()` | Kopiert fertig berechnete Phasen und startet STEP; ENABLE bleibt in Muninn |
| `Timer1_ISR()` | `stepTimerIsr()` in `StepTimer.cpp` | HIGH: STEP; LOW: zaehlen, Phase/Rampe, Timerperiode |
| `OCR1A` | `TIMERG1.hw_timer[0].alarm_low` | Naechster Timerabstand in **Mikrosekunden**, nicht AVR-Counts |
| `Motor::Update()` | Muninn-Motorloop | Erkennt Profilende und wechselt MOVING nach IDLE |
| direkte Nano-Zustandsabfrage | `GET_STATUS 20` ueber atomaren Snapshot | Auftraggeber pollt, keine spontane DONE-Meldung |
| reale Position | virtuelle Viertelschrittposition (nur Bench) | REF bleibt ungueltig; nach echtem LS-Referenzlauf ersetzen |

**Codepfad:** `SET_POSITION 7F 73` →
`Huginn::decodeExecute()` →
`xQueueSend(commands)` →
`Muninn_motorMockTask()` →
`Motor::prepareSetPosition()` →
`Motor::calcProfile()` →
`stepTimerStartProfile()` →
`stepTimerIsr()` →
`stepTimerTakeDone()` →
`Motor::onMoveFinished()`.
Der Request bekommt sofort nach Annahme ein ACK ueber die Motor-Event-Queue,
aber **niemals** ein ungefragtes END-/DONE-Telegramm.

## 2. Nano-Periodenwert vs. ESP32-Zeit

Der alte AVR Timer1 lief mit 16 MHz und Prescaler /256:
ein Nano-Zaehlertick dauerte 16 us. Mit Toggle-ISR ist eine
**halbe** STEP-Periode `(OCR1A + 1) * 16 us`.

Im ESP32-Framework laeuft ein Hardwaretimer mit 1 MHz:
`(currentNanoReload + 1) * 16` wird deshalb direkt in Mikrosekunden
als naechste Alarmperiode eingetragen, ohne AVR-Timerregister zu kopieren.

- `V_MIN = 120 STEP/s` -> Reload `259`, Half-period `4160 us`
- `V_MAX = 800 STEP/s` -> Reload `38`, Half-period `624 us`
- Minusbeschleunigung -> Reload wird kleiner -> Motor schneller
- Positive Beschleunigung -> Reload wird groesser -> Motor langsamer

**Arduino-ESP32 2.x** bezeichnet die Version des Arduino-*Frameworks* fuer den ESP32, **nicht** eine andere ESP32-Hardware. Bei 2.x adressiert
`timerBegin(1, 80, true)` tatsaechlich **Timergruppe 1, Timer 0**.
Die Aenderung wird in der IRAM-ISR direkt am Alarmregister
`TIMERG1.hw_timer[0].alarm_low` vorgenommen. Arduino-`timerAlarmWrite()`
wird **nicht** in der ISR aufgerufen. **Dieser Pfad hat den Motor-Benchtest
am 20.10.2026 bestanden.**

**Arduino-ESP32 3.x** ist dagegen eine neuere, inkompatible Framework-API
mit GPTimer. Die sechsphasige **Profilfahrt** ist fuer 3.x noch nicht portiert:
In `stepTimerStartProfile()` liefert der `#if ESP_ARDUINO_VERSION_MAJOR >= 3`-
Zweig absichtlich `false` statt eine falsche Konstantgeschwindigkeit
vorzutaeuschen. Diese Sperre betrifft **nicht die erfolgreich getestete
Firmware** und sagt nichts ueber die ESP32-Hardwareversion aus.
Ein Update auf Arduino-ESP32 3.x braucht deshalb einen eigenen Timer-Port
und erneute HW-Messung. `platformio.ini` legt aktuell keine explizite
Framework-/Plattformversion fest: Bei kuenftigen Neuinstallationen bzw.
Updates muss die tatsaechlich verwendete Version geprueft werden.

## 3. Sechs Stufen, wie beim Nano

| Phase | Default Schritte | Timerdifferenz | Intervall |
|---|---:|---:|---:|
| ACC1 | max. 100 | -2 Ticks | alle 1 STEP |
| ACC2 | max. 100 | -1 Tick | alle 2 STEP |
| KONST | Restdistanz | 0 | unveraendert |
| BRE1 | max. 100 | +1 Tick | alle 2 STEP |
| BRE2 | max. 100 | +2 Ticks | alle 1 STEP |
| POSI | 160 | 0 | unveraendert |

Die **effektiven** Nano-ISR-Rampenintervalle sind 1/2/2/1, obwohl
einige alte Default-Felder 1/1/1/1 auswiesen. Im ESP32-Port entspricht
`MotorProfileParam_t` jetzt der tatsaechlich gefahrenen Nano-ISR.

Beispiele:
- BG3 `32767` -> BG1 `29567`: 3200 Viertelschritte, Phasen
  `100/100/2640/100/100/160`, rechnerisch ca. 6,1 s.
- BG1 `29567` -> BG5 `35967`: 6400 Viertelschritte, Phasen
  `100/100/5840/100/100/160`, rechnerisch ca. 10,1 s.
- Kurze Strecken <= 560 STEP: nur KONST mit V_MIN, keine zappelige Minirampe.

Die Zeiten sind mathematisch simuliert und **noch nicht mit einer Uhr oder
einem Logikanalysator gemessen**. Die Rechenroutine wurde auf exakte
Phasensummen fuer 1..10000 STEP geprueft. **Inzwischen zusaetzlich bestaetigt:**
Der reale, lose NEMA11 fuhr BG3 -> BG1 -> BG5, quittierte beide Auftraege
und erreichte laut internem STEP-Zaehler die erwarteten virtuellen
Positionen. Die Nutzerin beurteilte die Bewegung/den Klang positiv.
Die exakten Geschwindigkeitsstufen, Halbperioden und der STEP-Jitter sind
damit noch **nicht messtechnisch nachgewiesen**.

## 4. Warum die LOW-Halbwelle?

```text
Timer ISR:
  STEP war LOW -> HIGH           // A4988 macht den Schritt
  STEP war HIGH -> LOW           // abgeschlossener Schritt
    completed++
    phaseRemaining--
    ggf. Reload +/- Accel
    ggf. naechste nichtleere Profilphase
    TIMG1 Alarm fuer NAECHSTE Halbwelle aendern
    ggf. Bewegung als beendet markieren
```

Kein `Serial`, keine CRC-Berechnung, kein `digitalWrite`, kein
FreeRTOS-Queue-Versand innerhalb der ISR. Die ISR setzt nur STEP und
ihre eigenen Timing-/Schrittzaehler. Muninn beobachtet das Ende im
Task-Kontext und aktualisiert den Status fuer angefragtes `STATUS 20`.

Endschalter LSL/LSR und Referenzlichtschranke LSREF sind derzeit
**nur kommentierte Einhaengepunkte**. Vor einer mechanisch gekoppelten
FY-Fahrt muessen Grenz- und Referenzlogik nachgeruestet und getestet werden.

## 5. Was die state machine macht – und was noch nicht

- POR startet `STOPPED` und verriegelt; `GO` ohne `STOP AA 55` -> NACK.
- `STOP AA 55` ent-/entriegelt; Treiber bleibt AUS.
- `GO AA 55` -> `IDLE` und Treiber EIN.
- `MOVE` oder Bench-`SET_POSITION` -> `MOVING`.
- ISR-Profilende -> Muninn `IDLE` (erst auf `GET_STATUS 20` sichtbar).
- `STOP 55 AA` -> Treiber AUS, `STOPPED`, STEP-Timer abgebrochen.
- `REFRUN`/echte Zielgleise, physische Grenzen und echtes `SET_POSITION`
  bleiben offen. Der Bench-Mittelpunkt 32767 ist **keine Referenz**.
- Nach STOP oder GO AUS wird im losen Prüfstand die virtuelle Position
  neu auf 32767 gesetzt. Das ist absichtlich keine realistische mechanische
  Positionsfortschreibung nach abgeschaltetem Motor.

## 6. Test und Messpunkte

1. **Build erfolgreich:** RAM **21584 / 327680 B (6,6 %)**,
   Flash **283917 / 1310720 B (21,7 %)**; Buildzeit **8,95 s**.
   Gegenueber Baseline **21544 B RAM / 281981 B Flash**:
   **+40 B RAM, +1936 B Flash**.
2. Motorversorgung beim Flashen **physisch abziehen**, wie für den
   Teststand vereinbart. Solange der Boot-Failsafe offen ist, keine
   mechanisch gekoppelte Bewegung.
3. **UART/POR erfolgreich:** `STOP AA 55` und `GO AA 55` quittiert;
   virtuelle Startposition `GET_POSITION` = `20 FF 7F 01 00 2A` (32767/BG3).
4. **Loser NEMA11 erfolgreich:** `SET_POSITION 7F 73` -> ACK
   `60 13 5E`, anschliessend Position `20 7F 73 01 00 E1` (29567/BG1).
   Danach `SET_POSITION 7F 8C` -> ACK `60 13 5E` und Position
   `20 7F 8C 01 00 CA` (35967/BG5). Motorbewegung und Klang positiv
   bestaetigt; dies sind interne, keine sensorisch gemessenen Positionen.
5. **Status bestaetigt:** Nach erster Fahrt `STATUS 20` ->
   `50 01 00 00 4F` (Treiber aktiv, Zustand IDLE).
   **Noch offen:** STEP-Halbperioden/Rampenstufen und Jitter messen,
   STOP-Abbruch waehrend Profilfahrt und Tests unter Core-0-Last.
6. **Framework-Kompatibilitaet:** Der aktuelle Fahrtest nutzt den
   **Arduino-ESP32-2.x-Timerpfad**. Bei einem spaeteren Wechsel auf
   **Arduino-ESP32 3.x** gibt `stepTimerStartProfile()` derzeit absichtlich
   `false` zurueck. Das ist **keine Fehlfunktion der getesteten
   ESP32-Firmware**, sondern eine offene Portierungsaufgabe fuer eine
   andere Framework-Version. Vor einem Upgrade Version fixieren/pruefen
   und den GPTimer-ISR-Pfad separat implementieren und testen.
