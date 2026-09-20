# UART – SetCommand(...) Flow
Flow from a decoded UART command to a motor job.

```mermaid
flowchart TD
    A[Decoded command] --> B[SetCommand (...)]
    B --> C[Read CommandBuffer]
    C --> D{Command valid?}

    D -->|No| E[Set command status: invalid]
    E --> F[Return]

    D -->|Yes| G[Create / fill MotorJob_t]
    G --> H[Motor::setJob()]
    H --> I{Job accepted?}

    I -->|No| J[Set command status: rejected]
    J --> F

    I -->|Yes| K[Command accepted]
    K --> F
```

## Responsibility boundary



```mermaid
flowchart LR
    U[UART]-->|CommandBuffer_t| S[SetCommand(...)]
    S -->|MotorJob_t| M[Motor::setJob()]
    M -->|accepted / rejected| S
    S --> U
```

SetCommand does not start the motor directly and does not inspect the internal motor profile or movement state. Those decisions belong to the Motor module.
