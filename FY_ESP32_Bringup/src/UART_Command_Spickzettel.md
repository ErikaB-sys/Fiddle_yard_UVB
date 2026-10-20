# FY ESP32 – UART-Command-Spickzettel mit Funktions-Ampel

**Stand:** 20.10.2026, nach HW2-Bench-Fahrtest · **Quelle:** aktueller Code in `feature/FY_ESP32_Port/FY_ESP32_Bringup`, nicht die alte Nano-Befehlsbeschreibung. **Nur loser NEMA11, nicht die FY-Mechanik.**

**Roadmap und detaillierte Prepare-Issue-Matrix:** [FY Produktivsoftware – Roadmap](../../Documents/FY_Productivsoftware_Roadmap.md) · **Protokollport:** [#156](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/156) · **Motor:** [#153](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/153), [#154](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/154).

## Ampelregel

- 🔴 **Rot:** Noch keine funktionale ESP32-Implementierung.
- 🟠 **Orange:** Geplant/spezifiziert, aber nicht ausführbar bzw. nur abweisender Stub.
- 🟡 **Gelb:** Im Wesentlichen programmiert, Integration/Verhalten noch unvollständig.
- 🟣 **Violett:** Läuft grundsätzlich; **weitere physische Tests / Grenz- und Lasttests** fehlen.
- 🟢 **Grün:** Im **genannten Geltungsbereich** physisch getestet und dokumentiert.

**⛔ Sicherheits-Gate:** Ein Bench-grüner Befehl ist **keine** Freigabe für die echte Bühne. Die echte Referenzierung, Endlagenerkennung und sichere Boot-/Reset-Freigabe sind offen.

## Alle ESP32-Kommandos: ID, Eingabe, tatsächlicher Status

**Terminalschnittstelle:** 115200 Baud, BabelFish; pro Datenbyte **zwei Hex-Zeichen und ein Leerzeichen zwischen den Bytes**, auch bei uint16 Little Endian. Kein `0x`-Präfix. `STOP` und `STOPP` sind beide gültig; `GET_POSITION` und `POSITION` sind beide gültig. **`getposition` oder `setSET_POSITION` sind ungültig.**

| Command | ID | Beispiel / Daten | Ampel | ESP32-Istzustand / nächster Schritt |
|---|---:|---|---|---|
| `STOPP` / `STOP` | `01` | `STOP 55 AA` sperren; `STOP AA 55` entriegeln | 🟣 | Beide Richtungen/ACK bench-getestet; Abbruch unter neuer Profilfahrt und Worst-Case-Latenz noch nachtesten. Prioritätsqueue vorhanden. |
| `GO` | `07` | `GO AA 55` EIN; `GO 55 AA` AUS | 🟢 **Bench** | Motortreiber-Freigabe/Ausschalten ohne automatisches Losfahren am losen Motor getestet. Boot-Hardware-Failsafe **separat offen**. |
| `MOVE` (**ESP32-Bench-Erweiterung**) | `0F` | `MOVE 21 32 00` = 50 Viertelschritte rechts | 🟣 | Payload, Grenzprüfung, Profil und STEP-Code vorhanden; MOVE mit neuem 6-Stufen-Timer bei unterschiedlichen Lasten/Mikroschrittarten nachtesten. Nicht im alten Nano-Command-Table enthalten. |
| `SET_POSITION` (**virtuell**) | `13` | `SET_POSITION 7F 73` (BG1); `SET_POSITION 7F 8C` (BG5) | 🟢 **Bench** | BG3→BG1→BG5 erfolgreich mit ACK/Positionsrückmeldung. Nur `FY_MOTOR_BENCH_TEST=1` und **ohne echte Referenz**. |
| `SET_POSITION` (**real**) | `13` | dieselben Bytes, echte mechanische Position | 🟠 ⛔ | Bei deaktiviertem Bench-Flag ausdrücklich gesperrt; REF, reale Grenzen, kalibrierte Koordinaten und Ausführung fehlen. |
| `GET_POSITION` / `POSITION` | `15` | `position` (keine Daten) | 🟢 **Bench** | Antwort `20` + 4 Bytes LE + CRC; Positions-Snapshot/Markierung Bit16 getestet. Echte Position noch offen. |
| `GET_STATUS`, Motor | `19` | `status 20` | 🟢 **Bench** | Antwort-ID `50`, DATA[0] ENABLE/MOVING und DATA[1] MotorState; `50 01 00 00 4F` als IDLE bestätigt. |
| `GET_STATUS`, System | `19` | `status 10` | 🟡 | Antwort-ID `10`, vorläufig fester Initialisierungs-/UART-Status statt vollständiger Systemdiagnose. |
| `GET_STATUS`, Sensoren | `19` | `status 40` | 🟠 | Antwort-ID `41`; nur Null-Platzhalter, **keine** echten LS/Hall-Werte. |
| `GET_STATUS`, Keyboard | `19` | `status 50` | 🟠 | Antwort-ID `51`; nur `0` = nicht verbunden, kein Keyboard angeschlossen. |
| `GET_ERROR` / `ERROR` | `1A` | `error` | 🟡 | UART-Fehlerspeicher und Antwort-ID `F0` implementiert; Motor-/Sensorfehler noch nicht systemweit vollständig. |
| `GET_TRACK` / `TRACK` | `23` | `track 00` | 🟠 | Für Option `00` derzeit `DEV_RESPONSE` mangels Track-Snapshot, **keine** gültige aktuelle Gleisnummer. |
| `SET_TRACK` | `1C` | `set_track 03` | 🟠 ⛔ | UART validiert 1…5; Muninn weist ab; echte `prepareSetTrack` fehlt. |
| `REFERENCE` | `16` | `reference 00` oder `reference 01` | 🟠 ⛔ | Byte 0/1 wird validiert, `Motor::prepareReference` weist absichtlich ab. Keine realen LS-/Hall-/Endschalter. |
| `SET_PARAM` | `2B` | `set_param 01 50 00 00` | 🟠 | Telegrammform vorhanden; aktuelle ESP32-Motorlogik übernimmt noch keine Profil-/Kalibrierwerte per Kommando. |
| `SET_MODE` | `26` | `set_mode 55 aa` (LOCAL); `set_mode aa 55` (REMOTE) | 🟠 | Muster geprüft, mangels Keyboard/Modusverwaltung absichtlich `NOT_READY`; keine Umschaltung. |
| `LEFT`, `RIGHT` (Nano-Legacy) | `0B`, `0D` | `left`, `right` | 🔴 **bewusst** | Im ESP32-Router absichtlich `INVALID_COMMAND`; klären, ob überhaupt neu benötigt oder vollständig von `SET_TRACK`/MOVE abgedeckt. |
| `HELP` | `25` | `help` | 🟡 | Antwort `18` mit norwegischem Hilfetext implementiert; Firmware-Link zeigt derzeit noch auf **Nano**-Spickzettel. |
| `GET_FIRMWARE` / `FIRMWARE` | `27` | `firmware` | 🟡 | String-Antwort `28` implementiert; `FW_GIT_COMMIT` fällt ohne Build-Definition auf `unknown` zurück. |

### Protokoll / Rückgabewerte

| Funktion | Ampel | Bemerkung |
|---|---|---|
| BabelFish ASCII→Binär + Hex-Parsing, CRC8, Command-ID/Längenprüfung | 🟢 **Bench** | Gültige und ungültige Eingaben sowie binäre Antworten beobachtet. |
| `ACK = 60 <Command-ID> <CRC>` | 🟢 **Bench** | ACK bedeutet **angenommen**, nicht: Bewegung abgeschlossen. |
| `NACK = 70 <Ort/Grund> <CRC>` | 🟢 **Bench** | High-Nibble = Modul, Low-Nibble = Ablehnungsgrund. Beispiel `70 24 AB` = Motor/INVALID_PARAMETER. |
| Motorantworten über Event-Queue und Sequenz-ID | 🟣 | Funktioniert im Bench; Belastung, Queue-Überlauf, STOP-Race und Korrelations-Stresstest fehlen. |
| Bewegungsende / DONE | 🟢 **Vertragsregel** | **Keine** ungefragte DONE-/END-UART-Meldung! Controller fragt `STATUS 20` bis IDLE. Diese Regel ist im Code umgesetzt und im Bench beobachtet. |
| Firmware-Identität und HELP-Link | 🟡 | Strings vorhanden, Git-Hash/Help-URL noch bereinigen. |
| Aktive HW-Notabschaltung, Sensorfehler, OTA-Neustart | 🟠 ⛔ | Noch keine durchgängige funktionale Freigabe; nicht durch ACK/STOP allein abgedeckt. |

## Datenformate und konkrete Testtelegramme

- `uint8`: ein Hex-Byte, Beispiel `SET_TRACK 03` (derzeit NACK).
- `uint16`: **zwei Little-Endian-Hexbytes**, Beispiel BG1 = dezimal 29567 = `0x737F` → `SET_POSITION 7F 73`.
- `SET_PARAM`: Parameter-ID + drei Rohwertbytes, z. B. `SET_PARAM 01 50 00 00` (momentan NACK).
- `MOVE`: DATA[0] oberes Nibble = Mikroschritt-Modus **0 voll / 1 halb / 2 viertel / 3 achtel / 4 sechzehntel**, Bit 0 = Richtung (0 links, 1 rechts), Bits 3..1 reserviert; DATA[1..2] Schrittzahl LE **1..10000**.
- Nur Bench-`SET_POSITION`: **1..65534** als virtueller Positionsraum; maximal **10600 Viertelschritte je SET_POSITION-Auftrag**; andere Mikroschrittauflösung kann die Bench-Position ungültig machen.
- Status Motor, DATA[0]: Bit0 Treiber an, Bit1 in Bewegung; DATA[1] **ESP32-Erweiterung** des Nano-Protokolls: `0 IDLE / 1 CONF / 2 REFRUN / 3 MOVING / 4 STOPPED / 5 ERROR`. Kompatibilitätsfreigabe hierzu noch in [#156](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/156).
- Bench-`GET_POSITION`: `0x00010000 | virtuelle_16bit_position`, also DATA[2] = `01`; **kein echter REF-Nachweis**. `POSITION_UNKNOWN` (ohne Bench-Bit) = `0x00007FFF`.
- NACK-Typen: `1 BUSY`, `2 NOT_READY`, `3 INVALID_COMMAND`, `4 INVALID_PARAMETER`, `5 EXECUTION_ERROR`, `6 INVALID_TELEGRAM`. Fehlerort z. B. UART `0x10`, Motor `0x20`.

### Bewährte HW2-Sequenz (loser Motor, keine angekoppelte Bühne)

```text
stop aa 55               # POR-STOP-Sperre entriegeln, Treiber aus
go aa 55                 # Motor einschalten
position                 # 20 FF 7F 01 00 2A  -> virtuell BG3
set_position 7f 73       # BG3 -> BG1 (3200 Viertelschritte)
status 20                # nach Fahrt: 50 01 00 00 4F -> IDLE
position                 # 20 7F 73 01 00 E1  -> 29567 (BG1)
set_position 7f 8c       # BG1 -> BG5 (6400 Viertelschritte)
position                 # 20 7F 8C 01 00 CA  -> 35967 (BG5)
stop 55 aa               # STOP aktivieren, ENABLE aus
```

**Bestätigte Plausibilitätsprüfung:** Von BG5 `set_position 50 50` versucht **15407** Schritte und wird mit `70 24 AB` (zu langer Einzelfahrweg) abgewiesen. Mit einer Zwischenfahrt über BG1 geht es; keine globale mechanische Grenzüberwachung daraus ableiten.

**Ungetestet für das neue ISR-Profil:** vollständiger STOP-Test während Bewegung, tatsächliche Timer-Halbperioden/Sechs-Phasen-Geschwindigkeiten, Jitter unter Last und reale LS-/Endschalter. Die erfolgreiche STEP-Zählung ist keine unabhängige Messung der realen Bühne.

## Prepare-Funktionen / Funktionen unter der Haube

Die alte Nano-Reihenfolge `validateCommand() → prepare...() → setJob() → Motor::Update() → Timer1` darf nicht wörtlich als ESP32-Architektur gelesen werden. Heute gilt:

```text
BabelFish / Huginn.validateCommand()
        -> FreeRTOS Command-Queue
        -> Muninn_motorMockTask()
        -> Motor::prepareMove() ODER Motor::prepareSetPosition()
        -> Motor::calcProfile()
        -> stepTimerStartProfile() / StepTimer-ISR
        -> Muninn.onMoveFinished() / Positions-Snapshot
        -> STATUS / POSITION nur auf Anfrage
```

| Alte/aktuelle Prepare-Funktion | Ampel | Aktuelle Bedeutung / Issue |
|---|---|---|
| `prepareMove()` | 🟣 | Bench-relative Bewegung, noch unterschiedliche Microsteps/Last prüfen; [#49](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/49) / [#154](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/154) |
| `prepareSetPosition()` **virtuell** | 🟢 **Bench** | Dokumentiert und praktisch gefahren; [#78](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/78) |
| `prepareSetPosition()` **real** | 🟠 ⛔ | Real-Referenz und Grenzprüfung fehlen; [#78](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/78) / [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155) |
| `prepareLeft()`, `prepareRight()` | 🟠 | Eigenständige Nano-Funktionen nicht portiert; Funktionsbedarf neu entscheiden; [#75](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/75) / [#76](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/76) |
| `prepareSetTrack()` | 🟠 ⛔ | Reale Track-Tabelle und Bewegung fehlen; [#77](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/77) |
| `prepareReference()` | 🟠 ⛔ | Nur Abweisungs-Stub, keine LS-Auswertung; [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155) |
| `prepareParameter()` | 🟠 | SET_PARAM nur Telegramm, keine Übernahme; [#79](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/79) |
| `CheckBorder()` | 🟡 Bench / 🟠 Real ⛔ | Bench-Bounds vorhanden; echte mechanische Grenze fehlt; [#81](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/81) |
| `prepareGo()` / `prepareStop()` | 🟣 | Aufgabe in Muninn statt gleichnamiger Funktionen; HW-Reset/Failsafe offen; [#86](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/86) |
| `setJob()` und `Update()` | 🟣 | Aufgaben auf Queue, Muninn-Motorloop und ISR verteilt; Stress-/Fehlertests offen; [#48](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/48), [#50](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/50) |
| Prepare-NACK-/Fehlersemantik | 🟡 | Grundtypen vorhanden, vollständige Motor-/REF-Fehler offen; [#80](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/80) |

**Historie:** Der alte Nano-UART-Vertrag bleibt separat unter [FY_Controller_Motor/src/UART_Command_Spickzettel.md](../../FY_Controller_Motor/src/UART_Command_Spickzettel.md) und im gesicherten Branch `baseline/Nano-2026-10-19` erhalten. Dies hier ist die **ESP32-Istfassung** und kein Anspruch, dass alle Nano-Befehle portiert seien.

**Vor dem realen FY-Betrieb:** `FY_MOTOR_BENCH_TEST=1` in `platformio.ini` abschalten, aber **nicht** einfach die reale Positionierung freischalten: erst Sensoren, sichere Endlagen, Boot-/OTA-Hardwarefreigabe, Referenzfahrt und Hardwareabnahme implementieren und verifizieren.
