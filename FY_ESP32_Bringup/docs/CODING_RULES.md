# ESP32 – Modul- und Taskregeln

## Zuständigkeit
- Bestehende funktionale Modulnamen aus dem Nano beibehalten (z. B. `lib/UART/`, `lib/Motor/`, `lib/BabelFish/`). Keine neuen Sammelmodule als Ersatz für die fachliche Gliederung.
- Jedes Modul besitzt seinen Zustand und seine Funktionen. **Wenn** es eine FreeRTOS-Task benötigt, gehört deren Implementierung und Startfunktion ins jeweilige Modul. Keine künstliche eigene Task nur um ein Modul zu bedienen (insbesondere nicht zwingend für `Error`).
- `src/main.cpp` zeigt in `setup()` die Initialisierung und das Verbinden der Module; `loop()` dokumentiert ausdrücklich die verbleibende Arduino-Loop-Verantwortung. Kein Protokolldecoder, CRC, Motorablauf oder Task-Loop in `main.cpp`.
- Für die gemeinsam benötigten Nachrichtentypen ist das Schnittstellen-Header `include/FY_Bringup.h` zuständig. Bei der eigentlichen Nano-Portierung `MotorJob_t` als vorhandenen Vertrag verwenden, keine parallele Fachsemantik einführen.

## Namenskonvention
- **Huginn = Core 0:** UART, BabelFish, Keyboard, **Error (zentrale Fehlerverwaltung)**, spätere Kommunikation/WLAN.
- **Muninn = Core 1:** Motor, **Switches/Sensoren**, Position, Referenzfahrt und unmittelbarer Hardwareschutz.
- Task-bezogene exportierte Funktionen tragen den Owner als Präfix: `Huginn_startUART()`, `Muninn_startMotorMock()`. Gleiches bei späteren Core-gebundenen Funktionen.
- **Ausnahme:** Bestehende Nano-API- und Protokollnamen (z. B. `SetCommand()`, `setJob()`, `prepareGo()`, `prepareStop()`) werden **nicht** nur für die Namenskonvention umbenannt: Parität und Nachvollziehbarkeit gehen vor. Private Helfer benötigen keinen Owner-Präfix.
- Die numerischen Core-IDs bleiben als `CORE_HUGINN=0` und `CORE_MUNINN=1` nachvollziehbar.
- FreeRTOS-Tasknamen beginnen mit `Huginn_` bzw. `Muninn_`.

## Fehlermodell und Kerngrenze
- Die vorhandene Nano-Fehlercodierung (oberes Nibble = Fehlerort, unteres Nibble = Fehlerart) bleibt erhalten.
- `Error` läuft logisch auf **Huginn** und ist alleiniger Besitzer des zentralen Fehlerzustands. Fehler von Muninn werden als Ereignisse über die RTOS-Verbindung übertragen; dort wird **kein** konkurrierender Fehlerregister-Klon gepflegt.
- Muninn muss zeitkritische Schutzaktionen unabhängig und unmittelbar ausführen. Das anschließende Fehlerereignis an Huginn ist nachgelagert, nicht sicherheitskritisch.
- Bei voller Queue muss es einen definierten Umgang mit verlorenen Ereignissen und einen späteren Statusabgleich geben; das wird bei der Implementierung festgelegt und getestet.

## PlattformIO und Tests
- In `lib/<Modul>/` gehört zum `.cpp` auch ein öffentlicher `.h`, den `src/main.cpp` oder ein anderes aktiv verwendetes Modul einbindet. So erkennt der PlatformIO Library Dependency Finder das Modul und kompiliert dessen Implementierung.
- Kompilier- und Linktest nach jeder Verschiebung von Dateien. Danach Smoke-Test mit `GO AA 55`.
- Der ursprüngliche Motor-Dummy wurde inzwischen durch einen motorischen Bench-Pfad mit A4988/NEMA11, STEP-Timer und Profil-Executor erweitert. **Das ist noch keine freigegebene FY-Mechaniksteuerung.** Status siehe [Motor-Modulbeschreibung](Motor/README.md).
- Änderungen per Commit einem konkreten Issue zuordnen: #152 RTOS/Queues, #156 UART, #153 ISR/Hardware, #154 Motorlogik, #162 Architektur.

## Dokumentation als Modulbestandteil (verbindlich)
- Nano-Dokumente dienen als **Ausgangsstand**, nicht als dauerhaft doppelt gepflegte Referenz im ESP32-Baum.
- **Neue, verbindliche Entscheidung vom 21.10.2026:** Modulbeschreibungen, Datenmodelle und Diagramme zentral unter `docs/<Modul>/README.md` (mit optionalen Unterseiten), Systemdiagramme unter `docs/System/`; bei `lib/<Modul>/` verbleiben **nur C++ und Doxygen-Kommentare**. Frühere MD-Arbeitsdiagramme stehen als historische Referenzen in `docs/<Modul>/archive/`. Siehe [DOCUMENTATION_RULES.md](DOCUMENTATION_RULES.md).
- Nano-Ausgangsdiagramme im Archiv als **historisch** kennzeichnen; für den ESP32-Zielstand die Anforderungen aus den referenzierten Issues ableiten. Alte Nano-Bilder niemals automatisch als aktuellen Code-/Sollstand ausgeben.
- Änderungen an Verhalten, Queue-Schnittstellen und Modulzuständigkeiten erfordern den Abgleich **Issue → Modulbeschreibung → Code** sowie Rückwärtsprüfung. Offene Deltas in `docs/VERIFICATION_GAPS.md`; die Änderungen nach Möglichkeit im selben Commit dokumentieren.
- `Keyboard` und `Switches` sind noch nicht als ESP32-Module implementiert. `Error.h/.cpp` **sind bereits vorhanden**, aber die geplante zentrale, kernübergreifende Fehleraggregation ist weiter offen. Einzelheiten in den Moduldokumenten.

## Sensor- und Sicherheitsregel (Entwurf; #155)
- `Switches` gehört **Muninn**, nicht Huginn. Der sicherheitskritische Eingangspfad bleibt direkt bei Motor/ISR; die Queue ist **kein Teil der Stopp-Kette**.
- `SwitchesSnapshot` ist intern zweibyteig geplant: `info` (REF, TRIM_LEFT, TRIM_RIGHT, TIMING_BELT: Bits 0–3) und `error` (Fehlercode nach bestehendem Ort/Typ-Schema).
- Muninn veröffentlicht Statusänderungen sowie zeitlich begrenzte Aktualisierungen über eine Ein-Platz-Mailbox (`xQueueOverwrite`); Huginn verwaltet ein gültiges, frisches Abbild für UART und Keyboard.
- `STATUS_Switches` bleibt auf der externen UART-Schnittstelle **ein Datenbyte**. Das zweite Byte ist ausschließlich ein interner Fehlertransport; zentrale Fehlerantwort `STATUS_Error` bleibt unverändert.
- Der `Error`-Modulzustand darf nicht ungeschützt auf beiden Kernen geschrieben werden. Fehlererkennung bleibt beim Modul; die systemweite Diagnose führt Meldungen kontrolliert zusammen.
