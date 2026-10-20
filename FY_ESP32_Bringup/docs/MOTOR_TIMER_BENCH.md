# FY ESP32 – MOVE-Inbetriebnahme (HW2, A4988)

Stand 20.10.2026. **Nur loser NEMA11**, keine FY-Mechanik.
`FY_MOTOR_BENCH_TEST=1` in `platformio.ini`. Ohne Bench-Flag
wird MOVE nicht ausgeführt. Original-Nano-Protokoll bleibt unverändert.

## MOVE Telegramm (neue ESP32-Bench-ID 0x0F)

`ID=0F | DATA[0]=MODE|DIR | DATA[1]=STEP_LO | DATA[2]=STEP_HI | CRC8`

- Byte 0, obere 4 Bits: Microstep-Modus 0=voll, 1=halb, 2=viertel,
  3=achtel, 4=sechzehntel.
- Byte 0, Bits 3..1: reserviert, müssen 0 sein.
- Byte 0, Bit 0: 0=links / 1=rechts.
- Byte 1/2: uint16 Schrittzahl little-endian, **1 bis 10000 pro Auftrag**.
- Konstante: `FY_MOVE_MAX_UNREFERENCED_STEPS = 10000U` in
  `include/FY_Move.h`. Dies begrenzt NICHT die kumulierte Wegstrecke.
- Beispiel: `MOVE 11 32 00` = 50 Halbschritte rechts;
  `MOVE 10 32 00` = 50 Halbschritte links;
  `MOVE 11 10 27` = 10000 Halbschritte rechts (ca. 83,3 s, STOP-Test).
- Abweisung: `MOVE 11 11 27` (10001), `MOVE 11 00 00` (0),
  `MOVE 12 32 00` (reservierte Bits),
  `MOVE 50 32 00` (ungültiger Modus).
- Hexbyte-Eingabe über BabelFish: `MOVE 50` ist **kein**
  gültiges Kommando; BYTE0, BYTE1 und BYTE2 müssen alle vorhanden sein.
- `LEFT` und `RIGHT` sind im ESP32-Port außer Betrieb, ihre
  Nano-IDs 0x0B/0x0D bleiben im originalen Nano erhalten.
- Erfolgreiches MOVE antwortet ACK mit DATA=0F: Auftrag **angenommen**,
  nicht abgeschlossen; NACK folgt den Nano-Regeln.
- **Request/Response-Prinzip:** Kein aktives DONE-/Ende-Telegramm. Die
  uebergeordnete Steuerung fragt per `GET_STATUS 20` nach dem Motor.
  Bereits vorhanden: Bit 1 = MOVING (1 faehrt, 0 steht); ein explizit
  codiertes `MotorState_t::IDLE` ist noch nicht implementiert. Das
  spaetere Zustands-Polling ersetzt aktive Meldungen ohne Anfrage.

## Pipeline

BabelFish erkennt MOVE → Huginn validiert Länge, CRC und DATA →
FreeRTOS-Queue → `Motor::prepareMove()` validiert nochmals →
Muninn prüft GO/STOP/BUSY → setzt MS0..2 **nur bei ruhendem Motor** →
`stepTimerStart(steps, direction, 120 Hz)` → Hardware-ISR STEP HIGH/LOW.

Die ISR zählt genau beim Übergang zu LOW einen abgeschlossenen
Schritt und begrenzt auf den Auftrag. Die Nano-Auswertung der
Lichtschranken, Rampen, Profilphasen und Profilwechsel in der LOW-Phase
kommt **erst als nächster Entwicklungsschritt**. Die bisherige
Nano-`Motor::Update()`/`setJob()`/`prepareGo/Stop()`-Logik ist
noch nicht vollständig portiert.

A4988-Moduswechsel wirken ab der nächsten steigenden STEP-Flanke
und können die absolute Position unzuverlässig machen. Nur am
unreferenzierten losen Motor testen; nicht an der FY-Mechanik.

## Prüfsequenz

1. **VMOT AUS** beim Umstecken, Reset und Flash. GPIO-ENABLE hat
   vor dem Boot noch kein vollständig nachgewiesenes Hardware-Failsafe.
2. Neuen Stand bauen und flashen; Bootmeldung:
   `FY ESP32: Huginn ready, Muninn MOVE BENCH (max 10000 steps / 120 Hz)`.
3. MS0 G13 HIGH, MS1 G14 LOW, MS2 G4 LOW (Default: Halbschritt);
   G27 ENABLE HIGH bis zu GO.
4. Vor Motorversorgung A4988 VREF/Stromgrenze, Wicklungspaare,
   3,3-V-VDD, RESET/SLEEP, VMOT-Kondensator und GND prüfen.
   Motor nie unter Spannung ab- oder anstecken.
5. Mit losem Motor: `STOP AA 55`, `GO AA 55`,
   `MOVE 11 32 00`, `MOVE 10 32 00`, `GO 55 AA`.
6. STOP-Test am frei liegenden, ungekoppelten Motor: `GO AA 55`,
   `MOVE 11 10 27` (10000 Schritte), dann **waehrend der Fahrt**
   `STOP 55 AA` per Paste senden, danach `STATUS 20` abfragen.
   Erwartet: Pulszug wird beendet, ENABLE HIGH, MOVING-Bit 0; MOVE
   ist nach STOP verriegelt, bis `STOP AA 55` die Sperre loest.
   50 Halbschritte @120 Hz brauchen ca. 0,42 s; 10000 ca. 83,33 s.
   Pulslänge, Schrittzahl und STOP-Latenz sind **noch nicht gemessen**.
7. STOP/GO OFF deaktivieren ENABLE und stoppen STEP;
   GO ON startet **keine** Bewegung. Keine gültige Referenz-/Position,
   kein frei erfundenes Zielgleis.

**Vor FY-Montage** Bench-Flag aus, Sensor-/Endlagenschutz,
Positionsmodell, Rampen und Referenzierung einbauen.
