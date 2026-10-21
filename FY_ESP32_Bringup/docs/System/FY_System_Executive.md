# FY System Executive – ESP32-Einordnung

**Das frühere Nano-Mini-Scheduler-Konzept ist nicht die aktive ESP32-Zielarchitektur.** Der unveränderte Ausgangstext ist archiviert: [archive/FY_System_Executive_Nano.md](archive/FY_System_Executive_Nano.md).

Statt Nano-`loop() → FY_System.update() → AVR Timer1` nutzt der ESP32 das in [#150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150) und [#152](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/152) vereinbarte FreeRTOS-Taskmodell: Huginn/Core 0 für Kommunikation, Muninn/Core 1 für den Motor und den ESP32-STEP-Timer als ISR/Hardware-Pfad. `src/main.cpp` startet die Tasks und bleibt im `loop()` ansonsten idle.

**Verbindliche Darstellung:** [System README](README.md), [Architektur](../SYSTEM_ARCHITECTURE.md), [UART-Datenfluss](UART_Datenfluss.md). Entwurfs-/Codeunterschiede: [VERIFICATION_GAPS.md](../VERIFICATION_GAPS.md).

Die frühere Nano-Implementierung und ihr Mini-Scheduler-Text bleiben historisch erhalten, erhalten aber keine neue ESP32-Entwurfsautorität.
