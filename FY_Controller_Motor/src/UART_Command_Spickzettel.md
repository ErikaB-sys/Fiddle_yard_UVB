# UART – Command-Spickzettel

Kurzübersicht für `FY_Controller_Motor/src/main.cpp` und den seriellen Terminalbetrieb.

## Commands

| Command | ID | Daten | ASCII-Eingabe |
|---|---:|---|---|
| GET_STATUS | `0x19` | 1 Byte Selektor (Hex) | `STATUS 10`, `STATUS 20`, `STATUS 40`, `STATUS 50` |
| GET_ERROR | `0x1A` | – | `ERROR` |
| REFERENCE | `0x16` | – | `REFERENCE` |
| SET_PARAM | `0x2B` | Parameter-ID + 24-Bit-Rohwert (4 DATA-Byte) | `SET_PARAM ID B0 B1 B2` (ESP32: Hex-Bytes) |
| GET_PARAM | `0x2C` | 1 Byte: Parameter-ID | `GET_PARAM ID` (ESP32-Erweiterung, Handler folgt) |
| STOPP | `0x01` | 2 Byte: `55 AA` = STOPP aktivieren, `AA 55` = STOPP aufheben | `STOPP 55AA` / `STOPP AA55` (auch `STOP` als Alias) |
| LEFT | `0x0B` | – | `LEFT` |
| RIGHT | `0x0D` | – | `RIGHT` |
| SET_POSITION | `0x13` | uint16 | `SET_POSITION 3200` |
| GET_POSITION | `0x15` | – | `GET_POSITION` |
| SET_TRACK | `0x1C` | uint8 | `SET_TRACK 3` |
| GET_TRACK | `0x23` | 1 Byte Option (`00` = Standardabfrage) | `GET_TRACK` |
| HELP | `0x25` | – | `HELP` |
| GET_FIRMWARE | `0x27` | – | `FIRMWARE` |
| SET_MODE | `0x26` | 2 Byte: `55 AA` = LOCAL, `AA 55` = REMOTE | `SET_MODE LOCAL` / `SET_MODE REMOTE` |
| GO | `0x07` | 2 Byte: `55 AA` = Treiber AUS, `AA 55` = Treiber EIN | `GO 55AA` / `GO AA55` |

> `GO` schaltet nur den Motortreiber ein oder aus; es startet keine Bewegung. Die STOPP-Sperre hat Vorrang vor `GO AA55`.

## Parameter-GET/SET und SAVE – ESP32-Protokollvertrag (#158)

> **Implementierungsstand:** IDs, Telegrammlaengen und SAVE-Muster stehen jetzt in
> `FY_ESP32_Bringup/include/Protokoll.h`. Die eigentlichen GET-/SET-/SAVE-Handler
> und die NVM-/Motoranbindung folgen. Der alte Nano-Spickzettel bleibt fuer
> existierende Nano-Befehle bestehen; neue Parameterbefehle noch nicht am Motor testen.

- `GET_PARAM ID`: ein Byte `ID`; Antwort `STATUS_Parameter (0x2D)` mit
  `[ID, V0, V1, V2]` (vier DATA-Byte), **derselbe ID-Raum** wie SET.
- `SET_PARAM ID V0 V1 V2`: vier DATA-Byte; unsigned 24-bit LE
  (`Wert = V0 + 256*V1 + 65536*V2`), nur im Stillstand, zuerst nur RAM.
- `SET_PARAM 7F BEREICH AA 55`: expliziter SAVE-Auftrag fuer **genau ein**
  Segment. Bereich `01` = Gleispositionen, `02` = Motorparameter.
  Kalibrierung wird ausschliesslich nach der Messfahrt geschrieben.
  Das Muster ist exakt zu pruefen; andere Werte
  muessen mit NACK abgelehnt werden. SAVE nur im Stillstand; waehrend SAVE
  keine zweite FY-Aktion zulaessig. STOP darf einen bereits begonnenen
  NVS-Commit nicht abbrechen oder dessen Daten beschaedigen; eingehender
  STOP ist nach dem Commit vorrangig abzuarbeiten. Kalibrierungswerte werden **nicht**
  durch SET oder SAVE erzeugt.
- Kalibrierungs-GETs sind read-only. Gleispositionen sind bis zur Teach-in-
  Funktion per SET veraenderbar; Motorparameter werden beim SET validiert.
- Ein SAVE ohne geaenderte Daten des gewaehlten Segments schreibt nichts und entwertet die aktuelle
  Referenz nicht. Ein erfolgreicher SAVE mit tatsaechlicher Parameter-
  Aenderung entwertet die aktuelle Referenz (nicht die Kalibrierdaten/CRC).
