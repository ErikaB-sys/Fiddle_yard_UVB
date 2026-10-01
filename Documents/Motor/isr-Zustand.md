# ISR-Zustand – Motor

## Zweck

Dieses Dokument beschreibt den aktuellen Zustand von Timer1, `stepRun`, `profileRun` und dem Referenzlauf in `FY_HW_TEST`.

Es dient als Grundlage für die weitere Untersuchung des Motor- und Positionierverhaltens.

## Grundprinzip

Timer1 erzeugt aktuell dauerhaft Compare-Interrupts. Ob tatsächlich STEP-Pulse erzeugt werden, entscheidet `stepRun`.

```mermaid
flowchart LR
    TIMER["Timer1 Compare ISR"]
    CHECK{"stepRun?"}
    IDLE["STEP LOW<br/>return"]
    STEP["STEP toggeln<br/>Position bearbeiten<br/>Profil bearbeiten"]

    TIMER --> CHECK
    CHECK -->|false| IDLE
    CHECK -->|true| STEP
    STEP --> TIMER
```

Damit ist `stepRun=false` **nicht gleichbedeutend mit einem vollständig beendeten Auftrag**.

## Zustandsfluss

```mermaid
flowchart TD

    START["Start / Setup"]
    TIMER["Timer1 Compare ISR<br/>OCIE1A aktiv"]

    IDLE["Ruhezustand<br/>stepRun = false<br/>profileRun = false"]

    MOVE["Position Move<br/>profileRun = true<br/>stepRun = true"]
    PROFILE["Motorprofil läuft<br/>ACC / V / Position"]
    MOVE_DONE["Profil beendet<br/>profileRun = false<br/>stepRun = false"]
    MOVE_LIMIT["Endschalter<br/>profileRun = false<br/>stepRun = false"]

    REF_START["REF_START<br/>stepRun = true"]
    REF_LEFT["REF_LEFT_END"]
    REF_RIGHT["REF_SEARCH_RIGHT"]
    REF_APPROACH["REF_APPROACH_REF"]
    REF_RETURN["REF_RETURN_TO_REF"]
    REF_SHIFT["REF_SHIFT_LEFT"]
    REF_MEASURE["REF_MEASURE_LEFT"]
    REF_CALC["REF_CALCULATE"]
    REF_VALID["Referenz gültig<br/>refRunActive = false"]
    REF_ERROR["REF_ERROR<br/>refRunActive = false"]

    START --> TIMER
    TIMER --> IDLE

    IDLE -->|Position Move| MOVE
    MOVE --> PROFILE
    PROFILE -->|Profil vollständig abgearbeitet| MOVE_DONE
    PROFILE -->|Endschalter| MOVE_LIMIT
    MOVE_DONE --> IDLE
    MOVE_LIMIT --> IDLE

    IDLE -->|Referenzlauf| REF_START
    REF_START --> REF_LEFT
    REF_LEFT -->|Limit Left| REF_RIGHT
    REF_RIGHT -->|Limit Right| REF_APPROACH
    REF_APPROACH -->|Position erreicht| REF_RETURN
    REF_RETURN -->|Referenzkante erkannt| REF_SHIFT
    REF_SHIFT -->|Position erreicht| REF_MEASURE
    REF_MEASURE -->|2. Messung fertig| REF_CALC
    REF_CALC --> REF_VALID
    REF_VALID --> IDLE

    REF_LEFT -->|Short Reference| MOVE
    REF_LEFT -->|Fehler| REF_ERROR
    REF_ERROR --> IDLE
```

## Timer-Zustand und Bewegungszustand

Für die weitere Architektur sollten zwei Dinge getrennt betrachtet werden:

```mermaid
stateDiagram-v2

    [*] --> TIMER_OFF

    TIMER_OFF --> TIMER_ON: Bewegung / Referenzphase starten

    state TIMER_ON {

        [*] --> PAUSE

        PAUSE --> RUN: stepRun = true
        RUN --> PAUSE: stepRun = false

        RUN --> RUN: Timer ISR erzeugt STEP
    }

    TIMER_ON --> TIMER_OFF: kompletter Auftrag beendet
```

### Bedeutung

| Zustand | Bedeutung |
|---|---|
| Timer OFF | Timer1 Compare-Interrupt deaktiviert |
| Timer ON + `stepRun=false` | ISR läuft, erzeugt aber keine Schritte |
| Timer ON + `stepRun=true` | Motor kann Schritte erzeugen |
| Timer OFF nach Auftrag | kompletter Bewegungs-/Referenzauftrag beendet |

## Wichtige Beobachtung

Der aktuelle Code aktiviert in `init_step_timer()` dauerhaft:

```cpp
TIMSK1 |= _BV(OCIE1A);
```

Die ISR selbst verhindert bei `stepRun=false` weitere STEP-Flanken:

```cpp
if (!stepRun)
{
    stepLevel = false;
    PORTD &= ~_BV(PD3);
    return;
}
```

Bei einem normalen Positionierprofil wird am Ende außerdem:

```cpp
profileRun = false;
stepRun = false;
```

gesetzt.

**Daraus folgt:** Der dauerhaft aktivierte Timer ist nach aktuellem Codeverständnis nicht automatisch die Ursache für weitere Motor-Schritte. Er läuft jedoch unnötig weiter und ist vom eigentlichen Bewegungszustand getrennt.

## Konsequenz für die weitere Untersuchung

Der Timer-Interrupt sollte nicht einfach bei jedem

```cpp
stepRun = false;
```

deaktiviert werden.

Im Referenzlauf wird `stepRun=false` mehrfach nur als kurze Pause zwischen zwei Bewegungsphasen verwendet.

Sinnvoll wäre daher eine spätere Trennung von:

- **Timer1 aktiv/inaktiv**
- **`stepRun`**
- **`profileRun`**
- **`refRunActive` / `refRunState`**

Erst danach sollte entschieden werden, an welchen terminalen Zuständen der Timer tatsächlich abgeschaltet wird.

## Aktueller Stand

- Keine Änderung am Motorcode durch dieses Dokument.
- Diagramm beschreibt den aktuellen Stand und die daraus abgeleitete Zielstruktur.
- Die Frage nach einem möglichen ungewollten Weiterlaufen des Motors bleibt ein eigener Prüfschritt.
