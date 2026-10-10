# UART Commands – Ablauf und Antwortbedingungen

Referenz für die Protokollabstimmung in [Issue #58](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/58).
Die Übersicht unterscheidet bereits implementierte Antwortpfade von noch offenen Handlern.

## Zentrale Übersicht

```mermaid
flowchart TD
    A[ASCII-Eingabe] --> B[BabelFish dekodiert Command]
    B --> C{Command erkannt?}
    C -- Nein --> N1[NACK: ungültige Eingabe]
    C -- Ja --> D[Validierung: Command-ID, DATA-Länge, CRC]
    D --> E{Validierung erfolgreich?}
    E -- Nein --> N2[NACK: ungültiges Telegramm]
    E -- Ja --> F{Command-Typ}
    F -- IMMEDIATE --> G[GET / HELP / FIRMWARE Handler]
    G --> R1[Passende STATUS-Antwort]
    F -- EXECUTE --> H{System Busy?}
    H -- Ja --> N3[NACK mit Command-ID]
    H -- Nein --> I{Execute-Command}
    I -- SET_MODE --> M[SET_MODE Handler]
    M --> M1{Daten und Zustand gültig?}
    M1 -- Ja --> ACK[ACK mit Command-ID]
    M1 -- Nein --> N4[NACK mit Command-ID]
    I -- Andere Execute-CMDs --> T[Antwortverhalten je Handler prüfen]
    F -- PRIORITY --> P[Priority-Handler]
    P --> R2[Antwort gemäß Command-Definition]
```

## SET_MODE – Detailablauf

```mermaid
flowchart TD
    A[CMD_SET_MODE, DATA-Länge 2] --> B{Systemstatus, Keyboard und Motor verfügbar?}
    B -- Nein --> N[NACK, keine Zustandsänderung]
    B -- Ja --> C{System Busy oder Motor bewegt sich?}
    C -- Ja --> N
    C -- Nein --> D{DATA = 55 AA?}
    D -- Ja --> L[Keyboard Mode = LOCAL]
    D -- Nein --> E{DATA = AA 55?}
    E -- Ja --> R[Keyboard Mode = REMOTE]
    E -- Nein --> N
    L --> ACK[ACK mit Command-ID]
    R --> ACK
```

## SET_MODE – Bedingungen

| Bedingung | Antwort | Zustandsänderung |
|---|---|---|
| Erforderliches Modul fehlt | NACK | keine |
| System meldet Busy | NACK | keine |
| Motor bewegt sich | NACK | keine |
| DATA `55 AA` | ACK | Mode = LOCAL |
| DATA `AA 55` | ACK | Mode = REMOTE |
| Anderes DATA-Muster | NACK | keine |

- Command-ID: `CMD_SET_MODE = 0x26` (bisherige SET_LOCAL-ID weiterverwendet).
- DATA-Länge: 2 Byte.
- `CMD_SET_LOCAL` und `CMD_SET_REMOTE` entfallen; die alte ID `0x2A` wird nicht mehr verwendet.
- ACK (`0x60`) und NACK (`0x70`) enthalten jeweils die Command-ID als ein DATA-Byte; die CRC folgt wie üblich.
- SET_MODE schaltet den Motor nicht ein und startet keine Bewegung.
- Der Wechsel ist nur im Stillstand zulässig.

## Noch offene Handler

Die zentrale Übersicht ist keine Behauptung, dass alle Command-Handler bereits vollständig implementiert sind. STOPP bleibt gemäß aktueller Arbeitsabsprache zurückgestellt. Die Antwortpfade der übrigen Execute-Commands werden zusammen mit deren tatsächlicher Implementierung abgeglichen. GET-Antworten richten sich nach den vorhandenen Response-Definitionen und werden mit den Modulen vervollständigt.
