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
| MOVE 0x0F | funktioniert im Bench | ESP32-Neubefehl; Richtung, Schrittzahl, A4988-Modus, bis 10000 pro Auftrag; 10000-MOVE und STOP real getestet |
| GET_STATUS 0x19 | teilweise | Alle 4 Selektoren; 20 ENABLE/MOVING und jetzt DATA[1] MotorState; 10/40/50 nur Initial-/Platzhalterdaten |
| GET_ERROR 0x1A | teilweise | UART-Fehler vorhanden; Motor-Fehlerkette noch nicht vollständig |
| GET_POSITION 0x15 | teilweise | 4 Byte, POSITION_UNKNOWN bis echte Referenz |
| GET_TRACK 0x23 | nicht portiert | DEV_RESPONSE mangels Track-Snapshot |
| HELP 0x25 | teilweise | String/CRC, verweist noch auf alten Nano-Spickzettel |
| GET_FIRMWARE 0x27 | teilweise | Build-/Versionsinfo, Git-Commit möglicherweise unknown |
| REFERENCE 0x16 | nicht portiert | Handler und REFRUN vorbereitet; keine reale Referenz, NACK |
| SET_PARAM 0x2B | nicht portiert | Nur Protokoll/Queue; keine Parameterverarbeitung |
| SET_POSITION 0x13 | nicht portiert | prepareSetPosition vorhanden, ohne validen REF-Status abgewiesen |
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
