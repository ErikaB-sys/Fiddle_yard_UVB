# UART – aktueller Modulvertrag

Huginn liest BabelFish-Kommandos, prüft ID, Länge und CRC und beantwortet GET-Abfragen unmittelbar. Fahrbefehle gelangen über die normale Muninn-Queue; STOP über die Prioritätsqueue. ACK/NACK ist an die eingegangene Sequenz gebunden. Ein ACK ist kein Fahrtende. Vollständige Drahtformate: [UART-Vertrag](../../src/UART_Command_Spickzettel.md).

## Daten- und Funktionsdiagramme (Release 2.0.0)

## Datenfluss und Ablauf

```mermaid
flowchart LR
  H["Host"] --> B["BabelFish"]
  B --> V["UART: ID/Länge/CRC"]
  V --> R{"Command-Typ"}
  R -->|IMMEDIATE| S["Snapshots lesen"]
  R -->|EXECUTE| Q["Muninn-Queue"]
  R -->|PRIORITY STOP| P["STOP-Queue"]
  Q --> E["ACK/NACK nach Sequenz"]
  P --> E
  S --> TX["STATUS + CRC"]
  E --> TX
  TX --> H
```

```mermaid
sequenceDiagram
  participant Host
  participant UART
  participant Muninn
  Host->>UART: Fahrkommando
  UART->>Muninn: CommandMessage
  Muninn-->>UART: MotorEvent (ACK/NACK)
  UART-->>Host: 60 oder 70
  loop Bewegung verfolgen
    Host->>UART: STATUS / GET_TRACK
    UART-->>Host: Snapshot
  end
```

**Zugehörige Issues:** [#150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150), [#171](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/171).
