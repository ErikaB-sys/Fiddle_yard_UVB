# FY ESP32 – UART-Command-Spickzettel mit Funktions-Ampel

**Stand:** 23.10.2026, HW3-Motorlauf und Fake-REF/SET_TRACK physisch im Bench getestet · **Quelle:** aktueller Code in `feature/FY_ESP32_Port/FY_ESP32_Bringup`, nicht die alte Nano-Befehlsbeschreibung. **Nur loser NEMA11, nicht die FY-Mechanik.**

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
| `MOVE` (**ESP32-Bench-Erweiterung**) | `0F` | `MOVE 21 32 00` = 50 Viertelschritte rechts | 🟣 | HW3: 1/4-Schritt (`MOVE 21 10 27`) und 1/16-Schritt gegensinnig (`MOVE 40 10 27`) erfolgreich bewegt, 1/16 deutlich leiser. Frequenz, Profilphasen und mechanische Schrittzahl noch nicht unabhängig gemessen. |
| `SET_POSITION` (**virtuell**) | `13` | Nach `REFERENCE 00`: `SET_POSITION 15 06` (BG1=1557), `SET_POSITION 15 1F` (BG5=7957) | 🟣 **Bench** | Neue HW3-Koordinaten aus HW_TEST; alte HW2-Serie (Start 32767) bereits geprüft, die neuen absoluten `SET_POSITION`-Ziele noch nicht separat getestet. |
| `SET_POSITION` (**real**) | `13` | dieselben Bytes, echte mechanische Position | 🟠 ⛔ | Bei deaktiviertem Bench-Flag ausdrücklich gesperrt; REF, reale Grenzen, kalibrierte Koordinaten und Ausführung fehlen. |
| `GET_POSITION` / `POSITION` | `15` | `position` (keine Daten) | 🟢 **Bench** | Antwort `20` + 4 Bytes LE + CRC; Positions-Snapshot/Markierung Bit16 getestet. Echte Position noch offen. |
| `GET_STATUS`, Motor | `19` | `status 20` | 🟢 **Bench** | Antwort-ID `50`, DATA[0] ENABLE/MOVING und DATA[1] MotorState; `50 01 00 00 4F` als IDLE bestätigt. |
| `GET_STATUS`, Referenz | `19` | `status 30` | 🟢 **Bench** | HW3: `30 02 9E` = simulierte Referenz; `00`=ungültig, `01`=echt (noch nicht implementiert), `02`=FAKE. |
| `GET_STATUS`, System | `19` | `status 10` | 🟡 | Antwort-ID `10`, vorläufig fester Initialisierungs-/UART-Status statt vollständiger Systemdiagnose. |
| `GET_STATUS`, Sensoren | `19` | `status 40` | 🟠 | Antwort-ID `41`; nur Null-Platzhalter, **keine** echten LS/Hall-Werte. |
| `GET_STATUS`, Keyboard | `19` | `status 50` | 🟠 | Antwort-ID `51`; nur `0` = nicht verbunden, kein Keyboard angeschlossen. |
| `GET_ERROR` / `ERROR` | `1A` | `error` | 🟡 | UART-Fehlerspeicher und Antwort-ID `F0` implementiert; Motor-/Sensorfehler noch nicht systemweit vollständig. |
| `GET_TRACK` / `TRACK` | `23` | `track 00` | 🟢 **Bench** | HW3: `40 05 00 01 41` während Fahrt nach BG5, dann `40 05 05 00 07` bei Ankunft. DATA = Ziel / erreicht / Zustand (00 erreicht, 01 fährt, 02 unbekannt). |
| `SET_TRACK` | `1C` | `set_track 03` | 🟢 **Bench** / 🟠 **Real** | HW3: `SET_TRACK 01`, `03`, `05` angenommen; BG1 und BG5 mit `GET_TRACK` als erreicht bestätigt. `Motor::prepareSetTrack()` übernimmt die fünf vermessenen HW_TEST-Koordinaten. Echte Bühnenfahrt gesperrt. |
| `REFERENCE` | `16` | `reference 00` / `reference 01` | 🟢 **Fake-Bench** / 🟠 **Real** | HW3: `REFERENCE 00` → `60 16 45`, `STATUS 30` → `30 02 9E`. Nur virtueller Positionsanker 1000 ohne Bewegung; `REFERENCE 01` und echte Referenz weiterhin gesperrt. |
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

