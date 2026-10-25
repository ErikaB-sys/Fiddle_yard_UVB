# FY ESP32 – UART-Spickzettel und Umsetzungsgrad

**Stand: 24.10.2026.** Codebasis `342cdd35`, Branch `feature/FY_ESP32_Port/FY_ESP32_Bringup`. Dokumentationsupdate nach Issue-Bereinigung und Klarstellung der Fahrregeln; **keine C++-Änderung**.

**Grün heißt: Softwarefunktion im genannten Umfang geprüft und dokumentiert.** Die Tisch-Funktionsnachweise zählen. Echte Kalibrierung, Sensorik und Bühnenintegration sind separate Restpunkte in [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155)/[#159](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/159).

[Kurze Roadmap](../../Documents/FY_Productivsoftware_Roadmap.md) · [#150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150) Überblick · [#154](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/154) Fahrvertrag · [#156](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/156) UART · [Motorzustände](../docs/Motor/README.md)

## Ampel

- 🟢 geprüft und dokumentiert im beschriebenen Funktionsumfang.
- 🟡 Teilfunktion vorhanden, konkreter funktionaler Rest offen.
- 🟠 beschlossen/geplant, noch kein ausführbarer Funktionspfad.
- 🟣 benannter Prüf-/Messnachweis fehlt; kein pauschales Zurückstufen fertiger Funktionen.
- ⚪ Legacy-/Option: derzeit nicht aktiv; kein zusätzlicher Pflichtumfang ohne Entscheidung.

## Verbindliche Fahrregeln

| Fall | Vereinbartes Verhalten | Heute |
| --- | --- | --- |
| MOVE ohne REF | Relativ eine begrenzte Schrittzahl in eine Richtung; für Inbetriebnahme zulässig. | 🟢 Vorhandener begrenzter MOVE-Pfad. |
| SET_POSITION ohne REF | Nicht zulässig; POR-Mitten-/Einzelfahrtvariante entfällt. | Kein zusätzlicher Implementierungsauftrag. |
| SET_POSITION mit gültiger REF | Fahrt nur im kalibrierten Bewegungsraum. | 🟠 Reale REF/Grenzen fehlen. Vorhandener synthetischer Positionspfad funktioniert. |
| Kurze REF | Erst mit gültigen Kalibrierdaten erfolgreich. | 🟠 Heute nur FAKE-REF 00, kein realer Ablauf. |
| SET_TRACK | Gleis 1..5; kalibriert **und** aktuelle REF gültig. | 🟢 Track→Position/Fahrt geprüft; 🟠 reale Kalibrier-/Referenzanbindung offen. |
| STOP / GO | STOP verwirft Job, Antrieb aus, REF ungültig. Entriegeln/GO aktiviert nur den Antrieb; danach für normale Fahrt neu mindestens kurz referenzieren. | Grundpfad vorhanden; gezielte Prüfung des STOP-/Antworttransports [#152](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/152) schließen. |

Kalibrierwerte bleiben nach STOP erhalten; aktuelle Referenzgültigkeit und Positionszähler sind davon getrennt. Ein gültiger neuer Fahrbefehl startet nach Prepare OK unmittelbar, kein GO je Fahrjob. Kein Pause/Resume.

## Serielle Befehle – tatsächlicher Iststand

115200 Baud, derzeit ASCII über BabelFish mit binären Antworten. Jedes DATA-Byte als **zwei Hex-Zeichen**, getrennt durch Leerzeichen; uint16 Little Endian. `STOP`/`STOPP`, `POSITION`/`GET_POSITION` sind gültige Namen. Keine unbeauftragten DONE-/ALIVE-Telegramme.

| Command | ID | Eingabe / DATA | Stand | Funktionsumfang / konkreter Rest |
| --- | ---: | --- | --- | --- |
| STOP / STOPP | 01 | `STOP 55 AA`; `STOP AA 55` entriegeln | 🟡 | Abschalten/Verriegeln/Verwerfen vorhanden und Grundweg geprüft. Eigene Priority-Queue vorhanden. Frühere Pending-voll-Fehlerbehauptung präzisiert: im gültigen aktuellen UART-Pfad nicht erreichbar; Antworttransport bei Stau gezielt beurteilen: [#152](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/152). |
| GO | 07 | `GO AA 55` EIN; `GO 55 AA` AUS | 🟢 | Treiberfreigabe ohne automatisches Losfahren geprüft. Kein Wiederaufnehmen eines alten Jobs. |
| MOVE | 0F | `MOVE 21 32 00` = 50 Viertelschritte rechts | 🟢 | Einziger relativer Fahrbefehl, mit Richtung/Mikroschritt/Schritten. Profilfahrt geprüft; 1..10000 je aktuellem Testauftrag. |
| SET_POSITION – vorhandener Positionspfad | 13 | nach FAKE-REF: `SET_POSITION 15 06` = 1557 | 🟢 | Prepare-/Profil-/STEP-Funktion geprüft, auch über SET_TRACK genutzt. Neue Koordinaten benötigen keinen zweiten identischen Funktionsabschluss. |
| GET_POSITION / POSITION | 15 | `POSITION` | 🟢 | Antwort 20 + 4 LE-Bytes + CRC; laufender Muninn-Snapshot. Bit16 kennzeichnet synthetische Position. |
| GET_STATUS Motor | 19 | `STATUS 20` | 🟢 | Antwort 50, ENABLE/MOVING plus MotorState; IDLE nach Fahrt dokumentiert. |
| GET_STATUS Referenz | 19 | `STATUS 30` | 🟢 | Antwort 30: 00 ungültig, 01 echt, 02 FAKE. FAKE-Wert dokumentiert. |
| GET_STATUS System | 19 | `STATUS 10` | 🟡 | Antwort 10 noch fester Wert 04/INIT; tatsächliche Init-/Fehlerflags anschließen. |
| GET_STATUS Sensoren | 19 | `STATUS 40` | 🟡 **HW3-Test offen** | Muninn liest G36/39/34 (LSL/LSR/LSREF), initial einmal und danach nur bei Pegeländerung. Eigene 1-Slot-Overwrite-Queue; Huginn antwortet `41 <Snapshot> <CRC>` ohne selbst GPIOs zu lesen. G35/LS4 reserviert, unbeschaltet. |
| GET_STATUS Keyboard | 19 | `STATUS 50` | 🟠 | Antwort 51 mit 0=nicht verbunden; I²C-Keyboard-Code fehlt. |
| GET_ERROR / ERROR | 1A | `ERROR` | 🟡 | Antwort F0, Systembyte + drei Fehlerbytes; UART-Fehler vorhanden, zentrale Motor-/Sensoraggregation fehlt. |
| GET_TRACK / TRACK | 23 | `TRACK 00` | 🟢 Ist-Abfrage | Ziel/fährt/erreicht geprüft. Während Fahrt setzt Code bestätigtes Gleis auf 0; Bedeutung gegenüber alter letzter Bestätigung noch in [#156](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/156) klären. |
| SET_TRACK | 1C | `SET_TRACK 03` | 🟢 Zuordnung/Fahrt | 1..5 auf Positionen abgebildet; Fahrten 01/03/05 dokumentiert. Reale Kalibriert-/REF-Prüfung noch ergänzen. |
| REFERENCE – Testanker | 16 | `REFERENCE 00` | 🟢 Testpfad | Setzt synthetische Position 1000 ohne Bewegung; STATUS 30=02. |
| REFERENCE – real kurz/lang | 16 | `REFERENCE 00` / `REFERENCE 01` | 🟠 | Echter Ablauf/Sensorik und Kalibriervoraussetzung fehlen. |
| SET_PARAM | 2B | 4 DATA-Bytes: ID + 24-Bit-LE-Wert | 🟠 | Telegramm vorhanden, keine Parameterübernahme/SAVE; IDs und erster Umfang festlegen. |
| SET_MODE | 26 | `SET_MODE 55 AA` / `SET_MODE AA 55` | 🟠 | Muster geprüft, derzeit NOT_READY. Bedienhoheit/Stillstandsumschaltung noch umsetzen. |
| HELP | 25 | `HELP` | 🟡 | Stringantwort 18 vorhanden; Link zeigt noch auf Nano-Spickzettel. |
| GET_FIRMWARE / FIRMWARE | 27 | `FIRMWARE` | 🟡 | Stringantwort 28 vorhanden; Git-Hash fällt noch auf unknown zurück. |

**Entfallen:** separate LEFT-/RIGHT-Kommandos. MOVE mit Richtungsangabe übernimmt die relative Fahrt; die historischen IDs 0B/0D werden im heutigen Code abgewiesen. [Konzept-Einstieg](../docs/README.md).

### Antworten und offene Nachweise

- 🟢 ACK `60 <Command-ID> <CRC>` bestätigt Annahme; NACK `70 <Ort/Grund> <CRC>` ist unmittelbare Ablehnung. Ort im hohen, NackReason im niedrigen Nibble.
- 🟢 Abschluss wird mit STATUS 20 bis IDLE abgefragt; keine spontane DONE-Sendung.
- 🟡 Queue-/Pending-/Event-Robustheit: gezielte Prüfung [#152](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/152). Kein nachgewiesener STOP-Ausfall im normalen UART-Ablauf; Pending wird bereits mit ACK/NACK frei. Stau-/Mehrquellenfälle gesondert beurteilen.
- 🟣 Benannte STEP-/Rampen-/Lastmessungen [#153](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/153); reale Bühnenintegration [#159](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/159) getrennt von erledigter Logik.
- Der aktuelle serielle **Eingang ist ASCII/BabelFish**; erzeugte/geprüfte CRC innerhalb dieser Kette belegt keinen vollständigen binären RX-/Leitungsfehler-Test. Empfangsumfang [#156](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/156) klären.

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

**Verbindliche Richtungskonvention (HW_TEST/HW3): LINKS = Motorseite (LSL/G36, zuvor Nano D12); RECHTS = gegenüberliegende Seite (LSR/G39, zuvor Nano D9).** LSREF/G34 (Nano D11) ist die Referenzlichtschranke; LS4/G35 bleibt frei. Die HW3-Sensorprüfung vom 25.10.2026 ergab die erwarteten Rohbits `01`/`02`/`04`. Keine Verdrahtungsänderung erforderlich. Links/rechts bezieht sich auf die Mechanik, **nicht auf die Blickrichtung**.

### HW3-Sensor-Snapshot (im Code, Hardwaretest offen)

`status 40` fragt Huginns zuletzt empfangenen Sensorstatus ab. Muninn sendet beim ersten Einlesen **einmal den vollständigen Rohzustand**, danach ausschließlich bei Änderung. Transport: **eigene Queue der Länge 1** mit `xQueueOverwrite()`, unabhängig von Motorantworten.

Antwort `STATUS_Switches = 0x41` mit **einem Datenbyte**: Bit 0 LSL/G36, Bit 1 LSR/G39, Bit 2 LSREF/G34, Bit 3 LS4/G35 (**noch nicht angeschlossen, deshalb 0**). Bits 4..7: Änderungszähler **0..E**, `F` für zukünftige Fehler reserviert. Bits zeigen Rohpegel (HIGH=1); elektrische Aktiv-Polarität an HW3 prüfen. Ohne Initial-Snapshot gibt `status 40` `NOT_READY` zurück, **nicht** fälschlich alle Sensoren frei.

**KISS-Hardwaretest:** `status 40` zuerst im Ruhezustand und anschließend nach einzelnem Auslösen von LSL, LSR und LSREF abfragen. Jede Änderung muss das jeweilige Bit und den Zähler beeinflussen. **Noch keine Fahrfreigabe, Endlagenabschaltung oder reale Referenzfahrt:** Muninns lokale Sicherheitseingriffe werden separat implementiert und getestet.

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

## Prepare / interner Pfad

Huginn validiert → typisierte Queue → Muninn prüft Betriebsbedingungen/Prepare → Profil → STEP-Timer → angefragter Snapshot. Kein Pflichtnachbau von Nano-`setJob()`/`Update()`.

| Funktion | Stand | Rest |
| --- | --- | --- |
| calcProfile / prepareMove | 🟢 geprüfter Funktionsumfang | Konkrete Timer-/Lastnachweise [#153](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/153). |
| prepareSetPosition | 🟢 vorhandener synthetischer Pfad | 🟠 Reale Kalibrier-/REF-Anbindung und Raumgrenzen; kein unreferenzierter Absolutpfad [#154](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/154)/[#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155). |
| prepareSetTrack | 🟢 Track→Position/Fahrt | 🟠 Kalibriert-/REF-Gate anschließen [#154](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/154)/[#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155). |
| prepareReference | 🟢 Testanker / 🟠 real | Kalibrierfahrt und kurze REF aus HW_TEST [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155). |
| Parameter / SAVE | 🟠 | Übernahme und Persistenz [#158](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/158). |
| Grenzen / Fehler | 🟡 Testbereich / 🟠 real | Fahrraum/Sensoren [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155), Fehlertransport [#152](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/152)/[#158](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/158). |

Die alten Nano-Detailissues sind am 24.10. abgeschlossen oder in die aktuellen Fachissues übernommen worden. Ihre Zustände bedeuten nicht, dass alle dort ursprünglich beschriebenen Nano-Tests bestanden seien. Die Zuordnung und verbleibenden Fragen stehen in [#150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150).
