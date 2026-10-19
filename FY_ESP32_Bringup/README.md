# FY – ESP32-WROOM / Huginn und Muninn

**Portierungsbranch:** `feature/FY_ESP32_Port`  
**Status:** ESP32-Bring-up; **kein produktiver Motorbetrieb**.

## Aufbau

| Besitzer | ESP32-Core | Aufgaben |
| --- | --- | --- |
| **Huginn** | Core 0 | UART, BabelFish, Kommandoprüfung, später Keyboard/OLED/WLAN und Response-Versand |
| **Muninn** | Core 1 | Motor-Dummy; später Motor, ISR, Referenzierung, Switches und Sicherheit |

**Stand im Code:** Die `main.cpp` erstellt Queue-Handles und startet die im jeweiligen Modul definierten Tasks. UART und Motor-Dummy laufen in `lib/UART/` bzw. `lib/Motor/`. Die Originale der Nano-Firmware bleiben im benachbarten `FY_Controller_Motor/`-Baum.

## Bereits übernommen / untersucht
- Nano-`Protokoll.h` und `BabelFish`; ASCII-zu-Kommando und CRC/Tabellenvalidierung getestet.
- FreeRTOS-Queues zwischen Huginn und Muninn (Bring-up-Nachrichten).
- LED-getriebener Motor-Dummy auf Muninn, **keine** STEP-, DIR- oder ENABLE-Ausgabe.
- Nano-Command-Spickzettel als Ausgangspunkt mit Portierungshinweis.
- Modulnahe Diagramme als Nano-Ausgangsstand; diese werden während der Portierung auf die tatsächliche ESP32-Implementierung angepasst.

**Noch nicht implementiert:** Originale Nano-UART-Handler/ACK/NACK, `MotorJob_t`-Übergabe, Response-Task, Motorzustandsautomat, `prepareStop()`/`prepareGo()`, Timer/ISR, echte Sensoren, Fehlerverwaltung und Keyboard.

## Plattform und erster Funktionstest
PlatformIO-Projektordner `FY_ESP32_Bringup`; `board = esp32dev`, Arduino-Framework, Monitor 115200 Baud.

1. `git pull`, Projekt in PlatformIO öffnen, bauen und hochladen.
2. UART-Monitor öffnen. Erwartete Startmeldung: `FY ESP32: Huginn (UART) <-> Muninn (motor mock), NO MOTION`.
3. Als derzeit getestetes Beispiel `GO AA 55` eingeben. BabelFish akzeptiert Hex-DATA-Bytes als **separate Tokens**.
4. Erwartete Bring-up-Rückmeldung: `FY_TEST: seq=... cmd=0x07 Muninn_core=1 accepted=1`.

Dies ist eine **simulierte Testantwort**, kein echtes Nano-ACK; sie bestätigt weder die GO-Verriegelung noch Motorbewegung. Der zuletzt umgebaute Modulschnitt muss lokal erneut kompiliert und getestet werden.

**GPIO2** wird ausschließlich als optionale Test-LED verwendet (Konfiguration in `platformio.ini`). Keine zusätzlichen Lasten ohne Boardprüfung anschließen.

## Dokumentation – lebende ESP32-Arbeitsstände
- [Coding Rules](docs/CODING_RULES.md) und [Systemarchitektur](docs/SYSTEM_ARCHITECTURE.md)
- [Systemdiagramme](docs/System/)
- [UART-Datenmodell und Ablauf](lib/UART/UART.md) und [UART-Command-Spickzettel](src/UART_Command_Spickzettel.md)
- [Motor, ISR, Referenzierung](lib/Motor/Motor.md)
- [Switches: Sensor- und Sicherheitspfad](lib/Switches/Switches.md)
- [Error: Fehlerverwaltung](lib/Error/Error.md)
- [Keyboard: Tasten und OLED](lib/Keyboard/Keyboard.md)
- [BabelFish: ASCII-Adapter](lib/BabelFish/BabelFish.md)

Die Moduldiagramme stammen zunächst aus dem Nano und werden **an Ort und Stelle** angepasst. Es gibt keine zweite dauerhafte Nano-Referenzablage im ESP32-Baum. Der unveränderte Nano-Stand bleibt in `FY_Controller_Motor/` im selben Repository.

## Weiteres Vorgehen
1. #152: `MotorJob_t` über FreeRTOS-Queues und simulierte Motorantworten.
2. #156: Nano-UART-Handler und Response-Sendeweg unverändert in der Befehlssemantik portieren.
3. #163: Cppcheck/MISRA früh einbauen und prüfen.
4. #154/#153/#155: Motorzustand, Hardware-STEP/ISR sowie Sensoren mit direkten Abschaltpfaden auf Muninn.
5. #157: Keyboard auf Huginn; I²C-Hardware prüfen.

Jeweilige Codeänderungen und passende Moduldiagramme gehören in denselben Issue-bezogenen Commit (#162 für übergreifende Architektur).