- A/B-Profilwerte sind **unsigned**: die Kennzeichnung als A (Beschleunigung)
  beziehungsweise B (Bremsung) bestimmt die Richtung der Timer-Aenderung;
  negative Zahlen gehoeren nicht auf die UART-Leitung.

| ID (hex) | Name | GET | SET | Inhalt |
|---|---|---|---|---|
| `01` | V_MIN | ja | ja | Minimale Fahrgeschwindigkeit |
| `02` | V_MAX | ja | ja | Maximale Fahrgeschwindigkeit |
| `03` | V_POS | ja | ja | Positioniergeschwindigkeit; erst mit angebundenem Verhalten freigeben |
| `04` | V_REF_MIN | ja | ja | Langsame Referenzgeschwindigkeit |
| `05` | V_REF | ja | ja | Referenzgeschwindigkeit |
| `06`–`09` | A1, A2, B1, B2 | ja | ja | Positive Beschleunigungs-/Bremsparameter |
| `0A`–`0D` | A1_ST, A2_ST, B1_ST, B2_ST | ja | ja | Rampenintervalle in Schritten |
| `20` | POS_LSR | ja | **nein** | Gemessene Gegenendlage |
| `21` | POS_REF_A | ja | **nein** | Gemessene Fahnenflanke A |
| `22` | POS_REF_B | ja | **nein** | Gemessene Fahnenflanke B |
| `30`–`34` | TRACK_BG1 bis TRACK_BG5 | ja | ja | Eingelernte Gleispositionen |
| `7E` | RESTORE_DEFAULT | **nein** | ja (Muster) | `7E 01 AA 55` = Gleise; `7E 02 AA 55` = Motor; geplant |
| `7F` | SAVE | **nein** | ja (Muster) | `7F 01 AA 55` = Gleise; `7F 02 AA 55` = Motor |

**Beispiel-Bytes (ESP32, noch nicht lauffaehig):**

```text
GET_PARAM 30          ; BG1 abfragen
SET_PARAM 30 15 06 00 ; BG1 = 0x000615 = 1557 (nur RAM)
SET_PARAM 7F 01 AA 55 ; nur geaenderte Gleispositionen ins NVS
SET_PARAM 7F 02 AA 55 ; nur geaenderte Motorparameter ins NVS
GET_STATUS 60         ; NVM: Fehlerflag, Fehlerbits, gueltige Segmente
```

Bei neuem ESP32: fehlende Motor- und Gleissegmente mit gueltiger CRC aus
Defaultwerten initialisieren. **Ungueltige vorhandene Daten nicht
automatisch neu schreiben.** Kalibrierung ausschliesslich durch
erfolgreiche lange Referenzfahrt; keine Kalibrierdefaults. Alle Segment-CRC
sind unabhaengig.

## NVM-Diagnose und Wiederherstellung (Issue #158)

- `GET_STATUS 60` meldet `STATUS_NVM (0x61)` mit 3 DATA-Byte: `[ErrorFlag, FaultMask, ValidMask]`.
- Bits der beiden Masken: Bit 0 = Kalibrierung, Bit 1 = Gleise, Bit 2 = Motorparameter; Bits 3–7 für spätere Datensätze.
- ErrorFlag: `00` = kein NVM-Fehler, `01` = mindestens ein korruptes/ungeprüftes Segment.
- `NOT_FOUND` bei Kalibrierung ist auf frischem ESP32 kein NVM-Fehler; das Gültigkeitsbit ist aber 0 (lange REF erforderlich).
- CRC-/Formatfehler an erforderlichen Segmenten und ungültige Motorparameter führen bei Bewegungsaufträgen zu NACK. Kein automatischer Default-Reset bei vorhandenen korrupten Daten.
- Geplanter Wartungsbefehl `SET_PARAM 7E 01 AA 55` (Tracks) oder `SET_PARAM 7E 02 AA 55` (Motor): setzt das **gewählte** Segment explizit auf Defaultwerte zurück und schreibt eine neue gültige CRC, auch bei beschädigtem Vorwert. Für die Kalibrierung **nicht erlaubt**; sie wird nur durch Messung neu gültig.
- `SAVE` und `RESTORE` arbeiten ausschließlich im Stillstand. STOP darf einen bereits begonnenen NVS-Commit nicht unterbrechen oder beschädigen; anschließend wird STOP prioritär verarbeitet.
- **Implementierungsstatus:** Definitionen im Protokoll, Handler/Recovery noch nicht abgenommen; siehe [NVM-Spezifikation](../../FY_ESP32_Bringup/docs/nvm/README.md).

