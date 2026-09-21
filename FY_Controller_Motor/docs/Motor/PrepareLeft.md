# MOTOR – Prepare LEFT Flow

Flow of the motor preparation logic for a LEFT movement.

The flow is intentionally used as a **coding guide**: it defines the decision order and makes safety/definition gaps visible before implementation.

The **first check is always the logical motor power state**. If the motor is OFF, no movement profile is prepared. The motor power state is tracked separately from the reference state; see **#86 – Motor Enable / Disable und Referenzverlust**.

```mermaid
flowchart TD

    M{Motor ON?}
    M -->|NO| G[return false]
    M -->|YES| B{MOTOR Busy}

    B -->|YES| G
    B -->|NO| A{Referenziert?}

    A -->|No| C{Requested Steps <= 200?}
    C -->|No| G
    C -->|Yes| K{ENDSCHALTER LEFT aktiv?}

    A -->|Yes| E{Requested Steps <= available RANGE?}
    E -->|No| G
    E -->|Yes| K

    K -->|Yes| G
    K -->|No| L[CALC_PROFILE]
    L --> D[Start Timer]
    D --> R[return true]
```

`PrepareRight.md` soll sinngemäß gespiegelt folgen.

Die Diagramme dienen außerdem als Grundlage für die Visualisierung des Reference-Runs und machen fehlende oder noch nicht eindeutig definierte Entscheidungen vor dem Coding sichtbar.
