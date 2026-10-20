# FY ESP32 – Motor- und UART-Portierungsstand (20.10.2026)

Quellen: FY_Controller_Motor/src/UART_Command_Spickzettel.md,
aktiver Branch `feature/FY_ESP32_Port`; HW2-Testprotokolle vom 20.10.2026.

## Verbindliche Architektur

Huginn (Core 0) empfängt UART, prüft Telegramme und legt Aufträge in
FreeRTOS-Queues. Muninn (Core 1) prüft/führt aus, antwortet mit einem
internen, sequenzkorrelierten Motor-Event. Huginn sendet **nur** die
angefragte Antwort (ACK, NACK oder GET-Response).
**Keine unaufgeforderten DONE-/Alive-Meldungen.** Eine übergeordnete
Instanz pollt `STATUS 20` bis `MotorState_t::IDLE`.

MotorState-IDs aus `Motor.h` in STATUS_Motor (`0x50`):
- DATA[0] Bit 0 = Treiber ENABLED, Bit 1 = MOVING.
- DATA[1] **ESP32-Port-Erweiterung:** Motorzustand (0 IDLE,
  1 CONF, 2 REFRUN, 3 MOVING, 4 STOPPED, 5 ERROR).
- DATA[2] reserviert 0.

Testtelegramme nach Neu-Flash (noch nicht HW-bestaetigt):
- Direkt nach POR: `STATUS 20` -> `50 00 04 00 70` (STOPPED).
- Nach `STOP AA 55` und `GO AA 55`: `50 01 00 00 4F` (IDLE, ENABLED).
- Waehrend `MOVE 11 10 27`: `50 03 03 00 A6` (MOVING, ENABLED).
- Nach `STOP 55 AA`: `50 00 04 00 70` (STOPPED, DISABLED).

**Achtung:** DATA[1] war im Nano-Spickzettel reserviert.
Diese ESP32-Erweiterung muss beim finalen Protokollabgleich festgelegt
werden; ein produktiver Referenzstatus wird dadurch nicht simuliert.

## Command-Status (Soll aus Nano-Spickzettel / Ist ESP32)

| Kommando | Status | Ist |
|---|---|---|
| GO 0x07 | funktioniert | ENABLE EIN/AUS mit STOP-POR-Sperre, hardwaregetestet |
| STOPP 0x01 | funktioniert im Bench | Prioritätsqueue; NEMA11 stoppt sofort, Versorgungstrom am Netzteil 0; Worst-Case-Latenz nicht gemessen |
| MOVE 0x0F | funktioniert im Bench | ESP32-Neubefehl; Richtung, Schrittzahl, A4988-Modus, bis 10000 pro Auftrag; 10000-MOVE und STOP real getestet; neuer 1/4-Default noch nicht auf HW2 geprüft |
| GET_STATUS 0x19 | teilweise | Alle 4 Selektoren; 20 ENABLE/MOVING und jetzt DATA[1] MotorState; 10/40/50 nur Initial-/Platzhalterdaten |
| GET_ERROR 0x1A | teilweise | UART-Fehler vorhanden; Motor-Fehlerkette noch nicht vollständig |
| GET_POSITION 0x15 | teilweise | Im Bench eine **markierte virtuelle Viertelschrittposition**, im Realbetrieb bis zur Referenz POSITION_UNKNOWN |
| GET_TRACK 0x23 | nicht portiert | DEV_RESPONSE mangels Track-Snapshot |
| HELP 0x25 | teilweise | String/CRC, verweist noch auf alten Nano-Spickzettel |
| GET_FIRMWARE 0x27 | teilweise | Build-/Versionsinfo, Git-Commit möglicherweise unknown |
| REFERENCE 0x16 | nicht portiert | Handler und REFRUN vorbereitet; keine reale Referenz, NACK |
| SET_PARAM 0x2B | nicht portiert | Nur Protokoll/Queue; keine Parameterverarbeitung |
| SET_POSITION 0x13 | Bench-only | Virtuelle absolute Zielposition wird mit Viertelschritten abgefahren; echte FY-Fahrt ohne REF bleibt gesperrt |
| SET_TRACK 0x1C | nicht portiert | Range-Prüfung 1..5, noch keine Gleisfahrt |
| SET_MODE 0x26 | nicht portiert | Bitmuster geprüft, mangels Keyboard NACK |
| LEFT/RIGHT 0x0B/0x0D | im ESP32 bewusst abgelöst | NACK; auf Nano unverändert |

## Response-Status

ACK (0x60) und NACK (0x70) über FreeRTOS-Queue, auf HW2 bestätigt.
System, Motor, Error, Position und Keyboard/Switches als Antworten,
teilweise nur Platzhalter. REFERENCE/TRACK und Betriebsartenergebnisse
fehlen. GETs werden nur auf Anfrage gesendet.

## Motorprofil

`Motor::calcProfile(uint16_t)` aus Nano in Motor.cpp:
ACC1, ACC2, KONST, BRE1, BRE2, POSI. `prepareMove()` berechnet ab jetzt
das Profil und prüft die exakte Schrittsumme. Für 1..10000 Schritte
rechnerisch auf Schrittsumme geprüft; **nicht auf HW mit Rampen geprüft**.

