# MOTOR – Prepare RIGHT Flow

Flow of the motor preparation logic for a RIGHT movement.

The flow mirrors the LEFT preparation logic in the opposite direction.

```mermaid
flowchart TD

    B{MOTOR Busy}
    B -->|YES| G[return false]
    B -->|NO| A{Referenziert?}

    A -->|No| C{Requested Steps <= 200?}
    C -->|No| G
    C -->|Yes| K{ENDSCHALTER RIGHT aktiv?}

    A -->|Yes| E{Requested Steps <= available RANGE?}
    E -->|No| G
    E -->|Yes| K

    K -->|Yes| G
    K -->|No| L[CALC_PROFILE]
    L --> D[Start Timer]
    D --> R[return true]
```

Die LEFT- und RIGHT-Flows sind bewusst gespiegelt, damit die Richtungslogik beim Coding eindeutig bleibt.
