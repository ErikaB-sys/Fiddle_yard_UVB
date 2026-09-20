# UART – Receive Flow

Flow of the UART receive state machine.

```mermaid
flowchart TD
    A[Byte received] --> B{Receive state}
    B -->|CheckCRC| E[Check CRC]
    B -->|FindCommand| C[Find command ID]
    B -->|ReadData| D[Read command data]
    

    C --> F{Command found?}
    F -->|Yes| H{Data required?}
    F -->|No| G[Ignore / wait for next command byte]
    
    H -->|Yes| D
    H -->|No| E
    

    D --> I{All data bytes received?}
     I -->|Yes| E
     I -->|No| J[Wait for next byte]
    


    E --> K{CRC valid?}
    K -->|Yes| M[Command valid]
    K -->|No| L[DATA_INVALID / reject command]
    
    M --> N[Dispatch command]

    D --> O{Inter-byte timeout?}
    O -->|Yes| L

    E --> P{CRC byte timeout?}
    P -->|Yes| L

    L --> Q[Reset receive state]
    N --> Q
    G --> A
    J --> A
```

## Timeout rule

The inter-byte timeout starts with the command byte and is refreshed for every received byte.

The timeout is derived from the configured UART baud rate. The current design uses a basis of 5 byte times (8N1: approximately 10 bit times per byte).

A timeout does not evaluate command semantics. It only protects the technical telegram reception.
