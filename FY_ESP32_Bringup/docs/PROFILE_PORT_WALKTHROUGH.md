# FY – Nano zu ESP32: Motorprofile verstehen

Stand: 20.10.2026 · `feature/FY_ESP32_Port` · Entwicklungsstand, **noch nicht auf HW2 geflasht/getestet**.

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

**ESP32 2.x**: `timerBegin(1, 80, true)` adressiert tatsächlich **Timergruppe 1,
Timer 0**. Die Änderung wird im IRAM-ISR direkt am Alarmregister
`TIMERG1.hw_timer[0].alarm_low` vorgenommen. Arduino-`timerAlarmWrite()`
wird **nicht** in der ISR aufgerufen. Version 3.x verwendet eine andere
GPTimer-API; die Profilfahrt wird derzeit ausdrücklich abgewiesen, bis
die ISR-sichere 3.x-Variante getestet ist. Es gibt keinen stillen
Konstanttakt-Fallback, der eine erfolgreiche Profilfahrt vortäuschen würde.

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

Die Zeiten sind mathematisch simuliert und **noch nicht gemessen**.
Verifiziert wurden fuer die Rechenroutine die exakten Phasensummen fuer
alle 1..10000 STEP. Das beweist noch keine korrekten ESP32-Alarmflanken.

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

1. PlatformIO Build: RAM/Flash mit Baseline **21544 B / 281981 B**
   vergleichen. Neue Werte erst nach einem echten Build.
2. Motorversorgung beim Flashen **physisch abziehen**, wie für den
   Teststand vereinbart. Solange der Boot-Failsafe offen ist, keine
   mechanisch gekoppelte Bewegung.
3. Mit VMOT getrennt Boot-/POR-STOP, `STATUS 20`, neue Timerfrequenz
   mit Logikanalysator nachmessen, falls vorhanden.
4. Nur loser NEMA11: `STOP AA 55`, `GO AA 55`,
   `SET_POSITION 7F 73`, danach `SET_POSITION 7F 8C`.
5. Nach beiden Fahrten `STATUS 20` -> `IDLE`, `GET_POSITION` ->
   virtuelle Bench-Position. Achten auf Richtung, Stufen, Aussetzer
   und STOP-Reaktionszeit.
6. Arduino-ESP32 3.x: Profile werden noch bewusst abgewiesen.
