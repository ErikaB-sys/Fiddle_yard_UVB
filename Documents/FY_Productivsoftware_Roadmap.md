# FY ESP32 – Roadmap und Funktionsampel

**Stand: 26.10.2026, nach HW3-Fahrtests.** Branch `main`, Projekt `FY_ESP32/`. Stand des zuletzt eingecheckten UART-Bootbanner-Fixes: `b8803bf`. Übergeordnet: [#150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150).

## Nächster Meilenstein – bis spätestens 01.11.2026

**Lokale Bedienung an HW3:** Das vorhandene I²C-Keyboard am ESP32 anschließen und anbinden. Die Tasten sollen die Bühne **manuell links/rechts bewegen, STOP auslösen und eine Referenzfahrt starten**. Die bestehende produktive Fahrsteuerung wird wiederverwendet; kein neuer Motorpfad, keine ISR-Umbauten. Tastatur, PCF8574, LEDs und OLED nur im tatsächlich geprüften Umfang als fertig melden. Hardwareanschluss und Softwareintegration dürfen parallel laufen ([#157](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/157)).

**Parallel am 27.10. – Dokumentation und Kommentarstil:** Code als Wahrheit, Dokumente daran angleichen; veraltete FAKE-REF-/BENCH-Texte gezielt entfernen. Englische Bezeichner/UART-Protokollnamen erhalten, **deutsche knappe Codekommentare und Doxygen-Funktionsverträge**, deutsche Markdown-Dokumente. Doxygen, Graphviz, Doxygen Awesome und spätere GitHub Pages **vorbereiten**, nicht als Fertigstellungsbedingung und vorerst **kein neues GitHub-Issue**. Für Funktionen: Zweck, Parameter und Einheit, Rückgabe/Fehler, wichtige Voraussetzungen/Core-Zuständigkeit; keine offensichtlichen Zeilen nacherzählen. Vor dem späteren Merge dokumentations- und codebezogener Abgleich gegen `main`.

## Ampel für den tatsächlichen Funktionsstand

**🔴 nicht vorhanden · 🟠 geplant/beschlossen · 🟡 implementiert/teilweise geprüft · 🟣 konkrete physische oder Randfallprüfung fehlt · 🟢 im angegebenen Umfang auf HW3 geprüft und dokumentiert.** Die Ampel bezeichnet jeweils *genau den genannten Funktionsumfang*, nicht eine pauschale Gesamtfreigabe.

| Befehl / Bereich | Status | Nachweis vom 26.10. / konkreter Rest |
| --- | --- | --- |
| POR und sichere Freigabe | 🟢 | `STATUS 20 = 50 00 04 00 70`: nach POR STOPPED, Antrieb aus. Elektrischer Reset-/Flash-Failsafe separat offen (#159/#161). |
| `STOP AA 55`, `GO AA 55` | 🟢 | ACK, definierte Entriegelung und Treiberfreigabe. **STOP während Fahrt und anschließende Neu-REF** noch 🟣 (#154/#152). |
| Echte `REFERENCE 00` | 🟢 | Mit gespeicherter Kalibrierung: `REF: VALID pos=1000 end=8334`; anschließend BG3. `STATUS 30 = 30 01 97`. |
| Lange `REFERENCE 01` | 🟡 | Kalibrier-/Persistenzpfad vorhanden; gezielte Wiederhol-/Sensor-/Fehlerfälle noch 🟣 (#155/#158/#159). |
| `SET_POSITION` | 🟢 | Reale Fahrt auf 3200 (`80 0C`); ohne aktuelle REF unzulässig. |
| `MOVE` | 🟢 | `31 32 00` = 50 Schritte bei festem 1/8-Modus, ACK `60 0F 0A`; reale Position 3250 danach. Ungültiger 1/4-Modus `21 …` → NACK `70 24 AB`. Unreferenzierte Inbetriebnahmefahrt und Randgrenzen separat 🟣. |
| `SET_TRACK` | 🟢 | Gleise angefahren; nach freier Absolut-/Relativfahrt erneute Rückfahrt auf gewünschtes Gleis bestätigt. |
| `GET_POSITION`, `STATUS 20/30`, `TRACK 00` | 🟢 | Realer Positions-Snapshot `20 B2 0C 00 00 80` = 3250, Motor IDLE, REF gültig; freie Fahrt meldet `40 00 00 02 88`. GET_TRACK-Bedeutung *während* Fahrt noch offen (#156). |
| Sechsphasiges Motorprofil | 🟡 | Reale Fahrten und 50-Step-POSI-Kurzfahrt geprüft. **ACC1/ACC2/KONST/BRE1/BRE2**, Signalform und STOP während laufender Fahrt gezielt 🟣 prüfen (#153). |
| Realer Lastbetrieb | 🟢 | Fahrtestfolge mit **1.452 g (1,452 kg) Zusatzlast**, gemessen mit Haushaltswaage; keine erkennbaren Schrittverluste bei beobachteter Ausrichtung. Keine unabhängige Schrittzählung oder quantitative Wiederholgenauigkeit behaupten (#153). |
| Motorparameter GET / RESTORE | 🟢 | `SET_PARAM 7E 02 AA 55` ACK; GET bestätigt V_MIN=120, V_POS=120, A1_LEN=100, POSI_MIN=160, KONST_MIN=400. |
| NVM v2: Laden/Status nach POR | 🟢 | `STATUS 60 = 61 00 00 07 BB` vor und nach POR; alle drei Segmente gültig und fehlerfrei; kurze REF mit gespeicherten Kalibrierdaten möglich. |
| NVM SET/SAVE/Recovery | 🟣 | **Geänderten Parameter** speichern, POR, erneut auslesen; CRC-/Versions-/Invalidierungs- und STOP-bei-SAVE-Fälle offen (#158). |
| UART-Parser, ACK/NACK, Parameterabfragen | 🟡 | Real verwendete Befehle/Antworten bestätigt. Robustheit, Systemstatus `10`, HELP/Firmware und Antwort-/BUSY-Korrelation noch offen (#156/#152). |
| Zusätzlicher Fahrbefehl während BUSY | 🟣 | Keine wartende Fahrbefehlsliste; zweiter Befehl während laufender Bewegung gezielt abweisen und prüfen (#152). |
| I²C-Keyboard / PCF8574 / OLED | 🟠 | Physischer Anschluss an HW3 und ESP32-Integration ab 27.10.; REF, STOP, links/rechts zuerst (#157). |
| Teach-in / Web / JMRI / Beleuchtung | 🟠 | Nach erstem Keyboard- und Software-Abschluss getrennt: #167/#166/#160/#165. |

## Bereits als geschlossener Funktionsnachweis betrachtete Fahrkette

1. `REFERENCE 00` mit gültiger gespeicherter Kalibrierung; `pos=1000`, Gegenendlage `8334`, anschließende BG3-Fahrt.
2. `SET_POSITION 80 0C` auf Koordinate **3200**.
3. `MOVE 31 32 00` = **+50** → `POSITION` bestätigt **3250**, aktuelle REF weiter gültig.
4. Anschließend `SET_TRACK` → Bühne fährt wieder zum passenden Gleis.
5. Alle diese Realfahrten unter **1,452 kg Zusatzlast**, keine sicht-/beobachtbaren Schrittverluste.

Dies ist **keine Gesamt-Abnahme** für Sicherheitsverhalten, Sensorfehler, Pulse unter Last oder alle Profilparameter.

## Nächste Reihenfolge

1. **27.10.: Dokumentation konsolidieren**, Sprachkonvention umsetzen, kurze deutsche Doxygen-Funktionsverträge nachführen, Vorbereitung Doxygen/Graphviz/Awesome. Dokumente aus Code und realen Testergebnissen aktualisieren; keine neue Baustelle daraus machen ([#12](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/12)).
2. **27.10.–01.11.: Keyboard an HW3 anschließen und integrieren.** Vorhandenen lokalen Tastenvertrag nutzen, UART- und Keyboard-Aufträge durch denselben Motor-Befehlsweg führen; STOP-Vorrang erhalten (#157).
3. **Gezielte verbleibende HW3-Funktionstests:** komplette Rampen über längere Strecke, STOP während Bewegung → REF ungültig → Entriegeln/GO/Neu-REF, zweiter Fahrbefehl bei BUSY, ungewöhnliche kurze/unreferenzierte MOVE-Fahrt (#153/#154/#152).
4. **NVM vervollständigen:** geänderte Werte SET→SAVE→POR→GET und verbleibende Fehler-/CRC-/Wiederherstellungsfälle (#158). Motor-NVM wurde auf Version 2 (34 Bytes) erweitert; alte Version benötigt explizites Restore.
5. **UART/Diagnose abgleichen:** tatsächlich implementierten Befehlsumfang einschließlich `MOVE`/`SET_POSITION`/`SET_TRACK`, Status und NVM in [UART-Spickzettel](../FY_ESP32/src/UART_Command_Spickzettel.md) berichtigen; offene GET_TRACK-/BUSY-/Systemstatus-Verträge klären (#156/#152).
6. **Vor erstem Merge nach `main`:** funktionalen Code und Dokumentation bereinigen, MISRA/Cppcheck-Befunde prüfen, Hauptzweigstand vergleichen, stabilen HW3-Stand sichern. Größeres Benennungs-/Mock-Cleanup gemäß #169 **erst nach** dem ersten Merge.

## Verbindliche Fahrregeln

- `MOVE`: relativ, Richtung und Schrittzahl; begrenzte Inbetriebnahmefahrt auch ohne REF. Bei gültiger REF Ziel im kalibrierten Fahrweg prüfen. Fest eingestellter A4988-Mikroschrittmodus: **1/8**, Byte0 `30` bzw. `31`; für aktuelle Richtung `31` beim geprüften Beispiel.
- `SET_POSITION`: absolute Koordinate, **nur mit aktuell gültiger realer REF** und innerhalb der Endlagen; kein POR-Mittenanker.
- `SET_TRACK`: Gleis 1–5 über gespeicherte Position; gültige Kalibrierung, Gleisdaten und aktuelle REF erforderlich.
- `STOP`: Antrieb aus, Auftrag verwerfen, aktuelle REF ungültig, keine Wiederaufnahme. `STOP AA 55` entriegelt, `GO AA 55` aktiviert ohne eigene Fahrt, danach neu referenzieren.
- **BUSY:** zweiter Fahrbefehl nicht auf Vorrat sammeln; bestehende PRIORITY-STOP-Queue erhalten.
- Keine spontanen DONE-/ALIVE-Meldungen, ACK ist Annahme, nicht Fahrabschluss. Abschluss via STATUS/POSITION/TRACK prüfen.

## Offene Themen und Eigentümer

| Issue | Nächste wirklich offene Punkte |
| --- | --- |
| [#150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150) HEAD | Gesamtfortschritt, erste stabile ESP32-Fassung und Merge |
| [#152](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/152) Router/Queues | BUSY ohne Nachstart, STOP und Antworttransport |
| [#153](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/153) Motorprofil | vollständige Rampen, Puls-/Stop-Prüfung, parallele Kommunikation |
| [#154](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/154) Fahrbefehle | STOP-/GO-/Neu-REF-Folge, Randgrenzen und Abschluss |
| [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155) Sensorik | Referenz-/Endlagenfehler und mechanische Sicherheit |
| [#156](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/156) UART | Status/HELP/Firmware, offene Protokoll- und Robustheitstests |
| [#157](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/157) Keyboard | Vier Tasten, PCF8574, LED/OLED, lokale Bedienung |
| [#158](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/158) NVM | SAVE→POR→GET, CRC/Fehler, Parameterausbau |
| [#159](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/159) Abnahme | benannte Randfälle, Bühnen-/Reset-Abnahme, Merge-Gate |
| [#163](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/163) Qualität | Cppcheck/MISRA und Dokumentationsqualität |
| [#169](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/169) Cleanup | ausdrücklich nach erstem Merge |

**Dokumentationsquellen:** [UART-Spickzettel](../FY_ESP32/src/UART_Command_Spickzettel.md) · [Motor](../FY_ESP32/docs/Motor/README.md) · [NVM v2](../FY_ESP32/docs/nvm/README.md) · [Dokumentationsregeln](../FY_ESP32/docs/DOCUMENTATION_RULES.md). Historische Fake-REF-/Bench-Angaben nicht als aktuellen HW3-Stand darstellen.
