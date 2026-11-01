# Historische Entwicklungsunterlagen – nicht der aktuelle Firmwarevertrag

**Stand der Einordnung:** 01.11.2026 · Branch `feature/FY_ESP32_Cleanup`.

Diese Seite trennt **aktuelle Betriebs-/Schnittstellendokumentation** von **Entscheidungs- und Testhistorie**. Historische Dateien bleiben absichtlich unveraendert, damit damalige Messwerte, Entscheidungen und Testschritte nachvollziehbar bleiben. Sie duerfen **nicht** als Anleitung fuer die aktuelle HW3-Firmware verwendet werden.

## Verbindlicher aktueller Einstieg

- [Handbuch und Lesepfad](README.md)
- [Systemarchitektur / Startfolge / Queues](SYSTEM_ARCHITECTURE.md)
- [UART-Drahtvertrag](../src/UART_Command_Spickzettel.md)
- [Keyboard](Keyboard/README.md), [Motor](Motor/README.md), [Sensorik](Switches/README.md), [NVM](nvm/README.md), [Fehler](Error/README.md)
- [Aktuelle HW3-Pinbelegung](pinouts/01_PINOUT_ESP32.md)

## Historische Berichte, Portierung und Entscheidungen

| Dokumente | Einordnung |
| --- | --- |
| [UART-Portierung Schritt 1](UART_PORT_STEP1.md), [Schritt 2](UART_PORT_STEP2.md) | Frueher UART-/Motor-Mock, teils falsche heutige STATUS-Antworten und HW2-Pins |
| [Motor-/MOVE-Tischtest](MOTOR_TIMER_BENCH.md) | HW2-Bench und damalige ungebaute Zwischenstaende, kein aktueller HW3-Betriebsvertrag |
| [Motor-Portierungsbericht 20.10.](reports/MOTOR_PORT_STATUS_2026-10-20.md), [Profil-Walkthrough](reports/PROFILE_PORT_WALKTHROUGH.md) | Zeitbezogene Portierungs-/Testprotokolle |
| [Soll-Ist-Abgleich D01–D19](VERIFICATION_GAPS.md) | Historischer Review-/Entscheidungsstand vom 25.10.; fuer aktuelle offene Risiken siehe Systemarchitektur und Issues |
| [Nano-/Modularchive](Motor/archive/), [UART-Archiv](UART/archive/), [Systemarchiv](System/archive/), [Keyboard-Archiv](Keyboard/archive/), [Switches-Archiv](Switches/archive/), [Error-Archiv](Error/archive/), [BabelFish-Archiv](BabelFish/archive/) | Fruehere Entwuerfe, Nano-Verhalten und Diagramme |
| [Nano-Referenzpinout](../reference/02_PINOUT_NANO.md), [Nano-ESP32-Adapter](pinouts/ADAPTER_NANO_ESP32.md) | Referenz- und Migrationsunterlagen, nicht HW3-Anschlussplan |
| [Projekt-Roadmap](../../Documents/FY_Productivsoftware_Roadmap.md), [GitHub Issues](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues) | Planung, Entscheidungen und offene Arbeit, nicht die Firmware-Schnittstellenspezifikation |

## Kennzeichnung

- **IST / implementiert:** Nur durch aktuellen Code auf diesem Branch belegte Eigenschaften.
- **Getestet:** Nur mit ausdruecklich genanntem Testaufbau, Datum und Testumfang.
- **Offen:** Noch nicht implementiert oder nach Codeaenderung nicht neu verifiziert.
- **Zukunft:** Beabsichtigte Erweiterung ohne Zusage einer vorhandenen Funktion.

### Bekannte Abweichungen historischer Unterlagen

Alte Berichte nennen unter anderem `GET_STATUS 10 = 04 00`, `GET_STATUS 50 = 00`, nicht angeschlossene Sensoren, reine Motor-Mocks, HW2-Pinbelegung oder nicht implementierte reale Referenzierung. Diese Aussagen sind **zeitgebundene Testprotokolle** und fuer den aktuellen HW3-Branch nicht gueltig.

**Neu 01.11.2026:** `GET_STATUS 10` und `50` pruefen die aktuelle PCF8574-Erreichbarkeit auf I2C-Adresse `0x27`. Der Code ist eingecheckt, der HW-Abzieh-/Regressionstest steht noch aus. `SET_MODE` bleibt `NOT_READY`; Web/CAN sind Zukunft.