## Status-Selektoren

| Selektor | Abfrage | Antwort-ID |
|---:|---|---:|
| `10` | System / Initialisierung | `0x10` |
| `20` | Motor (Bit 0 Treiber aktiv, Bit 1 Bewegung) | `0x50` |
| `40` | Digitale Eingänge REF, TRIM_LEFT, TRIM_RIGHT, TIMING_BELT in Bit 0–3 | `0x41` |
| `50` | Tastatur verbunden (Bit 0) | `0x51` |
| `60` | NVM-Fehler, Segmentmaske und Gültigkeitsmaske | `0x61` |

Selektor als Hex-Text ohne `0x`; BabelFish berechnet die CRC automatisch.

## Datenformat

- `uint8`: 1 Byte
- `uint16`: 2 Byte, **Little Endian**
- Beispiel: `SET_POSITION 3200` → `0x13 0x80 0x0C`
- Beispiel: `SET_TRACK 3` → `0x1C 0x03`
- Beispiel: `GO AA55` → Command `0x07`, DATA `AA 55`; auf der seriellen Diagnose erscheint `LEN=3` (Command plus zwei DATA-Byte).
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
| Parameter | `0x2D` | 4 Byte: Parameter-ID, Wertbyte 0, 1, 2 (Little Endian); Handler folgt |
| NVM | `0x61` | 3 Byte: Fehlerflag, Fehlersegment-Bitmaske, Gültigkeits-Bitmaske |
| ACK | `0x60` | 1 Byte: bestätigte Command-ID |
| NACK | `0x70` | 1 Byte: Fehlerort/Modul (oberes Nibble) + Ablehnungsgrund (unteres Nibble) |
| DEV_RESPONSE | `0xFF` | 2 Byte: Modulkennung, Entwicklungsfehlercode (unerwarteter Implementierungszustand) |
| Help | `0x18` | Hilfetext auf Norwegisch + GitHub-Spickzettel-Link (variabel) |
| Firmware | `0x28` | Automatisch generierte Firmware-Identität: Name, Version, Build-Zeit, Git-Commit |

## NACK – Codierung

`STATUS_NACK (0x70)` enthält genau ein DATA-Byte:

- **DATA[0], Bits 7..4:** Fehlerort bzw. zuständiges Modul; dafür werden die vorhandenen Fehlerort-/Modulkonstanten aus `Error.h` wiederverwendet.
- **DATA[0], Bits 3..0:** Ablehnungsgrund.

Die Command-ID wird **nicht** in der NACK-DATA übertragen. Sie ergibt sich aus dem vorausgegangenen Request. Damit unterscheiden sich ACK und NACK bewusst: ACK enthält die bestätigte Command-ID, NACK den Fehlerort und Ablehnungsgrund.

Die zentralen Werte sind in `Protokoll.h` als `NackReason` definiert:

| Wert | Name | Bedeutung |
|---:|---|---|
| `0x0` | `NONE` | reserviert; nicht für tatsächliche NACKs verwenden |
| `0x1` | `BUSY` | Controller/Ressource ist beschäftigt |
| `0x2` | `NOT_READY` | erforderliche Voraussetzung oder Modul ist nicht bereit |
| `0x3` | `INVALID_COMMAND` | unbekannte oder nicht unterstützte Command-ID |
| `0x4` | `INVALID_PARAMETER` | ungültige DATA-Länge, ungültiger Selektor oder Parameterwert |
| `0x5` | `EXECUTION_ERROR` | unmittelbarer Ausführungsversuch ist fehlgeschlagen |
| `0x6` | `INVALID_TELEGRAM` | Telegramm ist syntaktisch ungültig oder CRC fehlerhaft |
| `0x7..0xF` | reserviert | für spätere Erweiterungen |

Der UART setzt das obere Nibble aus der vorhandenen Fehlerortkonstante und das untere Nibble aus `NackReason` zusammen. Die NACK-DATA wird separat durch `encodeNackData()` codiert. Der jeweilige Aufrufer stellt sie anschließend mit `setResponse()` in den normalen Response-Puffer; übertragen wird sie ausschließlich über `sendResponse()`. Die Codierung selbst stellt keine Response ein.

NACK ist für eine unmittelbare, erwartbare Ablehnung eines Requests vorgesehen. Unerwartete interne Implementierungszustände werden weiterhin über `DEV_RESPONSE` gemeldet; Fehler, die erst während einer bereits gestarteten asynchronen Bewegung auftreten, sind keine nachträgliche NACK-Antwort.

