# FY ESP32 – kurze Roadmap

**Stand: 24.10.2026.** Aktiver Branch `feature/FY_ESP32_Port`, Projekt `FY_ESP32_Bringup/`. C++-Iststand dieses Abgleichs: `342cdd35`. Head: [#150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150).

## Lieferziel für Sonntagabend, 25.10.2026

**Ziel:** HW3-Firmware mit benutzbarem seriellem Befehlsumfang, Parameterliste einschließlich Speichern/Laden und implementiertem I²C-Keyboard. Die Bühne soll damit bewegt werden können. Physischer Keyboard-Anschluss hängt von Eris Bus-/Keyboard-Verdrahtung ab; Code und tatsächliche Hardwareerprobung getrennt ausweisen. Das ist das angestrebte Ergebnis, keine bereits erreichte Funktion oder Termingarantie.

| Lieferung | Woran wir sie erkennen |
| --- | --- |
| Parameterliste und Speicherung | Je Parameter ID, Name, Einheit, Typ/Wertebereich, Default und RAM/persistent festgehalten; SET_PARAM übernimmt gültige Werte, SAVE speichert ausdrücklich, Start lädt gültige Daten oder sichere Defaults. Aktuelle REF wird durch Laden nicht gültig. |
| Vereinbarte serielle Befehle | STOP/GO, MOVE, SET_POSITION, SET_TRACK, kurze/lange REFERENCE, SET_PARAM/SAVE, SET_MODE sowie Abfragen/HELP/Firmware führen ihre vereinbarte Funktion aus oder lehnen fachlich unzulässige Aufträge nachvollziehbar ab. Ein NOT_READY-Platzhalter allein ist kein Funktionsabschluss. LEFT/RIGHT und unbeauftragte DONE/ALIVE gehören nicht dazu. |
| I²C-Keyboard im Code | Vorhandene REF-/STOP-/Richtungsbedienung, Entprellung, Startverriegelung, LEDs/OLED und gemeinsamer Befehls-/Statusweg implementiert; fehlendes Gerät blockiert die Motorsteuerung nicht. Physisch angeschlossen/geprüft nur nach tatsächlicher Beobachtung angeben. |
| Bewegung mit HW3 | Bestehenden Profil-/STEP-Pfad weiterverwenden; reale Sensor-/REF-/Kalibrieranbindung ergänzen. FAKE-REF belegt weiterhin die benannte Softwarefunktion, ersetzt jedoch nicht die echte Referenzfunktion. |

**Reihenfolge in kleinen Schritten:** (1) ersten Parameterumfang/IDs und Restentscheidungen unten klären → (2) STOP-/Antwortpfad #152 gezielt beurteilen; nur belegte Funktionsfehler korrigieren → (3) Parameter/SAVE/Laden #158 → (4) reale Sensorik/Referenz und Fahrbedingungen #155/#154 → (5) UART-Reste und vereinbarte Stillstandsumschaltung #156/#157 → (6) Keyboard #157 integrieren. Nach jedem Schritt kurz bauen/praktisch prüfen und den Spickzettel nachführen. Bestehendes HW_TEST-Verhalten übernehmen; keine neue Framework- oder Nano-Paritätsrunde.

**Größter Terminrest:** Echte Referenz/Sensorik und Parameter/SAVE sind noch keine ausführbaren ESP32-Funktionen; auch Keyboard-Code fehlt. Bei Resten am Abend exakt Funktion/Blocker nennen, keine Platzhalter als fertig markieren. Die Verdrahtung kann parallel erfolgen.

## Schlanke Prüfung und Dokumentation

- Keine vollständige Unit-/White-/Black-Box-Suite, Coverage-Vorgabe oder formale Automotive-SPICE-Einführung als Voraussetzung für dieses Lieferziel. Keine zweite Implementierungsrunde nur für ein Testgerüst.
- Je geändertem Funktionspfad ein kurzer dokumentierter Durchlauf plus naheliegende Ablehnungen: z. B. ungültiger Parameter/Änderung während Fahrt, SAVE–Neustart–Laden, STOP–Entriegeln–GO ohne Wiederaufnahme, Absolutfahrt ohne REF abgelehnt. Vorhandene bestandene Funktionsnachweise wiederverwenden, unveränderte Pfade nicht pauschal neu aufrollen.
- Gewünschter Standard: Build mit Compilerwarnungen und automatisch mitlaufender Cppcheck-/MISRA-orientierter Analyse des eigenen Codes (#163). Heute existiert nur der separate Aufruf; dessen Konfiguration/Einbindung ist noch zu reparieren. Befunde und Werkzeugfehler sichtbar ausgeben; der genaue Fehler-/Baseline-Umgang wird bei der Einrichtung festgelegt. Keine vollständige MISRA-Konformität behaupten.
- **Sprachvorschlag:** aktive Erklärtexte und Doxygen auf Deutsch, Befehle/Bezeichner weiter wie im Code. Neue/geänderte APIs dokumentieren Zweck, Parameter/Einheiten, Voraussetzungen, Rückgabe/Fehler und ggf. Core-/Task-Zuständigkeit; keine flächige Altcode-Kommentierung als Zusatzblock.
- Doxygen-Erzeugung, danach optional [Doxygen Awesome](https://github.com/jothepro/doxygen-awesome-css), sind nachrangige Werkzeugpflege; keine Voraussetzung für Fahrbetrieb.
- **Hardwareerprobungen:** konkrete offene Fälle bleiben in #153/#155/#159/#161 erhalten. Ihre Priorisierung für morgen gegenüber späterer Bühnen-/Gesamterprobung ist noch abzustimmen.

**Grün = Softwarefunktion im benannten Umfang geprüft und dokumentiert.** Die spätere Kalibrierung/Prüfung auf der Bühne bleibt ein eigener Auftrag; das Ziel einer realen Referenzfunktion wird dadurch nicht gestrichen. Einstieg für den fachlichen Faden: [Hier beginnen](../FY_ESP32_Bringup/docs/README.md).

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
| [#152](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/152) STOP / Antworten | Eigene STOP-Queue vorhanden; Pending-voll ist im heutigen gültigen UART-Pfad kein belegter Fehler. Stockenden Antworttransport und spätere zusätzliche Quellen gezielt beurteilen; keine neue Architektur allein aus hypothetischer Sättigung. |
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