- `uint8`: ein Hex-Byte, Beispiel `SET_TRACK 03` (mit expliziter Fake-REF im Bench ACK).
- `uint16`: **zwei Little-Endian-Hexbytes**, Beispiel BG1 = dezimal 1557 = `0x0615` → `SET_POSITION 15 06`. Historische HW2-Position 29567 gehört zum alten virtuellen Koordinatensystem.
- `SET_PARAM`: Parameter-ID + drei Rohwertbytes, z. B. `SET_PARAM 01 50 00 00` (momentan NACK).
- `MOVE`: DATA[0] oberes Nibble = Mikroschritt-Modus **0 voll / 1 halb / 2 viertel / 3 achtel / 4 sechzehntel**, Bit 0 = Richtung (0 links, 1 rechts), Bits 3..1 reserviert; DATA[1..2] Schrittzahl LE **1..10000**.
- Nur Bench-`SET_POSITION`: **1..65534** als virtueller Positionsraum; maximal **10600 Viertelschritte je SET_POSITION-Auftrag**; **ab dieser Version erst nach `reference 00`**, keine automatische Position 32767. Andere Mikroschrittauflösung invalidiert die simulierte Referenz.
- Status Motor, DATA[0]: Bit0 Treiber an, Bit1 in Bewegung; DATA[1] **ESP32-Erweiterung** des Nano-Protokolls: `0 IDLE / 1 CONF / 2 REFRUN / 3 MOVING / 4 STOPPED / 5 ERROR`. Kompatibilitätsfreigabe hierzu noch in [#156](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/156).
- Bench-`GET_POSITION`: `0x00010000 | virtuelle_16bit_position`, also DATA[2] = `01`; **kein echter REF-Nachweis**. `POSITION_UNKNOWN` (ohne Bench-Bit) = `0x00007FFF`.
- NACK-Typen: `1 BUSY`, `2 NOT_READY`, `3 INVALID_COMMAND`, `4 INVALID_PARAMETER`, `5 EXECUTION_ERROR`, `6 INVALID_TELEGRAM`. Fehlerort z. B. UART `0x10`, Motor `0x20`.

### Neu: Fake-Kurz-REF und SET_TRACK aus HW_TEST (nur loser Motor)

Gemessene Tabelle aus `FY_HW_TEST/FY_HW_TEST/src/main.cpp`, **unveränderte absolute HW_TEST-Koordinaten im 1/4-Schritt-Testmaßstab**:

| Gleis | BG1 | BG2 | BG3 | BG4 | BG5 |
|---|---:|---:|---:|---:|---:|
| Position | **1557** | **3157** | **4757** | **6357** | **7957** |

`REFERENCE 00` ist im `FY_MOTOR_BENCH_TEST=1` **rein synthetisch**: Es gibt keine Motorfahrt, keine Sensorprüfung und keine echte Referenzfreigabe. Es setzt **1000** als Startposition und `STATUS 30` auf **02=simuliert**. Erst damit erlaubt `Motor::prepareSetTrack()` die Gleisauswahl; sie nutzt unverändert die `prepareSetPosition()`-/Profil-/STEP-Implementierung. Eine Änderung der Auflösung durch freies `MOVE`, STOP oder GO OFF löscht die simulierte Referenz. `REFERENCE 01` ist nicht implementiert; **außerhalb des Bench-Modus sind beide gesperrt**.

```text
stop aa 55            # Boot-STOP entriegeln
go aa 55              # Endstufen freigeben
status 30             # vor Fake-REF: STATUS_Reference 30 00 <CRC>
reference 00          # FAKE Kurzreferenz: Position = 1000, kein STEP!
status 30             # STATUS_Reference 30 02 <CRC>
position              # STATUS_Position: Low16=1000 und Bit16=1 (nur BENCH)
set_track 01          # Ziel BG1=1557, Differenz +557 in 1/4-Schritt
status 20             # poll bis IDLE
track 00              # STATUS_Track 40: Ziel, bestätigt, Status
set_track 05          # Ziel BG5=7957
```

**Am 23.10.2026 auf HW3 mit losem Motor gefahren:** `REFERENCE 00`, `SET_TRACK 01/03/05`, `GET_TRACK` während der Fahrt und nach Ankunft. Simulierte Referenz, **keine** reale FY-Referenzierung. `GO/STOP`-Verriegelung und echte Sensor-/Endlagenüberwachung bleiben separate Aufgaben. Keine ungefragte DONE-UART-Meldung; Abschluss abfragen.

### HW3-Prüfprotokoll vom 23.10.2026

- Lose Motorwelle: A4988 bei 1/4- und 1/16-Mikroschritt gedreht, 1/16 hörbar ruhiger.
- `STOP AA 55` → `60 01 20`; `GO AA 55` → `60 07 32`.
- `REFERENCE 00` → `60 16 45` und `STATUS 30` → `30 02 9E` (simuliert).
- `SET_TRACK 01/03/05` → jeweils `60 1C 73`. Beobachtete `TRACK 00`-Antwort BG1 erreicht: `40 01 01 00 F8`; nach Auftrag BG5: `40 05 00 01 41` (unterwegs), später `40 05 05 00 07` (erreicht).
- Absichtlich falsches Kommando `TRACK` ohne erforderliches Datenbyte → `70 14 3B` (Parameter fehlt); UART-Validierung funktioniert.
- 12-V-Versorgung während Firmware-Upload eingeschaltet: beim beobachteten Versuch **kein Motorzucken**. Ältere Zuck-Beobachtungen werden dadurch nicht widerlegt; Boot-/Flash-Interlock weiter offen.
- Bench-Grenze: keine echte REF, keine LS/Endlagenprüfung, keine unabhängige Messung der absoluten Mechanikposition, keine Hardware-Freigabe für gekoppelte Schiebebühne.

### HW3: STEP/ENABLE-GPIO-Tausch und einmalige SLEEP-Startfreigabe (23.10.2026)

HW3: **!ENABLE G25 (Blau)**, **DIR G26 (Gelb)**, **STEP G27 (Weiß)**, **!SLEEP + !RESET G32 (Rot)**.

**LED-Polaritäten aus tatsächlicher Verdrahtung:** Rot (Anode an G32, Vorwiderstand nach GND) leuchtet bei **G32 HIGH = A4988 wach** und ist bei LOW (Sleep/Reset) AUS. Blau/Gelb/Weiß haben gemeinsames Plus an 3,3 V und leuchten bei **GPIO LOW**.

`setup()` hält G32 anfangs LOW. Nach Motor-/MS-/Timer-Initialisierung setzt `Muninn_initMotorMock()` G32 **einmalig HIGH (Rot AN)**, wartet **1,5 ms** und lässt erst danach die Tasks starten. **Die ISR wurde nicht geändert** und nimmt ihren STEP-GPIO unverändert aus `MOTOR_PWM_PIN`. Beim Start ist die POR-STOP-Sperre aktiv, ENABLE HIGH, Blau AUS. `STOP AA 55` entriegelt nur STOP. `GO AA 55` setzt ENABLE LOW (Blau AN); `GO 55 AA` sowie beide STOP-Varianten deaktivieren ENABLE, ohne G32 nochmals zu verändern.

**HW3 am 23.10.2026 gebaut und betrieben:** G32-SLEEP/RESET-Freigabe sowie GO/STOP und STEP/DIR/ENABLE im Motor-Bench bestätigt. Upload mit anliegender 12-V-Versorgung blieb im beobachteten Versuch ohne Motorzucken. **Kein vollständiger Boot-/Reset-/Flash-Failsafe-Nachweis.** Vor VMOT den G32-SLEEP-/RESET-LOW-Pegel während Boot/Reset prüfen; 10-kΩ-Pull-down gegen den am Modul gemessenen ca. 97-kΩ-Pull-up. Siehe [HW3-Pinout](../docs/pinouts/01_PINOUT_ESP32.md) und [#153](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/153).

### Historische HW2-Sequenz (alter Build mit Startposition 32767 – nicht mehr aktuelle FW)

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
        -> Motor::prepareMove() / prepareSetPosition() / prepareSetTrack()
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
| `prepareSetTrack()` | 🟢 **Bench** / 🟠 **Real** | HW3: `Motor::prepareSetTrack()` löst 1–5 in 1557/3157/4757/6357/7957 auf; Bewegungen und `GET_TRACK`-Rückmeldung getestet. Echte Referenz/Grenzen fehlen; [#77](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/77), [#126](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/126) |
| `prepareReference()` | 🟢 **Fake-Bench** / 🟠 **Real** | `REFERENCE 00` simuliert Kurz-REF=1000; über `STATUS 30` eindeutig als FAKE markiert. Reale LS-Auswertung/`REFERENCE 01` offen; [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155) |
| `prepareParameter()` | 🟠 | SET_PARAM nur Telegramm, keine Übernahme; [#79](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/79) |
| `CheckBorder()` | 🟡 Bench / 🟠 Real ⛔ | Bench-Bounds vorhanden; echte mechanische Grenze fehlt; [#81](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/81) |
| `prepareGo()` / `prepareStop()` | 🟣 | Aufgabe in Muninn statt gleichnamiger Funktionen; HW-Reset/Failsafe offen; [#86](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/86) |
| `setJob()` und `Update()` | 🟣 | Aufgaben auf Queue, Muninn-Motorloop und ISR verteilt; Stress-/Fehlertests offen; [#48](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/48), [#50](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/50) |
| Prepare-NACK-/Fehlersemantik | 🟡 | Grundtypen vorhanden, vollständige Motor-/REF-Fehler offen; [#80](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/80) |

**Historie:** Der alte Nano-UART-Vertrag bleibt separat unter [FY_Controller_Motor/src/UART_Command_Spickzettel.md](../../FY_Controller_Motor/src/UART_Command_Spickzettel.md) und im gesicherten Branch `baseline/Nano-2026-10-19` erhalten. Dies hier ist die **ESP32-Istfassung** und kein Anspruch, dass alle Nano-Befehle portiert seien.

**Vor dem realen FY-Betrieb:** `FY_MOTOR_BENCH_TEST=1` in `platformio.ini` abschalten, aber **nicht** einfach die reale Positionierung freischalten: erst Sensoren, sichere Endlagen, Boot-/OTA-Hardwarefreigabe, Referenzfahrt und Hardwareabnahme implementieren und verifizieren.
