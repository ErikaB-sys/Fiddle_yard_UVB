# Module States – FY Controller

> **Arbeitsdokument / erster Stand**
>
> Diese Datei ist eine systemweite Übersicht der aktuell im Code definierten Zustandsmaschinen.
> Die eigentliche Zustandsdefinition bleibt jeweils im zuständigen Modul.
>
> Die Übersicht darf sich ändern, wenn Zustände umbenannt, ergänzt, verschoben oder in
> Zustandsmaschine und Statusinformation sauberer getrennt werden.

## 1. System – \`FY_SystemState_t\`

Quelle: \`FY_Controller_Motor/include/FY_System.h\`

\`\`\`mermaid
stateDiagram-v2
    [*] --> INIT
    INIT --> REF
    REF --> Idle
    Idle --> Busy
    Busy --> Idle
    Busy --> Error
    Idle --> Error
    Error --> INIT
\`\`\`

Aktuell definierte Zustände:

- \`INIT\` – Systeminitialisierung nicht abgeschlossen
- \`REF\` – System befindet sich im Referenzzustand
- \`Idle\` – System wartet auf Befehle
- \`Busy\` – System bearbeitet eine Aufgabe
- \`Error\` – System befindet sich im Fehlerzustand

---

## 2. Motor – \`MotorState_t\`

Quelle: \`FY_Controller_Motor/lib/Motor/Motor.h\`

\`\`\`mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> CONF
    CONF --> REFRUN
    CONF --> MOVING
    MOVING --> STOPPED
    MOVING --> ERROR
    REFRUN --> STOPPED
    REFRUN --> ERROR
    STOPPED --> IDLE
    STOPPED --> MOVING
    ERROR --> IDLE
\`\`\`

Aktuell definierte Zustände:

- \`IDLE\`
- \`CONF\`
- \`REFRUN\`
- \`MOVING\`
- \`STOPPED\`
- \`ERROR\`

Die konkrete Übergangslogik ist noch nicht als verbindliche Gesamt-Zustandsmaschine festgelegt.
Das Diagramm zeigt daher den aktuellen Arbeitsstand und ist **nicht** als endgültige Spezifikation zu verstehen.

---

## 3. Referenzierung – \`REF_STATE\`

Quelle: \`FY_Controller_Motor/include/FY_System.h\`

\`\`\`mermaid
stateDiagram-v2
    [*] --> REF_REQ
    REF_REQ --> REFR_EXEC
    REFR_EXEC --> REF_VALID
    REFR_EXEC --> REF_ERROR
    REF_ERROR --> REF_REQ
    REF_VALID --> REF_REQ
\`\`\`

Aktuell definierte Zustände:

- \`REF_REQ\` – Referenzierung erforderlich / angefordert
- \`REFR_EXEC\` – Referenzlauf läuft
- \`REF_VALID\` – Referenzierung erfolgreich
- \`REF_ERROR\` – Referenzierung fehlgeschlagen

**Wichtig:** Diese Zustände sind keine Fehlercodes. Insbesondere \`REFR_EXEC\` und \`REF_VALID\`
werden über den normalen Statusweg (z. B. \`GETSTATUS\`) abgefragt bzw. gemeldet.

---

## 4. UART – Empfang – \`ReceiveState\`

Quelle: \`FY_Controller_Motor/lib/UART/UART.h\`

\`\`\`mermaid
stateDiagram-v2
    [*] --> FindCommand
    FindCommand --> ReadData
    ReadData --> CheckCRC
    CheckCRC --> FindCommand
    CheckCRC --> ReadData
\`\`\`

Aktuell definierte Zustände:

- \`FindCommand\`
- \`ReadData\`
- \`CheckCRC\`

---

## 5. UART – Command-Ergebnis – \`UART_CommandStatus_t\`

Quelle: \`FY_Controller_Motor/lib/UART/UART.h\`

\`\`\`mermaid
stateDiagram-v2
    [*] --> VALID
    VALID --> CMD_INVALID
    VALID --> DATA_INVALID
    VALID --> CRC_INVALID
\`\`\`

Aktuell definierte Ergebnisse:

- \`VALID\`
- \`CMD_INVALID\`
- \`DATA_INVALID\`
- \`CRC_INVALID\`

Diese Werte sind aktuell als Command-Ergebnis definiert und werden bei der weiteren
Fehlercode-Architektur noch gegen die systemweiten Error-Codes abgegrenzt.

---

## 6. Endschalter / Positionssensorik

Aktuell ist hierfür noch keine eigenständige Zustandsmaschine im Code definiert.

Sobald die Sensorlogik als eigenes Modul mit eigenen Zuständen vorliegt, wird sie hier als
eigenes Kapitel ergänzt.

---

## 7. Systemweite Übersicht

\`\`\`mermaid
flowchart TB
    SYS["System<br/>FY_SystemState_t"]
    MOTOR["Motor<br/>MotorState_t"]
    REF["Referenzierung<br/>REF_STATE"]
    UART_RX["UART Empfang<br/>ReceiveState"]
    UART_CMD["UART Command<br/>UART_CommandStatus_t"]
    SENSOR["Endschalter / Sensorik<br/>noch offen"]

    SYS --> MOTOR
    SYS --> REF
    SYS --> UART_RX
    UART_RX --> UART_CMD
    MOTOR --> REF
    MOTOR --> SENSOR
\`\`\`

### Grundsatz

Die Diagramme in dieser Datei sind eine **Übersicht**, keine zweite Quelle der
Zustandsdefinition.

Die fachliche Definition eines Zustands bleibt beim jeweiligen Modul:

- System → \`FY_System.h\`
- Motor → \`Motor.h / motor.cpp\`
- UART → \`UART.h / UART.cpp\`
- Referenzierung → zuständige Referenzierungslogik
- Sensorik → zuständiges Sensor-/Endschaltermodul

Diese Datei dient dazu, beim Systemdesign schnell erkennen zu können, **welche
Zustandsmaschinen existieren und wie sie zueinander stehen**.
