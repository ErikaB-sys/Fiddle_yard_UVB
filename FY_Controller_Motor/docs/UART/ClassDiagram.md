# UART – Class Diagram

Architecture view of the UART module and its hand-off to the Motor module.

```mermaid
classDiagram
    class UART {
        +Update()
        +SetCommand()
        +getCommandStatus()
        +clearCommandStatus()
        -FindCommand()
        -ReadData()
        -CheckCRC()
        -CalcCRC()
    }

    class CommandBuffer_t {
        +cmd
        +data
        +length
        +crc
    }

    class MotorJob_t {
        +cmd
        +data
        +valid
    }

    class Motor {
        +setJob(MotorJob_t)
        +Update()
    }

    class UART_CommandStatus_t

    UART --> CommandBuffer_t : receives / decodes
    UART --> UART_CommandStatus_t : reports status
    UART --> MotorJob_t : creates
    MotorJob_t --> Motor : submitted via setJob()
```

## Architectural rule

UART is responsible for transport and command decoding.
Motor is responsible for accepting and executing motor jobs.

The UART handlers stay thin. They dispatch the already validated command instead of containing motor-specific execution logic.