## GO und STOPP – Motortreibersteuerung

Beide Befehle sind Execute-Commands mit zwei DATA-Byte. Die Hex-Werte werden ohne `0x` eingegeben; BabelFish prüft die Eingabe auf vier Hex-Zeichen, wandelt sie in zwei Bytes um und berechnet die CRC. Ob das Datenmuster für den Befehl zulässig ist, entscheidet der Motorhandler.

| ASCII-Eingabe | Erwartete Wirkung | Erwartete Antwort |
|---|---|---|
| `GO 55AA` | Motortreiber ausschalten; laufende Bewegung wird abgebrochen und der Positionszustand invalidiert | ACK `0x60`, wenn angenommen |
| `GO AA55` | Motortreiber einschalten, aber keine Bewegung starten | ACK `0x60`, sofern STOPP nicht aktiv ist; andernfalls NACK `NOT_READY` |
| `STOPP 55AA` | STOPP-Sperre setzen und Treiber sofort ausschalten | ACK `0x60`, wenn angenommen |
| `STOPP AA55` | STOPP-Sperre aufheben; Treiber bleibt ausgeschaltet | ACK `0x60`, wenn angenommen |
| `GO 1234` / `STOPP 1234` | Ungültiges Datenmuster | NACK `INVALID_PARAMETER` |

**Wichtig:** Nach `STOPP 55AA` wird `GO AA55` abgewiesen. Nach `STOPP AA55` läuft der Motor nicht automatisch wieder an; erst ein neuer `GO AA55` schaltet den Treiber ein. `GO` schaltet den Treiber, startet aber selbst keine Fahrt und stellt keine Referenz wieder her.

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
STATUS 10
GET_POSITION
SET_TRACK 3
SET_POSITION 3200
SET_PARAM 1 80
SET_MODE LOCAL
SET_MODE REMOTE
GO 55AA
GO AA55
STOPP 55AA
GO AA55
STOPP AA55
GO AA55
FIRMWARE
\`\`\`

BabelFish wandelt diese Eingaben in die binären Commands um.


## SET_MODE – Verweis auf Issue #58

`CMD_SET_MODE` ist ein Execute-Command. Die bisher getrennten `CMD_SET_LOCAL` und `CMD_SET_REMOTE` entfallen. Da Issue #58 die gemeinsame Command-ID nicht separat nennt, wird `0x26` (die bisherige SET_LOCAL-ID) als gemeinsame `CMD_SET_MODE`-ID weiterverwendet; `0x2A` entfällt.

- DATA-Länge: 2 Byte
- `55 AA`: LOCAL
- `AA 55`: REMOTE
- Betriebsartenwechsel nur im Stillstand
- Antwort: ACK (`0x60`) mit Command-ID oder NACK (`0x70`) mit NACK-Code (oberes Nibble: Fehlerort/Modul, unteres Nibble: Ablehnungsgrund)
- Bei ungültigem Datenmuster, fehlendem erforderlichem Modul oder laufender Bewegung erfolgt NACK; die Betriebsart bleibt unverändert.
- Der Befehl schaltet den Motor nicht ein und startet keine Bewegung.

Siehe [Issue #58](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/58).


## DEV_RESPONSE – Entwicklungsfehler

`DEV_RESPONSE` (`0xFF`) ist für Zustände reserviert, die im normalen Betrieb nicht auftreten dürfen, aber erst später durch einen bislang nicht durchlaufenen Codepfad entdeckt werden könnten.

- DATA[0]: oberes Nibble = Fehlerort/Modul, unteres Nibble = Fehlercode; aktuell `0x10 | 0x01 = 0x11` für UART / NULL_POINTER.
- DATA[1]: Debug-Reserve; im Release-Code immer `0x00`. Lokale Diagnosewerte dürfen hier nur temporär während der Entwicklung eingetragen und müssen vor einem Release zurückgesetzt werden.
- Beispiel: `FF 11 00` = UART meldet einen unerwarteten Nullpointer-Zustand; CRC folgt gemäß Telegrammformat.
- Modulkennung, Fehlercode und Release-Standardwert sind zentral in `Protokoll.h` definiert.
- Die Antwort ist keine normale Betriebsantwort. Bei einem solchen Fehler wird der betroffene Handler kontrolliert abgebrochen; es dürfen keine erfundenen oder ungültigen Nutzdaten gesendet werden.
- Weitere Modul- und Fehlercodes werden bei Bedarf ergänzt.

Siehe [Issue #58](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/58).
