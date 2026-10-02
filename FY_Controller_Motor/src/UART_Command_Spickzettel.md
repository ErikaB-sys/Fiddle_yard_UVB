# UART – Command-Spickzettel

Kurzübersicht für `FY_Controller_Motor/src/main.cpp` und den seriellen Terminalbetrieb.

## Commands

| Command | ID | Daten | ASCII-Eingabe |
|---|---:|---|---|
| GET_STATUS | `0x19` | – | `STATUS` |
| GET_ERROR | `0x1A` | – | `ERROR` |
| REFERENCE | `0x16` | – | `REFERENCE` |
| SET_SPEED | `0x2B` | uint16 | `SET_SPEED 800` |
| STOPP | `0x01` | – | `STOP` |
| LEFT | `0x0B` | – | `LEFT` |
| RIGHT | `0x0D` | – | `RIGHT` |
| SET_POSITION | `0x13` | uint16 | `SET_POSITION 3200` |
| GET_POSITION | `0x15` | – | `GET_POSITION` |
| SET_TRACK | `0x1C` | uint8 | `SET_TRACK 3` |
| GET_TRACK | `0x23` | – | `GET_TRACK` |
| HELP | `0x25` | – | `HELP` |
| GET_FIRMWARE | `0x27` | – | `FIRMWARE` |
| SET_LOCAL | `0x26` | – | – |
| SET_REMOTE | `0x2A` | – | – |

> `CMD_GO` ist bewusst nicht aufgeführt. Bewegungsbefehle sollen direkt ausgeführt werden.

## Datenformat

- `uint8`: 1 Byte
- `uint16`: 2 Byte, **Little Endian**
- Beispiel: `SET_POSITION 3200` → `0x13 0x80 0x0C`
- Beispiel: `SET_TRACK 3` → `0x1C 0x03`
- Beispiel: `SET_SPEED 800` → `0x2B 0x20 0x03`

## Responses

| Response | ID | Daten |
|---|---:|---|
| Alive | `0x03` | – |
| Error | `0xF0` | 4 Byte |
| System | `0x10` | 2 Byte |
| CMD | `0x15` | 1 Byte |
| Position | `0x20` | 4 Byte |
| Reference | `0x30` | 1 Byte |
| Track | `0x40` | 3 Byte |
| Motor | `0x50` | 3 Byte |
| ACK | `0x60` | 1 Byte |
| NACK | `0x70` | 1 Byte |
| Help | `0x18` | variabel |

## Terminal-Test

Die Eingabe im Terminal erfolgt als lesbarer ASCII-Command:

\`\`\`text
STATUS
GET_POSITION
SET_TRACK 3
SET_POSITION 3200
SET_SPEED 800
STOP
FIRMWARE
\`\`\`

BabelFish wandelt diese Eingaben in die binären Commands um.
