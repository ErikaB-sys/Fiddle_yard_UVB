# ESP32 – Modul- und Taskregeln

## Zuständigkeit
- Bestehende funktionale Modulnamen aus dem Nano beibehalten (z. B. `lib/UART/`, `lib/Motor/`, `lib/BabelFish/`). Keine neuen Sammelmodule als Ersatz für die fachliche Gliederung.
- Jedes Modul besitzt seinen Zustand, seine Funktionen und seine FreeRTOS-Task. Es exportiert nur die benötigten Start-/Schnittstellenfunktionen.
- `src/main.cpp` zeigt in `setup()` die Initialisierung und das Verbinden der Module; `loop()` dokumentiert ausdrücklich die verbleibende Arduino-Loop-Verantwortung. Kein Protokolldecoder, CRC, Motorablauf oder Task-Loop in `main.cpp`.
- Für die gemeinsam benötigten Nachrichtentypen ist das Schnittstellen-Header `include/FY_Bringup.h` zuständig. Bei der eigentlichen Nano-Portierung `MotorJob_t` als vorhandenen Vertrag verwenden, keine parallele Fachsemantik einführen.

## Namenskonvention
- **Huginn = Core 0:** UART, BabelFish, Kommunikation, spätere Bedienelemente und WLAN.
- **Muninn = Core 1:** Motor, Sensoren, Position, Referenzfahrt, Sicherheit.
- Task-bezogene exportierte Funktionen tragen den Owner als Präfix: `Huginn_startUART()`, `Muninn_startMotorMock()`. Gleiches bei späteren Core-gebundenen Funktionen.
- **Ausnahme:** Bestehende Nano-API- und Protokollnamen (z. B. `SetCommand()`, `setJob()`, `prepareGo()`, `prepareStop()`) werden **nicht** nur für die Namenskonvention umbenannt: Parität und Nachvollziehbarkeit gehen vor. Private Helfer benötigen keinen Owner-Präfix.
- Die numerischen Core-IDs bleiben als `CORE_HUGINN=0` und `CORE_MUNINN=1` nachvollziehbar.
- FreeRTOS-Tasknamen beginnen mit `Huginn_` bzw. `Muninn_`.

## PlattformIO und Tests
- In `lib/<Modul>/` gehört zum `.cpp` auch ein öffentlicher `.h`, den `src/main.cpp` oder ein anderes aktiv verwendetes Modul einbindet. So erkennt der PlatformIO Library Dependency Finder das Modul und kompiliert dessen Implementierung.
- Kompilier- und Linktest nach jeder Verschiebung von Dateien. Danach Smoke-Test mit `GO AA 55`.
- Der aktuelle Motor ist ein Dummy und erzeugt **keine** STEP-Impulse und keine echten Nano-Responses.
- Änderungen per Commit einem konkreten Issue zuordnen: #152 RTOS/Queues, #156 UART, #153 ISR/Hardware, #154 Motorlogik, #162 Architektur.
