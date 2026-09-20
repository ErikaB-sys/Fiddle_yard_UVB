# MOTOR – Prepare LEFT Flow

Flow of the motor preparation logic for a LEFT movement.

The flow is intentionally used as a **coding guide**: it defines the decision order and makes safety/definition gaps visible before implementation.

```mermaid
flowchart TD

    B{MOTOR Busy}
    B -->|YES| G[return false]
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

`MOTOR_PrepaireRight.md` is intended to follow the same logic, mirrored for the RIGHT direction.

The diagrams are also intended as a basis for visualizing the reference run later and for identifying missing or ambiguous definitions before coding.