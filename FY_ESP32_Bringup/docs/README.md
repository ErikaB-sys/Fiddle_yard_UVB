# FY ESP32 – hier beginnen

**Stand: 24.10.2026 · Branch `feature/FY_ESP32_Port`.** Diese Seite ist der Einstieg in unsere gemeinsame Konzeptprüfung. Wir gehen von den Bedienfällen zum System, dann zu Zuständen und Befehlen. Erst danach leiten wir neue Codearbeit ab.

## 1. Was soll die Schiebebühne für uns leisten?

Die Bühne soll auf Anforderung ein Gleis oder eine Position anfahren, sich lokal bedienen lassen und auf STOP immer reagieren. Als Bedienwege genügen **I²C-Keyboard, später Weboberfläche und JMRI**. UART dient der seriellen Bedienung, Entwicklung und Anbindung. Ein zusätzlicher Keyboard-Nano entfällt.

| Bedienfall | Fachlicher Zweck |
| --- | --- |
| Inbetriebnahme / Service | Die Bühne eine begrenzte Strecke in eine Richtung bewegen, auch solange noch kein absoluter Lagebezug vorhanden ist. |
| Kalibrieren | Den Bewegungsraum und nötige Referenz-/Gleiswerte mit der vorhandenen Sensorik erfassen. |
| Betriebsreferenz herstellen | Mit gültigen Kalibrierdaten durch mindestens kurze Referenzfahrt den aktuellen Lagebezug herstellen. |
| Normal fahren | Ein Gleis oder eine absolute Position innerhalb des bekannten Bewegungsraums anfahren. |
| Anhalten | STOP verwirft den Job, deaktiviert den Antrieb und entwertet die aktuelle Referenz. Entriegeln/GO nimmt keinen Job wieder auf. |

Kalibrierdaten bleiben nach STOP erhalten. Vor normaler Positions-/Gleisfahrt wird neu mindestens kurz referenziert. Kalibrierung und aktuelle Referenzgültigkeit sind verschiedene Informationen.

## 2. Verbindliche Befehlsaufteilung

**Beschlossen am 24.10.2026:**

| Befehl | Zweck und Voraussetzungen |
| --- | --- |
| MOVE | Relativ eine begrenzte Schrittzahl in eine Richtung fahren; auch für Inbetriebnahme ohne gültige REF. Prepare prüft die jeweils geltenden Betriebs- und Grenzbedingungen. |
| SET_POSITION | Absolute Position im kalibrierten Bewegungsraum anfahren; nur mit gültiger REF. |
| SET_TRACK | Gespeicherte Gleisposition 1..5 anfahren; nur kalibriert und aktuell referenziert. |

Separate LEFT-/RIGHT-Kommandos entfallen; lokale Richtungstasten liefern entsprechende MOVE-Aufträge.

**SET_POSITION ohne REF und ein dafür benötigter virtueller POR-Mittenanker entfallen.** Die begrenzte relative Inbetriebnahmefahrt ist durch MOVE abgedeckt. Der vorhandene geprüfte Positions-/Profilpfad bleibt für referenzierte Positions- und Gleisfahrten erhalten.

Verbindlicher Detailvertrag: [#154](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/154). Die heutige FAKE-Referenz ist ein gekennzeichneter Testpfad; die reale Referenz-/Kalibrieranbindung bleibt eine konkrete Restarbeit.

## 3. Unser Lesepfad – nur den nächsten Schritt öffnen

| Schritt | Dokument | Was klären wir dort? |
| --- | --- | --- |
| 1 | **Diese Seite** | Bedienfälle und die Zuordnung relativ/absolut. Hier beginnen wir gemeinsam. |
| 2 | [Systemübersicht](System/README.md) | Aufgaben von Huginn/Muninn, gemeinsamer Befehlsweg und Zuständigkeiten. |
| 3 | [Motorverhalten und Zustände](Motor/README.md) | Kalibrierung, Referenz, Prepare, Auftragsende und STOP/GO. |
| 4 | [UART-/Schnittstellenübersicht](UART/README.md) | Wie die vereinbarten Bedienfälle als Auftrag/Antwort übergeben werden. |
| 5 | [Spickzettel](../src/UART_Command_Spickzettel.md) und [kurze Roadmap](../../Documents/FY_Productivsoftware_Roadmap.md) | Erst jetzt: vorhandene Implementierung, konkrete Telegramme und verbleibende kleine Arbeiten. |

**Arbeitsweise:** Einen Bedienfall prüfen und entscheiden, dann Beschreibung/Diagramm und Issue angleichen. Erst danach den nötigen Code anpassen, kurz prüfen und dokumentieren. Wir beginnen nicht mit allen Issues und Testlisten gleichzeitig.

## 4. Wo steht welche Information?

- **Fachvertrag:** verlinkte Modulbeschreibung und zuständiges Issue; [#150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150) ist der Arbeitsüberblick.
- **Iststand / Bediennachschlagehilfe:** Spickzettel; geprüfte Funktionen bleiben im benannten Umfang grün.
- **Offene Abweichungen:** [Review-Liste](VERIFICATION_GAPS.md), gezielt beim betroffenen Thema lesen; keine zusätzliche Lesepflicht vor dem Einstieg.
- **Historie / Nachweise:** `docs/reports/`, Modul-`archive/`, Nano-Referenzen und Git-Historie. Sie sind keine zweite aktuelle Spezifikation.

<details>
<summary>Weitere Modul- und Werkzeuglinks – nur bei Bedarf</summary>

[Keyboard](Keyboard/README.md) · [Switches](Switches/README.md) · [BabelFish](BabelFish/README.md) · [Error](Error/README.md) · [Pinouts](pinouts/README.md) · [Architekturdetails](SYSTEM_ARCHITECTURE.md)

[Dokumentationsregeln](DOCUMENTATION_RULES.md) · [Coding Rules](CODING_RULES.md) · [Modulvorlage](MODULE_TEMPLATE.md) · [Nano-Referenzen](../reference/)

</details>
