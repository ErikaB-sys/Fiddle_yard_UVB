# HTTP-Babelfisch – erste Integration (feature/FY_ESP32_Web)

**Stand: 02.11.2026. Testintegration, noch nicht als Produktivversion freigegeben.**
Der funktionierende FY-Controller in `FY_ESP32/` bleibt die Grundlage.
`FY_ESP32_Web/` bleibt als **unabhängiger Simulationsprüfstand** erhalten.

## Architektur und Startfolge

1. `FY_ESP32/src/main.cpp` setzt sichere Motor-GPIO-Pegel und initialisiert Muninn.
2. Die bestehenden fünf FreeRTOS-Queues werden erzeugt; anschließend wird das Keyboard initialisiert.
3. `HTTP_prepare(&bridge)` erstellt lediglich die HTTP-Diagnosequeue und übernimmt den Kontext. **Kein WLAN-Warten in `setup()`.**
4. Muninn (Core 1) und die bestehende UART-/BabelFish-Task (Huginn, Core 0) starten unverändert.
5. Danach startet `Huginn_startHTTP(&bridge)` einen **eigenen, niedrig priorisierten Core-0-Task**. Er pflegt WLAN, mDNS und `WebServer`, während das lokale Keyboard weiter im Arduino-`loop()` abgefragt wird.

Der HTTP-Task übernimmt **keine** Motorfunktion, greift **nicht** auf STEP/Timer/GPIO zu und entnimmt **keine** Motorantwort aus der UART-Queue. Die aktuelle HTTP-Anbindung gibt daher auch noch **keine Fahrbefehle** an Muninn weiter.

## WLAN lokal einrichten

Die Produktivfirmware ist **auch ohne WLAN-Datei baubar** und bleibt dann ausschließlich lokal bedienbar.

Um HTTP zu aktivieren, `FY_ESP32/include/WiFi_Secrets.example.h` **lokal** zu `FY_ESP32/include/WiFi_Secrets.h` kopieren und SSID/Passwort eintragen. `WiFi_Secrets.h` ist in `FY_ESP32/.gitignore` ausgeschlossen. Alternativ die vorhandenen Zugangsdaten aus dem separaten `FY_ESP32_Web`-Projekt lokal übernehmen; **nicht committen**.

Beim ersten gemeinsamen Test den alten Web-Prüfstand-ESP32 abschalten, damit nicht zwei Geräte denselben Hostnamen `fy-uvb.local` anbieten.

`pio run -d FY_ESP32` (bzw. PlatformIO-Projekt `FY_ESP32/` öffnen), danach erst auf HW3 flashen und testen.

## HTTP-Vertrag – Zwischenstand

| Endpunkt | Verhalten |
| --- | --- |
| `GET /` | Kopie der Weboberfläche, LIVE/LOCAL-Anzeige |
| `GET /api/v1/status` | Echtzeit-Snapshot von Muninn; ohne HTTP-Motorsteuerung |
| `POST /api/v1/command`, `command=SELECT_TRACK&value=1..5` | Gemeinsame **Web-Vorwahl** auf allen Browsern, ausdrücklich **keine Fahrt** |
| `POST /api/v1/command`, `SET_TRACK` / `REFERENCE` / andere | **423 NACK/HTTP_CONTROL_LOCKED** |
| `POST /api/v1/stop` | **423 NACK/HTTP_CONTROL_LOCKED**; örtlicher STOP funktioniert unverändert |
| `GET /api/status` | WLAN-/IP-Diagnose |
| `GET /api/system` | Flash-/Heap-Diagnose |

Die Weboberfläche sperrt FAHRT, REFERENZ und HTTP-STOP sichtbar. **Der Server sperrt diese Befehle zusätzlich selbst** – JavaScript-Sperren allein reichen nicht als Schutz.

### Statusfelder

`simulation=false`, `readonly=true`, `mode=LOCAL`, `state`, `busy`, `referenced`, `target_track`, `track` (nur bei bestätigter Stellung), `selected_track`, `position_steps` und `position` (grafische Gleisskala 1..5).

`position` wird ausschließlich aus einer **gültigen realen Referenzposition** und den gespeicherten Gleispositionen abgeleitet. Wenn die Position nicht sicher darstellbar ist, lautet sie **null**; es wird **keine erfundene BG3-Lage** gemeldet. Der erste Browser-Snapshot kann ohne Referenz also noch keine Bühnenstellung bestätigen. Die Web-Vorwahl bleibt unabhängig davon benutzbar.

`SELECT_TRACK` ist eine reine Web-Vorwahl im RAM. Sie ist **nicht** das bestehende Motorprotokollkommando `CMD_SET_TRACK` (`0x1C`). Der spätere zentrale Command Router muss die Bedienhoheit regeln, bevor Motorbefehle aus HTTP angenommen werden dürfen.

## UART-Diagnose: nur HTTP-Funkverkehr

`HTTP.cpp` legt bei jedem **HTTP-POST-Befehl** eine RX- und eine TX-Diagnose als je eine JSON-Zeile in einer nicht blockierenden Queue ab. `UART::update()` gibt bis zu vier dieser Zeilen pro Durchlauf über `Serial` aus. **Statuspolling erzeugt keinen Trace.** Beispiele:

```json
{"transport":"HTTP","dir":"RX","cmd":"SELECT_TRACK","value":4}
{"transport":"HTTP","dir":"TX","cmd":"SELECT_TRACK","result":"ACK","reason":"ACCEPTED"}
```

Das alte UART-Drahtprotokoll und dessen binäre Antworten bleiben im Code bestehen. Die JSON-Zeilen dienen ausschließlich der **Entwicklungsdiagnose**; ein späterer serieller Protokollclient müsste diese Zusatz-Ausgaben deaktivieren oder korrekt trennen. Bei voller Trace-Queue wird eine Debugzeile verworfen, **niemals** der HTTP-Auftrag aufgehalten.

## Abnahmeschritte für diesen Stand

1. **Clean Build** von `FY_ESP32` mit und ohne lokaler `WiFi_Secrets.h`; Flash/RAM notieren.
2. **POR mit ausgeschaltetem WLAN:** örtliche STOP-Verriegelung und Keyboard-Funktion unverändert; kein WLAN-Starttimeout.
3. **WLAN aktiv:** beide Browser zeigen denselben echten Muninn-Zustand. Fehlende REF darf nicht als bestätigte Gleisstellung erscheinen.
4. **Vorwahl BG1–BG5:** der Fahrgeber synchronisiert sich zwischen Laptop und Handy, ohne einen STEP auszulösen.
5. **Direkter HTTP-POST SET_TRACK / REFERENCE / STOP:** muss NACK/423 liefern; nicht auf JS-Sperre verlassen.
6. **USB-Monitor:** genau ein RX und ein TX für jeden POST; **keine** periodischen GET-Statusmeldungen.
7. **LOCAL-Regression:** STOP, STOP-Freigabe, GO, kurze/lange Referenz, Gleisfahrten und UART-Telegramme nach bestehendem Vertrag prüfen.

**Nächste Stufe, ausdrücklich noch offen:** gemeinsamer Command Router; korrelierte HTTP-ACK/NACK-Antworten von Muninn; LOCAL/FERN/AUTO-Bedienhoheit; priorisierte STOP-Einbindung ohne Queue-Risiken. Siehe [Bedienhoheit/JMRI](../../../FY_ESP32_Web/docs/BEDIENHOHEIT_JMRI.md) und [Systemarchitektur](../SYSTEM_ARCHITECTURE.md).
