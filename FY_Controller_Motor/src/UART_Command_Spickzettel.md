# UART – Command-Spickzettel

Kurzübersicht für `FY_Controller_Motor/src/main.cpp` und den seriellen Terminalbetrieb.

## Commands

| Command | ID | Daten | ASCII-Eingabe |
|---|---:|---|---|
| GET_STATUS | `0x19` | 1 Byte Selektor (Hex) | `STATUS 10`, `STATUS 20`, `STATUS 40`, `STATUS 50` |
| GET_ERROR | `0x1A` | – | `ERROR` |
| REFERENCE | `0x16` | – | `REFERENCE` |
| SET_PARAM | `0x2B` | Parameter-ID + 24-Bit-Rohwert (4 DATA-Byte) | `SET_PARAM <ID> <Rohwert>` |
| STOPP | `0x01` | – | `STOP` |
| LEFT | `0x0B` | – | `LEFT` |
| RIGHT | `0x0D` | – | `RIGHT` |
| SET_POSITION | `0x13` | uint16 | `SET_POSITION 3200` |
| GET_POSITION | `0x15` | – | `GET_POSITION` |
| SET_TRACK | `0x1C` | uint8 | `SET_TRACK 3` |
| GET_TRACK | `0x23` | 1 Byte Option (`00` = Standardabfrage) | `GET_TRACK` |
| HELP | `0x25` | – | `HELP` |
| GET_FIRMWARE | `0x27` | – | `FIRMWARE` |


| SET_MODE | `0x26` | 2 Byte: `55 AA` = LOCAL, `AA 55` = REMOTE | `SET_MODE LOCAL` / `SET_MODE REMOTE` |

> `CMD_GO` ist bewusst nicht aufgeführt. Bewegungsbefehle sollen direkt ausgeführt werden.

## Status-Selektoren

| Selektor | Abfrage | Antwort-ID |
|---:|---|---:|
| `10` | System / Initialisierung | `0x10` |
| `20` | Motor (Bit 0 Treiber aktiv, Bit 1 Bewegung) | `0x50` |
| `40` | Digitale Eingänge REF, TRIM_LEFT, TRIM_RIGHT, TIMING_BELT in Bit 0–3 | `0x41` |
| `50` | Tastatur verbunden (Bit 0) | `0x51` |

Selektor als Hex-Text ohne `0x`; BabelFish berechnet die CRC automatisch.

## Datenformat

- `uint8`: 1 Byte
- `uint16`: 2 Byte, **Little Endian**
- Beispiel: `SET_POSITION 3200` → `0x13 0x80 0x0C`
- Beispiel: `SET_TRACK 3` → `0x1C 0x03`
- Beispiel: `SET_PARAM 1 80` → `0x2B 0x01 0x50 0x00 0x00` (Beispiel-ID; Rohwert 80, Semantik gemäß Issue #145)

## Responses

| Response | ID | Daten |
|---|---:|---|
| Alive | `0x03` | – |
| Error | `0xF0` | 4 Byte |
| System | `0x10` | 2 Byte |
| CMD | `0x15` | 1 Byte |
| Position | `0x20` | 4 Byte |
| Reference | `0x30` | 1 Byte |
| Track | `0x40` | 3 Byte: Zielgleis, zuletzt bestätigtes Gleis, Bewegungsstatus |
| Motor | `0x50` | 3 Byte |
| ACK | `0x60` | 1 Byte: bestätigte Command-ID |
| NACK | `0x70` | 1 Byte: abgelehnte Command-ID |
| DEV_RESPONSE | `0xFF` | 2 Byte: Modulkennung, Entwicklungsfehlercode (unerwarteter Implementierungszustand) |
| Help | `0x18` | Hilfetext auf Norwegisch + GitHub-Spickzettel-Link (variabel) |
| Firmware | `0x28` | Automatisch generierte Firmware-Identität: Name, Version, Build-Zeit, Git-Commit |

## HELP und GET_FIRMWARE

- `HELP` liefert `STATUS_Help (0x18)` mit einem kurzen norwegischen Gimmick und dem Link zu diesem Spickzettel.
- `FIRMWARE` liefert `STATUS_Firmware (0x28)` mit denselben automatisch erzeugten Angaben wie die Firmware-Ausgabe beim Start: Name, Version, Build-Datum/-Zeit und Git-Commit.

## GET_TRACK – Antwortformat (Issue #58)

Request DATA:
- `DATA[0] = 0x00`: Standardabfrage (einzige aktuell gültige Option)

Response `STATUS_Track (0x40)`:
- `DATA[0]`: Zielgleis BG1–BG5
- `DATA[1]`: zuletzt bestätigtes aktuelles Gleis BG1–BG5
- `DATA[2]`: Bewegungsstatus: `0x00` = REACHED, `0x01` = MOVING, `0x02` = ERROR

Während der Bewegung bleibt das aktuelle Gleis das zuletzt bestätigte Gleis. Die Gleiszustandsführung selbst muss vom Motor-/Sensorpfad aktualisiert werden; GET_TRACK liest diesen Zustand nur aus.

## Terminal-Test

Die Eingabe im Terminal erfolgt als lesbarer ASCII-Command:

\`\`\`text
STATUS
GET_POSITION
SET_TRACK 3
SET_POSITION 3200
SET_PARAM 1 80
SET_MODE LOCAL
SET_MODE REMOTE
STOP
FIRMWARE
\`\`\`

BabelFish wandelt diese Eingaben in die binären Commands um.


## SET_MODE – Verweis auf Issue #58

`CMD_SET_MODE` ist ein Execute-Command. Die bisher getrennten `CMD_SET_LOCAL` und `CMD_SET_REMOTE` entfallen. Da Issue #58 die gemeinsame Command-ID nicht separat nennt, wird `0x26` (die bisherige SET_LOCAL-ID) als gemeinsame `CMD_SET_MODE`-ID weiterverwendet; `0x2A` entfällt.

- DATA-Länge: 2 Byte
- `55 AA`: LOCAL
- `AA 55`: REMOTE
- Betriebsartenwechsel nur im Stillstand
- Antwort: ACK (`0x60`) mit Command-ID oder NACK (`0x70`) mit Command-ID
- Bei ungültigem Datenmuster, fehlendem erforderlichem Modul oder laufender Bewegung erfolgt NACK; die Betriebsart bleibt unverändert.
- Der Befehl schaltet den Motor nicht ein und startet keine Bewegung.

Siehe [Issue #58](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/58).


## DEV_RESPONSE – Entwicklungsfehler

`DEV_RESPONSE` (`0xFF`) ist für Zustände reserviert, die im normalen Betrieb nicht auftreten dürfen, aber erst später durch einen bislang nicht durchlaufenen Codepfad entdeckt werden könnten.

- DATA[0]: Modulkennung; aktuell `0x01` = UART
- DATA[1]: Fehlercode; aktuell `0x01` = NULL_POINTER
- Beispiel: `FF 01 01` = UART meldet einen unerwarteten Nullpointer-Zustand (CRC folgt gemäß Telegrammformat).
- Die Antwortdefinition bleibt regulär in `Protokoll.h` enthalten. Sie ist keine normale Betriebsantwort.
- Bei einem solchen Fehler wird der betroffene Handler kontrolliert abgebrochen; es dürfen keine erfundenen oder ungültigen Nutzdaten gesendet werden.
- Weitere Modul- und Fehlercodes werden bei Bedarf ergänzt.

Siehe [Issue #58](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/58).