**Der StepTimer fährt gegenwärtig nur 120Hz Konstantgeschwindigkeit.**
Die variable Halbperioden-Nachladung aus der LOW-Halbwelle, Profilphasen,
Sensorprüfungen und Min/Max-Rate sind noch zu integrieren und zu messen.

MotorState-Grundgerüst in Muninn: POR/STOP -> STOPPED;
STOP-Freigabe (Treiber bleibt AUS); GO EIN -> IDLE;
MOVE -> MOVING; Pulsende -> IDLE; GO AUS/STOP -> STOPPED.
CONF/REFRUN/ERROR sind vorgehalten. Eine echte REFRUN mit Referenzsensoren
und referenzierte SET_POSITION sind **absichtlich gesperrt**.
Die drei Sensor-/Endschalter dürfen nicht mit Dummy-Werten als
funktionsfähige Grenzsicherung ausgegeben werden.

## Hardware

NEMA11/A4988 am losen HW2-Testaufbau: Richtung, GO, MOVE, STOP,
POR-STOP-Sperre bestätigt. Beim Flashen VMOT physisch abstecken.
Keine künstlichen STEP/DIR-Startpulse mehr. Vor festem Bühnenbetrieb
hardwareseitige Boot-/RESET-Sicherung und Sensorabnahme klären.

## Nächste Arbeitsblöcke

1. ESP32-Hardwaretimer mit variablen Halbperioden und sechs Phasen
   (Nano-Timerwerte nicht 1:1 mit ESP32-Mikrosekunden gleichsetzen).
2. Reproduzierbarer Build, SRAM/Flash-Ausgabe, Profil-Messung mit
   abgestecktem Motor und anschließend losem NEMA11.
3. REFRUN mit realen Sensoren und eindeutiger Positionsgültigkeit;
   erst dann SET_POSITION für tatsächliche Fahrten aktivieren.
4. HW3-Trägerplatine, NEMA14-Treiberstrom, ggf. TB6600/Pegelstufe.
5. Parameter und Speicher: ESP32 NVS/Preferences als erste Option prüfen;
   externe I2C-FRAM/EEPROM nur bei zusätzlichem Bedarf.
6. Keyboard; später SPI-Erweiterung.

Kein produktiv sicherer FY-Anlagenbetrieb freigegeben.

## Neuer virtueller SET_POSITION-Test – 20.10.2026 (noch ungebaut)

Benutzerentscheidung: `SET_POSITION` darf **am frei liegenden Tischmotor**
ohne echte Referenz fahren, weil mechanisch nichts angekoppelt ist. Das
gilt NUR bei `FY_MOTOR_BENCH_TEST=1`; `_ReferenceValid=false` bleibt
**immer** gesetzt, bis reale Referenzsensorik implementiert wird.

- Start bei `uint16_t max / 2 = 32767`; A4988-Default 1/4-Schritt.
- Nano-HW-Test Gleisteilung `1600` Viertelschritte:
  virtuelle BG1=29567, BG2=31167, BG3=32767, BG4=34367, BG5=35967.
- Nominaler Testweg `1000 + 6*1600 = 10600` pro SET_POSITION-Auftrag,
  keine harte absolute Bewegungsfensterbegrenzung.
- Einzelne virtuelle `uint16_t`-Positionen nur 1..65534; mit
  32-Bit-Zwischenrechnung werden Ueber- und Unterlauf **vor** START geprüft.
  Ein entkoppelter Tischmotor kann nicht als sicherer Anlagentest gelten.
- `GET_POSITION` markiert die Simulation durch Bit 16 (`0x00010000`);
  niederwertige 16 Bits enthalten die Viertelschritt-Position. Das
  Anfangstelegramm ist `20 FF 7F 01 00 2A` einschließlich CRC.
- STEP-Zähler wird nur in Muninn (Core 1) nach einer ISR-gelesenen,
  per Critical Section gesicherten Snapshotzahl verarbeitet;
  UART (Core 0) liest eine atomare Positionskopie.
- Nach STOP/GO AUS wird für den Tischtest bewusst auf den **synthetischen**
  Startwert 32767 re-ankert. Es gibt keine echte Rückreferenzierung.
  Bei MOVE mit einer anderen Auflösung als 1/4 verliert die
  Viertelschritt-Koordinate ihre Gültigkeit bis zur Neuverankerung.
- `SET_POSITION` veranlasst ebenfalls `calcProfile()`, startet aber
  derzeit weiterhin einen **konstanten 120-Hz-STEP-Pulszug**. Die
  Echtzeit-Rampen bleiben ein separates, offenes #153-Thema.

**Gemeldete Speicher-Baseline des unmittelbar vorangegangenen Release-Builds:**
RAM 21544 von 327680 Byte = 6,6 %; Flash 281981 von 1310720 Byte =
21,5 %; Build erfolgreich in 4,05 s. Diese Werte stammen **vor**
der neuen SET_POSITION-Änderung; noch kein neuer Build-Bericht.

Vollständige Hex-Testfolge in `docs/MOTOR_TIMER_BENCH.md`.
