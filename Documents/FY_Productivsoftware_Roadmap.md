# FY_ESP32 – Roadmap / Liste offener Punkte (LOP)

**Stand: 27.10.2026.** Produktives Projekt: [FY_ESP32](../FY_ESP32/README.md) auf `main`; ESP32-Port über [PR #170](https://github.com/ErikaB-sys/Fiddle_yard_UVB/pull/170) integriert. Der erste HW3-Funktionsmeilenstein ist erreicht, aber der Release-Tag `FY_ESP_2.0.0` ist **noch nicht gesetzt**. Releasekennung in der Firmware: `2.0.0`.

## Ampel

**🟢** im genannten Umfang implementiert und grob HW3-geprüft · **🟡** funktioniert teilweise / konkreter Code-Rest offen · **🟣** weitergehende Prüfung im Testing-Issue · **🟠** geplant für später · **⚪** kein Release-Umfang.

| Funktion | Stand 2.0.0 | Nachweis / Rest |
|---|---|---|
| ESP32-Port / FreeRTOS / STEP | 🟢 | HW3-Fahrten und Merge nach main |
| Keyboard PCF8574 / OLED | 🟢 | Lokalbedienung, REF-Auswahl, BG1–5 und STOP physisch getestet |
| POR STOP / Freigabe | 🟢 | STOP-Latch; bewusstes LINKS-halten-dann-GO |
| STOP UART + Keyboard während Fahrt | 🟢 | Sofortiger Stillstand und Verriegelung physisch bestätigt; Freigabe erneut getestet |
| SHORT-REF und automatisch BG3 | 🟢 | Kalibrierte echte Referenzfahrt bestätigt |
| LONG-REF / Kalibrierung | 🟡 / 🟣 | Implementiert, gezielte Persistenz-/Sensorfehlerprüfung in #171 |
| SET_TRACK / SET_POSITION / MOVE | 🟢 | Reale Fahrten erfolgreich; Grenzfälle in #171 |
| Motorprofile / Richtungs-LEDs | 🟢 / 🟣 | Fahrfunktion verifiziert; quantitative Puls-/Last-/Profilprüfungen in #171 |
| UART / Status / ACK/NACK | 🟢 / 🟡 | Hauptpfad verifiziert; STATUS 10/50 und GET_TRACK-Verträge gezielt prüfen #156 |
| NVM-Segmente / Status | 🟢 / 🟣 | Persistenzgrundfunktion; SAVE/POR/CRC/Recovery unter #171 |
| Pinout ohne SPI | 🟢 | HW3 dokumentiert, nicht belegte Pins NC |
| MISRA / Cppcheck | 🟡 | Eigener Auftrag #163, kein pauschaler Qualifikationsnachweis |
| LOCAL/REMOTE-Steuerhoheit | 🟠 | SET_MODE liefert NOT_READY; #166 |
| Web / JMRI / CAN / Teach-in / Hot-Plug | 🟠 | Spätere Ausbaustufen #160/#166/#167, nicht 2.0.0 |

## Vor Release FY_ESP_2.0.0

- [x] ESP32-Zweig nach main integrieren; alten Stand als Backup sichern.
- [x] Verzeichnis in `FY_ESP32/` umbenennen und Clean Build unter neuem Pfad bestätigen.
- [x] Firmwarekennung auf `2.0.0` ändern; SPI-Reservierungen aus HW_Config entfernen.
- [x] Aktive Dokumentation von Historie trennen: UART-Vertrag, Keyboard, Pinout, Modul-READMEs, Mermaid-Diagramme und geführter Lesepfad.
- [ ] **Auf Entwicklerrechner: Git Pull → Clean Build → firmware.bin und firmware.elf** (nach den letzten Code-/Versionsänderungen).
- [ ] Kurzer Start-, Keyboard-, STOP- und Referenz-/Gleisfahrt-Regressionstest mit der neuen Firmware.
- [ ] Prüfstand/Commit dokumentieren und den Git-Tag `FY_ESP_2.0.0` erst danach setzen.
- [ ] Optional Release-Dateien (`firmware.bin`, `firmware.elf`) an GitHub-Release anhängen.

## Coding und Testing klar trennen

- [#171 – Testing](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/171) sammelt offene Randfall-, Langzeit-, Sensor- und Regressionstests. Coding-Issues dürfen bei fertiger Implementierung und grobem Funktionstest abgeschlossen werden.
- [#163 – MISRA / Cppcheck](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/163) bleibt **separat** offen.
- [#169 – Cleanup](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/169) bleibt **separat** offen: Mock-/Altbenennungen, strukturelles Codecleanup.
- [#156 – UART](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/156) bleibt bei tatsächlichen STATUS-/Protokolllücken ein Coding-Issue.
- [#152 – Router](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/152) bleibt bei tatsächlichem fehlendem BUSY-Interlock ein Coding-Issue, nicht nur wegen noch offener Stresstests.
- [#158 – NVM](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/158) bleibt für nachweislich noch fehlende Funktionalität; Tests wandern nach #171.
- [#150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150), [#153](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/153), [#154](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/154), [#157](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/157) sind Kandidaten für **Abschluss im dokumentierten Umfang**.

## Dokumentation

[Hier beginnen](../FY_ESP32/docs/README.md) · [UART-Gegenstellenvertrag](../FY_ESP32/src/UART_Command_Spickzettel.md) · [Keyboard](../FY_ESP32/docs/Keyboard/README.md) · [HW3-Pinout](../FY_ESP32/docs/pinouts/01_PINOUT_ESP32.md) · [Motor](../FY_ESP32/docs/Motor/README.md) · [NVM](../FY_ESP32/docs/nvm/README.md).

**Historie:** Frühere Nano- und HW_TEST-Entwicklungsstände, Tests und Entscheidungen sind in den jeweiligen Git-Commits, Issues, Backup-Branches und ggf. `archive/` erhalten. Sie gehören nicht in die aktive Funktionsspezifikation.
