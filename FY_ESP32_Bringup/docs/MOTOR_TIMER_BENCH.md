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
  `MOVE 21 10 27` = 10000 Viertelschritte rechts (Profilfahrt ca. 14,6 s, STOP-Test).
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
  STATUS 20: DATA[0] Bit 1 = MOVING; DATA[1] ist inzwischen die
  MotorState-Enum (0 IDLE, 3 MOVING, 4 STOPPED). Die Rueckmeldung
  erfolgt nur auf Anfrage, nie ungefragt beim Ende.

## Pipeline

BabelFish erkennt MOVE → Huginn validiert Länge, CRC und DATA →
FreeRTOS-Queue → `Motor::prepareMove()` validiert nochmals →
Muninn prüft GO/STOP/BUSY → setzt MS0..2 **nur bei ruhendem Motor** →
`stepTimerStartProfile(steps, direction, profile)` → Hardware-ISR STEP HIGH/LOW.

Die ISR zaehlt beim Uebergang zu LOW einen abgeschlossenen Schritt.
In derselben LOW-Phase aktualisiert sie die jeweilige
ACC1/ACC2/KONST/BRE1/BRE2/POSI-Stufe und die naechste Timerperiode.
Aus dem Nano-Timer1-Tick (16 us) wird eine ESP32-Halbperiode
`(reload+1)*16 us`. Die Grenzen entsprechen 120..800 STEP/s.
Die Rampenintervalle entsprechen der **effektiven** Nano-ISR:
ACC1=1, ACC2=2, BRE1=2, BRE2=1 STEP pro Timerdelta.
WICHTIG: Der neue variable Timerpfad ist noch nicht gebaut oder am
Motor gemessen. Fuer Arduino-ESP32 2.x wird die Group-1/Timer-0
Alarmperiode direkt im ISR-sicheren Register geaendert. Ein 3.x-Build
lehnt Profilfahrten bis zur separaten API-Portierung mit NACK ab.
Die LS-Eingaenge und die vollstaendige Nano-`Motor::Update()`/`setJob()`
Referenzlogik sind weiterhin offen.

A4988-Moduswechsel wirken ab der nächsten steigenden STEP-Flanke
und können die absolute Position unzuverlässig machen. Nur am
unreferenzierten losen Motor testen; nicht an der FY-Mechanik.

## Virtueller SET_POSITION-Tischtest (NEU, noch nicht auf HW2 gebaut)

Nur bei `FY_MOTOR_BENCH_TEST=1`, loser NEMA11, keine FY-Buehne.
**START:** `uint16_t(65535/2) = 32767 = 0x7FFF`, virtuelle BG3.
A4988 startet im **1/4-Mikroschrittmodus** (MS1 LOW, MS2 HIGH, MS3 LOW).

Die von Nano/HW-Test uebernommene Gleisteilung ist **1600
Viertelschritte**. Ein nominaler Verfahrbereich ist
`1000 + 6 * 1600 = 10600` Schritte. Dieser Wert begrenzt beim
Bench-`SET_POSITION` die **Einzelfahrt**, nicht die gesamte kumulierte
Position. Fuer den Schreibtischtest sind wiederholte Fahrten im
`uint16_t`-Raum von 1 bis 65534 moeglich; eine geplante Fahrt darf
niemals zu 0, 65535 oder jenseits der Grenze fuehren. Ohne reale Sensoren
ist das **kein mechanischer Sicherheitsabstand**.

| Ziel | Virtuell dezimal | Hex-Bytes in SET_POSITION |
|---|---:|---|
| BG1 | 29567 | `SET_POSITION 7F 73` |
| BG2 | 31167 | `SET_POSITION BF 79` |
| BG3 | 32767 | `SET_POSITION FF 7F` |
| BG4 | 34367 | `SET_POSITION 3F 86` |
| BG5 | 35967 | `SET_POSITION 7F 8C` |

`SET_POSITION` bleibt in einem Nicht-Bench-Build **gesperrt, solange
keine reale Referenzierung vorliegt**. `_ReferenceValid` wird durch den
Tischtest niemals auf true gesetzt. `REFERENCE` bleibt gesperrt.
Bereitgestellt wird ausschliesslich `_BenchPositionValid`.

`GET_POSITION` bleibt 4 Datenbytes (Little Endian). Zur eindeutigen
Kennzeichnung der virtuellen Testposition wird in Bench-Builds **Bit 16
gesetzt**: `0x00010000 | uint16-Position`, z. B. am Start
`20 FF 7F 01 00 2A`. **Kein** normales referenziertes
Nano-Positionsdatum! Nach ungültiger Bench-Position bleibt der alte
UNKNOWN-Wert `0x00007FFF`. Es gibt keine ungefragte Positionsmeldung,
nur GET auf Anfrage.

Die aktuelle Schrittzahl stammt aus dem ISR-Zaehler und wird alle
ca. 2 ms in Muninn in die Position uebernommen. Absolute Fahrt,
Statusabfrage und relative MOVE-Fahrten werden somit im Viertelschritt
konsistent fortgeschrieben. MOVE mit einer anderen Mikroschrittauflösung
macht die virtuelle Viertelschrittposition ungueltig. Erst STOP/GO-AUS
und neue Freigabe starten den **simulierten** Positionsnullpunkt erneut
bei 32767. STOP/GO-AUS koennen eine physische Position nicht bewahren.

