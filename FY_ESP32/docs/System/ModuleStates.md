# Modulzustände – ESP32-Systemüberblick

**Stand: 24.10.2026.** Fachliche Quellen: [#154](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/154)/[#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155); Dokumentationspflege [#12](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/12).

## Motor

Die verbindliche [Motor-Zustandsübersicht](../Motor/README.md#zustandsübersicht--vereinbartes-verhalten) steht beim Motor-Modul. Sie wird hier nicht ein zweites Mal gezeichnet.

| Übergang | Bedeutung |
| --- | --- |
| POR → STOPPED | Antrieb aus, STOP verriegelt, Referenz ungültig. Ein virtueller POR-Mittenanker für SET_POSITION ohne REF entfällt. |
| STOP entriegeln | Bleibt STOPPED; Antrieb bleibt aus. |
| GO EIN bei entriegeltem Stop → IDLE | Antrieb frei; kein alter Auftrag und keine Referenz entstehen. |
| IDLE → MOVING nach Prepare OK | Neuer gültiger Auftrag wird ausgeführt. |
| MOVING → IDLE am Ziel | Job fertig, Abschluss auf Anfrage. |
| STOP → STOPPED | Job verwerfen, Antrieb aus, REF ungültig; vor normaler Fahrt erneut mindestens kurz referenzieren. |

STOP-Verriegelung, Kalibrierzustand und Referenzgültigkeit sind zusätzlich zum Motor-State zu führen. Die Referenzierung ist inzwischen implementiert und am HW3-Aufbau funktional geprueft. Einzelne Fehler- und Robustheitspfade sind weiterhin offen; verbindlich sind die aktuellen Codebedingungen in `Motor.cpp` und die Testnachweise, nicht fruehere Soll-Diagramme.

## Andere Maschinen

Nano-Parser-/Mini-Scheduler-Diagramme sind kein automatischer ESP32-Vertrag. Aktueller Datenfluss: [UART](../UART/README.md), [Systemarchitektur](../SYSTEM_ARCHITECTURE.md). Historische Nano-Diagramme bleiben im [Archiv](archive/ModuleStates_Nano.md).
