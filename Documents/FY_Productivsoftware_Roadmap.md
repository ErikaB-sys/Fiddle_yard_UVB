# FY ESP32 – kurze Roadmap

**Stand: 24.10.2026.** Aktiver Branch `feature/FY_ESP32_Port`, Projekt `FY_ESP32_Bringup/`. C++-Iststand dieses Abgleichs: `342cdd35`. Head: [#150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150).

## Ziel für die nächste Arbeitsrunde am 25.10.

**Zuerst Konzepte top-down prüfen:** [Hier beginnen](../FY_ESP32_Bringup/docs/README.md). Danach den kleinsten sinnvollen Befehlsumfang ableiten. MOVE mit Richtungsangabe ist entschieden; LEFT/RIGHT entfallen. SET_POSITION und SET_TRACK benötigen Kalibrierung/gültige REF; SET_POSITION ohne REF/POR-Mitte entfällt.

Den vereinbarten ESP32-Befehlsumfang über die serielle Schnittstelle benutzbar machen: offene Entscheidungen nacheinander klären, einen kleinen Funktionsschritt umsetzen, kurz prüfen und dokumentieren. Heute erfolgen ausschließlich Issue-/Dokumentationsänderungen.

**Grün = Softwarefunktion im benannten Umfang geprüft und dokumentiert.** Die spätere Kalibrierung und Prüfung auf der Bühne bleibt ein eigener Auftrag. Kein Nachbau alter Nano-Funktionsnamen und keine pauschale vollständige Testsuite als Voraussetzung.

## Fachlich festgelegt

| Fall | Verbindliches Verhalten |
| --- | --- |
| MOVE | Relative begrenzte Fahrt mit Richtung/Schrittzahl, auch zur Inbetriebnahme ohne REF. |
| SET_POSITION ohne gültige REF | Nicht zulässig. Die vorherige POR-Mittenvariante entfällt. |
| SET_POSITION mit gültiger REF | Ziel innerhalb des durch Kalibrierung erfassten Bewegungsraums. |
| Kurze REF | Kann nur mit gültigen Kalibrierdaten erfolgreich sein; stellt den aktuellen Lagebezug wieder her. |
| SET_TRACK | Gleisnummer 1..5; nur kalibriert UND aktuell referenziert. Derselbe Positionierungs-/Profilpfad. |
| Gültiger Fahrbefehl | Prepare prüft alle Bedingungen; bei OK wird ausgeführt. Kein zusätzliches GO je Fahrjob. |
| STOP | Auftrag und vorgemerkte Fahrjobs verwerfen, Antrieb deaktivieren, aktuelle REF ungültig. Kein Pause/Resume. |
| STOP entriegeln → GO | Entriegelt und aktiviert nur den Antrieb. Kein alter Job und keine Referenz werden wiederhergestellt; für normalen Fahrbetrieb neu mindestens kurz referenzieren. Kalibrierdaten bleiben erhalten. |

Der heutige Positions-Testpfad verwendet explizite FAKE-REF mit Start 1000. Die geprüfte Positionsfunktion bleibt fertig. Ein zusätzlicher unreferenzierter SET_POSITION-Pfad wird nicht geplant; MOVE deckt die relative Inbetriebnahmefahrt ab.

## Bereits erledigt

- [x] ESP32-/PlatformIO-Grundaufbau, Huginn/Core 0 und Muninn/Core 1; erfolgreiche Builds dokumentiert.
- [x] UART/BabelFish, binäre ACK/NACK-/Positions-/Motorantworten und pollender Bewegungsabschluss.
- [x] MOVE, sechsgliedrige Profilfahrten und vorhandene SET_POSITION-Funktion am Tisch geprüft.
- [x] FAKE-REF, SET_TRACK 01/03/05 und GET_TRACK während/nach Fahrt am 23.10. dokumentiert; BG1–BG5-Tabelle zusätzlich an den tatsächlichen Headern geprüft.
- [x] Issue-Bereinigung: 125 → 17 offen, 15 abgeschlossen, 93 abgelöst/zusammengeführt. Alte Texte und Kommentare bleiben erhalten.

## Wirklich offene Entscheidungen

| Reihenfolge | Frage | Vorschlag / Zuständigkeit |
| --- | --- | --- |
| 1 | GET_TRACK während Fahrt: letzte Bestätigung oder 0? | Code setzt 0, ältere Definition behält die letzte Bestätigung. [#156](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/156) |
| 2 | Erster Parameterumfang und IDs einschließlich SAVE? | Konkrete IDs/Wertebereiche schrittweise festlegen; RAM/SAVE-Semantik steht bereits. [#158](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/158) |
| 3 | Wer fordert LOCAL/FERN an und wer gibt frei? | Nur im Stillstand; Netzverbindung allein erteilt keine Bedienhoheit. [#157](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/157)/[#166](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/166) |

## Offene Umsetzung – je einen kleinen Punkt abschließen

| Auftrag | Konkreter Rest |
| --- | --- |
| [#154](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/154) Fahrt | Beschlossene Trennung MOVE relativ / SET_POSITION und SET_TRACK nur mit gültiger REF in die produktiven Bedingungen übernehmen. |
| [#152](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/152) STOP / Antworten | STOP darf nicht an vollen gemeinsamen Pending-Slots scheitern; `emit(...portMAX_DELAY)` darf Muninn nicht unbefristet blockieren. |
| [#156](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/156) UART | GET_TRACK-Vertrag, echte System-/Sensor-/Keyboardwerte statt Platzhalter; HELP-Link korrigieren. |
| [#158](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/158) Parameter / Diagnose | SET_PARAM tatsächlich übernehmen, SAVE/Laden, Git-Identität und zentrale Fehlerweitergabe. |
| [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155) REF / Sensorik | HW_TEST-Referenzablauf, Kalibrierraum und direkte Endlagenreaktion in Muninn übernehmen. |
| [#157](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/157) I²C-Keyboard | Bestehende HW_TEST-Bedienung in ein gekapseltes ESP32-Modul übernehmen; Taster/LEDs/OLED, Startverriegelung und Modusfreigabe. |
| [#153](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/153)/[#159](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/159) Nachweise | Die benannten offenen STEP-/STOP-/Lastfälle; später reale Ausrichtung und Gesamtabnahme. |
| [#163](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/163) Werkzeugpflege | Unvollständige Cppcheck-Konfiguration und separaten PlatformIO-Prüfaufruf verbessern. |

**Keyboard-Hardware:** I²C-Löten kann parallel erfolgen. Die vorhandene Logik ist eine nutzbare Quelle; ESP32-Kapselung und gemeinsame Befehls-/Statusübergabe bleiben nötig. Mehr Speicher erlaubt eine übersichtlichere Anzeige, ohne neue Bedienfunktionen zu verlangen.

## Später / getrennt

- Echte Bühnenwerte, Ausrichtung, Endlagen-/Start-/Reset-Prüfung und main-Freigabe: [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155)/[#159](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/159)/[#161](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/161). Die Prüfung auf der Bühne macht erledigte unveränderte Softwarefunktion nicht erneut zur Dauerbaustelle.
- Bedienumfang: **lokales I²C-Keyboard, Weboberfläche und JMRI**. UART bleibt Entwicklungs-/Steuerschnittstelle. Ein separater Keyboard-Nano über UART entfällt.
- Web zuerst als getrennte ESP32-Komponente: Netzwerk/einstellungen, mDNS, HTTP → Web-Grundstruktur → Übergabe → Bedienhoheit. [#166](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/166); JMRI [#160](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/160).
- Mechanik [#97](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/97), Teach-in [#167](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/167) und optionale Gebäudegestaltung [#165](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/165) bleiben erreichbar; kein paralleler Pflichtblock für morgen.

**Quellen:** [UART-Spickzettel](../FY_ESP32_Bringup/src/UART_Command_Spickzettel.md) · [Motorvertrag und Zustände](../FY_ESP32_Bringup/docs/Motor/README.md) · [Dokumentationsregeln](../FY_ESP32_Bringup/docs/DOCUMENTATION_RULES.md) · [Testprotokoll](../FY_ESP32_Bringup/docs/MOTOR_TIMER_BENCH.md). Frühere Roadmap-Stände bleiben in der Git-Historie; Nano ist Referenz, kein aktiver Fertigstellungsplan.