**Testsequenz nach POR** (VMOT beim Flashen aus):
```text
GO AA 55                 # NACK, POR ist verriegelt
STOP AA 55               # Entriegeln
GO AA 55                 # Motor EIN
GET_POSITION             # 20 FF 7F 01 00 2A, virtuelle 32767
SET_POSITION 7F 73       # BG3 -> BG1, -3200; Nano-Profil ca. 6,1 s
STATUS 20                # Bit 1 waehrend Bewegung = 1
GET_POSITION             # Nach Fahrt: 20 7F 73 01 00 E1
SET_POSITION 7F 8C       # BG1 -> BG5, +6400; Nano-Profil ca. 10,1 s
STOP 55 AA               # Sofort AUS + Sperre aktiv, Bench-Pos neu verankert
```

**Neu implementiert:** Der sechsphasige Profil-Executor in der ISR
mit periodischer Rampenaenderung in der LOW-Phase; die Berechnung
kommt aus `Motor::calcProfile()`. **Noch nicht gebaut oder auf HW2
abgenommen.** Das zuvor nachgewiesene 120-Hz-Konstantprofil bleibt als
separater Diagnosetarif `stepTimerStart()` im Code. Neuer RAM/Flash
Verbrauch steht erst nach dem neuen Build fest.

## Power-On-Reset: STOP verriegelt

**Nach jedem Einschalten oder Reset** startet Muninn mit STOP-Sperre
(`stopActive=true`), A4988 ENABLE HIGH und STEP LOW. Es gibt beim
Firmwarestart keine künstlichen STEP/DIR-Testpulse mehr.

- `GO AA 55` direkt nach POR -> `NACK Motor/NOT_READY` (`70 22 B9`).
- `MOVE 11 32 00` direkt nach POR -> `NACK Motor/NOT_READY`.
- `STOP AA 55` -> `ACK 01`; STOP freigegeben, Treiber bleibt AUS.
- `GO AA 55` -> `ACK 07`; Motorstrom wird freigegeben.
- `MOVE 11 32 00` -> `ACK 0F`; STEP-Pulszug erlaubt.
- `STOP 55 AA` -> ENABLE HIGH, STEP beendet, STOP wieder verriegelt.
- Nach erneutem Reset gilt dieselbe Verriegelung, auch wenn der Treiber
  zuvor eingeschaltet war.

Dies ist eine **Software-Verriegelung ab Start der Anwendung**, **kein
Hardware-Interlock waehrend ROM-Bootloader/Flash/Reset-Pegelwechseln**.
VMOT bei Flash/Reset AUS; bevor die FY-Mechanik angeschlossen wird,
externen ENABLE-Failsafe/VMOT-Trennschalter realisieren und pruefen.

## Prüfsequenz

1. **VMOT AUS** beim Umstecken, Reset und Flash. GPIO-ENABLE hat
   vor dem Boot noch kein vollständig nachgewiesenes Hardware-Failsafe.
2. Neuen Stand bauen und flashen; Bootmeldung:
   `FY ESP32: Huginn ready, Muninn PROFILE BENCH (120..800 Hz)`.
3. MS0 G13 LOW, MS1 G14 HIGH, MS2 G4 LOW (neuer Default: Viertelschritt);
   G27 ENABLE HIGH bis zu GO.
4. Vor Motorversorgung A4988 VREF/Stromgrenze, Wicklungspaare,
   3,3-V-VDD, RESET/SLEEP, VMOT-Kondensator und GND prüfen.
   Motor nie unter Spannung ab- oder anstecken.
5. Nach POR zuerst GO ohne STOP-Freigabe testen (NACK); danach mit losem Motor: `STOP AA 55`, `GO AA 55`,
   `MOVE 21 32 00`, `MOVE 20 32 00`, `GO 55 AA` (Viertelschritt).
6. STOP-Test am frei liegenden, ungekoppelten Motor: `GO AA 55`,
   `MOVE 21 10 27` (10000 Viertelschritte), dann **waehrend der Fahrt**
   `STOP 55 AA` per Paste senden, danach `STATUS 20` abfragen.
   Erwartet: Pulszug wird beendet, ENABLE HIGH, MOVING-Bit 0; MOVE
   ist nach STOP verriegelt, bis `STOP AA 55` die Sperre loest.
   Rechnerisch: 50 Viertelschritte ca. 0,42 s und 10000 Profil-STEP
   ca. 14,6 s. Die neue Rampenform, Schrittzahl und STOP-Latenz
   sind **noch nicht am Motor gemessen**.
7. STOP/GO OFF deaktivieren ENABLE und stoppen STEP;
   GO ON startet **keine** Bewegung. Keine gültige Referenz-/Position,
   kein frei erfundenes Zielgleis.

**Vor FY-Montage** Bench-Flag aus, Sensor-/Endlagenschutz,
Positionsmodell, ISR-Sensorabfragen und reale Referenzierung
fertigstellen sowie Rampenprofil auf HW pruefen.
