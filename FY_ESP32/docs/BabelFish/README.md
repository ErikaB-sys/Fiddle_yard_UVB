# BabelFish – ASCII-Adapter des UART-Moduls

**Soll:** [#156](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/156). Besitzer Huginn/UART; **keine eigene Task**.

```mermaid
flowchart LR
  A["Terminal ASCII"] --> B["BabelFish"]
  B --> U["UART: ID/Länge/CRC, Handler"]
  U --> C["Response oder Motor-Queue"]
```

BabelFish dekodiert Kommando-Token und Hex-DATA, ergänzt CRC für den Terminalpfad; **keine** Entscheidung über freigegebene Motorbefehle und **keine** zweite Protokolltabelle.

**Ist:** [BabelFish.h/.cpp](../../lib/BabelFish/BabelFish.h) aktiv eingebunden; `GO` per ASCII laut HW2-UART-Berichten geprüft. Sonderfälle/Parserfehler nicht vollständig abgenommen.

- [x] Ziel und Codebezug geklärt.
- [ ] Parsergrenzen, Binärkompatibilität, gemeinsame UART-TX-Hoheit prüfen.
- [ ] MISRA-/Reviewbeleg erfassen.

Offen: [VERIFICATION_GAPS.md](../VERIFICATION_GAPS.md); Altstand [archive/BabelFish.md](archive/BabelFish.md).

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
