# BabelFish – aktueller Modulvertrag

BabelFish wandelt ASCII-Zeilen über USB-UART in decodierte Befehle einschließlich Hex-DATA um. Die UART-Schicht übernimmt die Befehlsvalidierung und erzeugt die binären Antworttelegramme. Der Parser ist keine zweite Motorsteuerung. [Gegenstellenvertrag](../../src/UART_Command_Spickzettel.md).

## Daten- und Funktionsdiagramme (Release 2.0.0)

## Aktueller Daten- und Funktionsfluss

```mermaid
flowchart LR
  A["USB UART / ASCII"] --> B["Zeilenpuffer"]
  B --> C["Befehl und Hex-DATA parsen"]
  C --> D{"Syntaktisch gültig?"}
  D -->|Ja| E["BabelFishCommand_t an UART"]
  D -->|Nein| F["Parserfehler an UART"]
```

```mermaid
sequenceDiagram
  participant Host
  participant BabelFish
  participant UART
  Host->>BabelFish: ASCII-Zeile
  BabelFish->>BabelFish: Syntax / Hex-DATA prüfen
  alt gültig
    BabelFish-->>UART: decoded
  else ungültig
    BabelFish-->>UART: INVALID
  end
```

**Zugehörige Issues:** [#150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150), [#171](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/171).
