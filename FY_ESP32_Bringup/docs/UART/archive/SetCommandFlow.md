# UART – SetCommand(...) Flow

> **ESP32-Arbeitsdokument – Nano-Ausgangsstand.** Aus `FY_Controller_Motor/docs/UART/SetCommandFlow.md` übernommen. Diagramme und Funktionen beschreiben zunächst den Nano und werden bei der Portierung an Huginn/Muninn, FreeRTOS und den tatsächlichen Implementierungsstand angepasst. Bis zur Anpassung **keine Aussage über bereits implementierte ESP32-Funktionen**. Zugehöriges Dokumentations-Issue: #162.


SetCommand() is the existing UART-to-Motor hand-off for commands that become MotorJob_t.

```mermaid
flowchart TD
    A["Decoded execute command"] --> B["UART::SetCommand()"]
    B --> C["Check module / motor context"]
    C -->|missing| D["Return false"]
    C -->|available| E["Create MotorJob_t"]
    E --> F["Copy command + data"]
    F --> G["Motor::setJob()"]
    G --> H{Job accepted?}
    H -->|No| I["Return false"]
    H -->|Yes| J["Return true"]
```

## Responsibility boundary

```mermaid
flowchart LR
    U["UART"] -->|validated CommandBuffer| S["SetCommand()"]
    S -->|MotorJob_t| M["Motor::setJob()"]
    M -->|accepted / rejected| S
    S --> U
```

SetCommand() does not start the motor directly and does not inspect the internal motor profile or movement state. Those decisions belong to the Motor module.

### Important distinction

SetCommand() returning true means that the Motor module accepted the job.

It does **not** mean that physical movement has already completed.

The exact rejection reason and the system-wide error mapping are handled separately by the command/error architecture.
