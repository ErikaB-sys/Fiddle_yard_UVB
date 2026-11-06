# HTTP-Babelfisch – Stand Web 3.0.0 / Netzwerkkonfiguration in Entwicklung

> **Experimenteller Branch:** `feature/FY_ESP32_Network_Config` (Issue #188).
> **Erster HW3-Test (06.11.2026):** Clean Build mit RAM 14,8 % und
> Flash 70,3 %, Initialeinrichtung sowie NVS-Erhalt ueber POR bestaetigt.
> Der nachfolgende direkte WLAN-Wechsel ist neu und noch nicht getestet.
> Erst nach Regressionstests nach `main` mergen.

## Netzwerk-Konfiguration ohne einkompilierte Zugangsdaten (neu)

- **Keine `WiFi_Secrets.h` mehr erforderlich.** `httpTask` startet auch bei
  fehlenden Zugangsdaten und startet dann ein WPA2-gesichertes Setup-WLAN.
- `/network` ist von Bedienpult und Diagnose verlinkt. **Normalfall:**
  Im bereits verbundenen privaten Heim-WLAN neue SSID und Passwort direkt
  eingeben und `Verbindung testen und uebernehmen` druecken (Buehne steht).
  Kein Wechsel ins Setup-WLAN notwendig.
- Erstinstallation und Recovery laufen weiterhin ueber den WPA2-Setup-AP.
  Eine neue SSID kann den bestehenden Browser-Zugriff unterbrechen:
  Geraet ins neue WLAN wechseln und `http://fy-uvb.local/network` neu
  oeffnen; dort neue IP und Erfolg anzeigen. Die IP steht immer auch im
  seriellen Monitor. Bei Fehlschlag versucht der FY das alte WLAN wieder.
- Setup-SSID, zufaellig erzeugtes Setup-Passwort und die URL
  `http://192.168.4.1/network` erscheinen am seriellen Monitor (115200 Baud).
  Das echte WLAN-Passwort erscheint weder im UART-Log noch in einer Antwort.
- Bei bestehender Heimnetzverbindung kann das Setup-WLAN ueber den Button
  **Einrichtungs-WLAN aktivieren** geoeffnet werden, ausschliesslich bei
  stillstehender Buehne. Ein aktiver Setup-AP ermoeglicht keine FY-Fahrbefehle.
  STOP bleibt als Prioritaetsbefehl verfuegbar.
- Neue Zugangsdaten werden zunaechst **getestet** und nur nach erfolgreichem
  STA-Connect und im Stillstand in einem eigenen NVS-Namespace
  `fy_wifi` gespeichert (CRC-gesicherter, atomarer Record); die bestehenden
  Motor-, Kalibrier- und Gleis-Records im Namespace `fy_uvb` bleiben unberuehrt.
- Bei gescheitertem Test wird das alte WLAN wieder verwendet; ohne
  funktionierende Heimnetzverbindung steht der Setup-AP als Recovery bereit.
- Setup-AP wird nach erfolgreicher Speicherung mit kurzer Verzoegerung
  geschlossen; unbenutzter AP wird bei bestehender WLAN-Verbindung automatisch
  nach einer Frist geschlossen.
- **Sicherheitsmodell:** direkte Netzwerkverwaltung im *vertrauenswuerdigen*
  Modellbahn-/Heim-LAN. HTTP ist weder Ende-zu-Ende verschluesselt noch durch
  eigenes Administrator-Passwort geschuetzt. Die POST-Routen erfordern einen
  per Boot zufaellig erzeugten, ueber die lokale Netzwerkseite auslesbaren
  Anfragetoken gegen browserseitige Cross-Site-POSTs. Das ist **keine**
  Administrator-Authentifizierung und schuetzt nicht gegen Teilnehmer, die
  bereits Zugriff aufs LAN haben. Ausbau bei Bedarf in #187.
- **Oeffentliche Releases:** Ausschliesslich generische Firmware erzeugen;
  keine persoenlichen SSIDs/Passwoerter im Binary veroeffentlichen.

## HW3-Abnahme vor Merge

1. Neues Geraet ohne Daten: Setup-AP, zufaelliges Passwort, Netzwerkseite.
2. Gueltige Zugangsdaten: Verbindung, IP ueber UART, Speichern im NVS;
   Neustart/POR laedt Daten ohne Neukompilieren (**06.11. bereits getestet**).
3. Vom bestehenden Heimnetz aus ueber /network neues WLAN setzen: alter
   Browserzugriff kann abbrechen; auf neuem WLAN IP/Erfolgsstatus ablesen.
4. Falsches Passwort: Abbruch und Wiederherstellung der bisherigen Verbindung,
   Setup-AP bleibt erreichbar.
5. Netzwerkausfall/Routerwechsel: lokaler Motorbetrieb funktioniert weiter,
   Recovery-AP steht zur Verfuegung.
6. AP-Zugriff auf `/api/v1/command` und `/api/v1/mode` wird abgelehnt;
   lokale STOP-/GO-Tasten und bestehende Webbedienung im Heimnetz bleiben korrekt.
7. Gespeicherte Gleispositionen, Motorparameter und Referenzdaten bleiben nach
   Netzwerkwechsel erhalten.

---

# HTTP-Babelfisch – HW3-Stand 03.11.2026

**Abgenommen im realen HW3-Test:** gemeinsamer Motorstatus, Web-Fahrbefehle, Diagnose, LOCAL/REMOTE-Bedienhoheit, Ein-Browser-Verriegelung, automatische Rueckgabe an LOCAL beim WLAN-/Verbindungsverlust sowie STOP ueber die erprobten Bedienwege. **AUTO/JMRI und OTA sind noch nicht implementiert.**

## Aktuelle Bedienhoheit (gilt vor historischen Testnotizen unten)

- **Bootzustand LOCAL:** fest angeschlossenes Keyboard darf steuern, Webbrowser duerfen lesen und STOP ausloesen.
- **REMOTE:** ausdruecklich per wechselndem Betriebsarten-Button am Web-Bedienpult uebernehmen, nur im Stillstand. Der ESP32 vergibt ein browsergebundenes zufaelliges Token. Nur der Besitzer darf `SELECT_TRACK`, `UNLOCK`, `ENABLE`, `DISABLE`, `REFERENCE` und `SET_TRACK` ausloesen. Weitere Browser bleiben Beobachter.
- **OLED im REMOTE-Betrieb:** `FERNSTEUERUNG / WEB AKTIV`; das lokale Keyboard sperrt alle Befehle ausser STOP. Rueckwechsel auf LOCAL erfolgt per Button oder Lease-Ablauf.
- **Lease:** Browser-Heartbeat ca. alle 2 s; ohne gueltige Lebenszeichen Rueckfall auf LOCAL nach ca. 10 s. Bereits angenommene Fahrt wird nicht durch den Berechtigungsverlust unterbrochen. Kein automatischer Wiederanlauf.
- **STOP:** `POST /api/v1/stop` bleibt ohne Token und unabhaengig von LOCAL/REMOTE zulaessig; auch lokaler STOP bleibt erlaubt. Bei HTTP-Statusabfragen/Diagnose besteht keine Steuerberechtigungspflicht.
- **AUTO:** nur als reservierte Betriebsart / Anzeige, noch keine JMRI-Uebergabe.
- **Parameter/NVM:** Aenderungen bleiben ueber UART; Browserdiagnose ist nur lesend.
- **Build-Option:** `FY_HTTP_WEB_TEST_CONTROL=1` aktiviert derzeit die Web-Fahrbefehle **unter der tatsaechlich implementierten Besitzerpruefung**; `0` schaltet sie ab. Der Name ist historisch. Das private Modellbahn-WLAN ist eine Betriebsannahme, kein Schutz gegen boeswillige Teilnehmer.
- **Statusfelder:** `mode` (LOCAL/REMOTE/AUTO), `web_owner` (nur true bei richtigem Token), `readonly`, `web_control`, `stop_latched`, `busy` und die uebrigen Muninn-Snapshots.
- **HTTP-Endpunkte zusaetzlich:** `POST /api/v1/mode` (mit Token bei Rueckgabe an LOCAL), `POST /api/v1/heartbeat` (mit Token), `GET /api/v1/status?token=...` zur besitzerspezifischen Anzeige. Regulaere `POST /api/v1/command` erhalten das Token als Formularfeld.

**Gezielt noch nicht einzeln als bestanden dokumentiert:** Hoheitswechsel waehrend einer Fahrt sowie Lease-Ablauf exakt waehrend der Bewegung. Die Nutzerin bestaetigte am 03.11.2026 den realen HW3-Test der normalen LOCAL/REMOTE-Umschaltung, der Mehrbrowser-Sperre, der Anzeigen, des Netzverlust-Rueckfalls und der STOP-Kombinationen. Testnachweis: [Issue #166](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/166).

---

## Historischer Entwicklungs- und Testverlauf (02.11.2026)


**Stand: 02.11.2026. Web-Fahrtest + Diagnose implementiert; bis zum neuen Firmware-Build und HW3-Test noch nicht abgenommen.**
Der funktionierende FY-Controller in `FY_ESP32/` bleibt die Grundlage.
`FY_ESP32_Web/` bleibt als **unabhängiger Simulationsprüfstand** erhalten.

## Architektur und Startfolge

1. `FY_ESP32/src/main.cpp` setzt sichere Motor-GPIO-Pegel und initialisiert Muninn.
2. Die bestehenden fünf FreeRTOS-Queues werden erzeugt; anschließend wird das Keyboard initialisiert.
3. `HTTP_prepare(&bridge)` erstellt lediglich die HTTP-Diagnosequeue und übernimmt den Kontext. **Kein WLAN-Warten in `setup()`.**
4. Muninn (Core 1) und die bestehende UART-/BabelFish-Task (Huginn, Core 0) starten unverändert.
5. Danach startet `Huginn_startHTTP(&bridge)` einen **eigenen, niedrig priorisierten Core-0-Task**. Er pflegt WLAN, mDNS und `WebServer`, während das lokale Keyboard weiter im Arduino-`loop()` abgefragt wird.

Der HTTP-Task übernimmt **keine** Motorfunktion, greift **nicht** auf STEP/Timer/GPIO zu und entnimmt **keine** Motorantwort aus der UART-Queue. HTTP, UART und Keyboard benutzen denselben `CommandRouter_enqueue()` fuer Muninns vorhandene normale und priorisierte Queue. HTTP besitzt eine eigene ACK/NACK-Queue mit Sequenzkennung `0x40000000`; UART und Keyboard behalten ihre eigenen Antwortpfade. Die bestehende Betriebsart `LOCAL` wird **nicht** umgeschaltet.

## WLAN lokal einrichten

Die Produktivfirmware ist **auch ohne WLAN-Datei baubar** und bleibt dann ausschließlich lokal bedienbar.

Um HTTP zu aktivieren, `FY_ESP32/include/WiFi_Secrets.example.h` **lokal** zu `FY_ESP32/include/WiFi_Secrets.h` kopieren und SSID/Passwort eintragen. `WiFi_Secrets.h` ist in `FY_ESP32/.gitignore` ausgeschlossen. Alternativ die vorhandenen Zugangsdaten aus dem separaten `FY_ESP32_Web`-Projekt lokal übernehmen; **nicht committen**.

Beim ersten gemeinsamen Test den alten Web-Prüfstand-ESP32 abschalten, damit nicht zwei Geräte denselben Hostnamen `fy-uvb.local` anbieten.

`pio run -d FY_ESP32` (bzw. PlatformIO-Projekt `FY_ESP32/` öffnen), danach erst auf HW3 flashen und testen.

## Zweiter Integrationsstand – Web-Fahrtest (noch NICHT an HW3 abgenommen)

**Explizite Testfreigabe im Branch:** `FY_ESP32/platformio.ini` enthält vorübergehend `-D FY_HTTP_WEB_TEST_CONTROL=1`. In einem Nicht-Test-Build ohne dieses Flag (`0`) weist der Server sämtliche Motorbefehle mit `NACK / WEB_TEST_DISABLED` ab. **Dies ist keine LOCAL/FERN-Bedienhoheit und keine Netzwerkauthentifizierung**. Jeder Teilnehmer im gleichen erreichbaren WLAN könnte im Test-Build Fahrbefehle senden. Deshalb nur in einem kontrollierten, privaten Entwicklungsnetz und unter Aufsicht an der realen Bühne testen; vor einem Produktiv-Merge das Flag wieder deaktivieren und die Bedienhoheit/Auftragsautorisierung einbauen.

### Befehlsweg

`Browser → HTTP-Server (Core 0) → CommandRouter_enqueue() → Muninn (Core 1) → httpEvents (eigenes ACK/NACK) → HTTP-JSON → Browser`.

| HTTP-Befehl | Muninn-Command | Daten | Wirkung |
| --- | --- | --- | --- |
| `UNLOCK` | `CMD_STOPP` | `AA 55`, priorisiert | STOP-Verriegelung **nur im verriegelten Stillstand** lösen |
| `ENABLE` | `CMD_GO` | `AA 55` | Treiber einschalten; ohne automatische Fahrt |
| `REFERENCE` | `CMD_REFERENCE` | `00` KURZ oder `01` LANG | Echte Referenzsequenz |
| `SET_TRACK` | `CMD_SET_TRACK` | `01..05` | Echte Fahrt zum gespeicherten Gleis |
| `DISABLE` | `CMD_GO` | `55 AA` | Treiber ausschalten (verwirft gemäß bestehender Muninn-Logik die Referenz) |
| `STOP` | `CMD_STOPP` | `55 AA`, Prioritätsqueue | Verriegelt STOP, bricht Fahrt ab und verwirft REF |
| `SELECT_TRACK` | – | `01..05` | **Nur** gemeinsame Vorwahl; keine Bewegung |

**Antworten:** HTTP `202 / ACK` ausschließlich nach passendem Muninn-Motorereignis; `NACK` mit Motorfehlergrund bei Ablehnung; `504 / UNKNOWN` bei Wartezeitüberschreitung: Annahme unklar, **nicht blind wiederholen**, zunächst Status abfragen. ACK steht niemals für abgeschlossene Fahrt. Die HTTP-Task wartet für einen einzelnen Auftrag begrenzt (bis 350 ms), während LOCAL und Muninn weiterlaufen. Weil `WebServer` derzeit synchron ist, können andere HTTP-Anfragen während dieses kurzen ACK-Wartens verzögert werden; **örtlicher STOP bleibt der maßgebliche Rückfallweg**.

**STOP-Latch:** `stop_latched` ist ein eigener atomarer Muninn-Snapshot. `STOPPED` allein kennzeichnet die Verriegelung nicht eindeutig. Nach `UNLOCK` bleibt der Motor zunächst `STOPPED`, bis `ENABLE` ihn freigibt.

**Wichtig zur Bedienung:** Browser und Keyboard können im Testbetrieb nacheinander Aufträge an Muninn geben; es gibt noch **keine explizite Bedienhoheit zwischen beiden**. Für die erste Erprobung nur eine Bedienstelle zur Zeit einsetzen. Das lokale Keyboard-Menü kann nach einem Web-Fahrtauftrag weiterhin im Zustand `LOCKED` sein; vor lokaler Übernahme dessen STOP-/Freigabesequenz durchlaufen.

### Diagnoseseite

`http://fy-uvb.local/diagnostics` enthält eine **rein lesende**, im Sekundentakt aktualisierte Statusansicht: reale Bühnenposition, Gleisziel/Istgleis, STOP-Latch, Motor-Treiber, Referenz, Fahrtrichtung, vier Sensorrohpegel, NVM-Masken, Firmware, Flash/Heap, WLAN und HTTP-RX/TX-Zähler sowie Füllstände der fünf FreeRTOS-Kommunikationsqueues plus neue HTTP-Queue. Die Daten stammen von `GET /api/v1/diagnostics`; dieser GET erzeugt **keinen UART-Trace**.

### Empfohlene HW3-Testfolge nach erfolgreichem Build

1. Altes Web-ESP32-Gerät ausschalten; `git pull` und `FY_ESP32/` neu kompilieren (nicht `FY_ESP32_Web/`).
2. Boot/Reset: `stop_latched=true`, Fahreranzeige `STOPPED`. Keine unerwartete Bewegung.
3. **ENTRIEGELN**: Muninn-ACK; `stop_latched=false`, Zustand ggf. weiterhin `STOPPED`.
4. **MOTOR EIN**: Muninn-ACK; Treiber an, `IDLE/UNREFERENCED`, keine Bewegung.
5. **REFERENZ KURZ**: reale Sensor-/Profilfahrt, BG3; korrekter Abschluss und `referenced=true`.
6. **BG5 vorwählen**, dann **FAHRT**: Motor fährt zur Position und beide Browser zeigen reale Bewegung.
7. STOP während Testfahrt: bevorzugt erst lokal; Browser-STOP bei beaufsichtigter weiterer Erprobung; prüfen, dass Bühne stoppt, Referenz ungültig wird und Verriegelung gesetzt ist.
8. **/diagnostics**: Sensorpegel, NVM, Queue-Zähler und HTTP-RX/TX prüfen. UART zeigt nur echte HTTP-RX-/TX-Ereignisse.
9. Keyboard/Browser nacheinander, ohne parallele Fahrbefehle; WLAN-Ausfall darf keine automatisch neue Fahrt starten.

### Multi-Client-HW3-Befund vom 02.11.2026 (spaeter Abend)

- **Bestaetigt:** Gleiswahl auf dem Handy, danach Start des Fahrbefehls am Laptop. Die gemeinsame Web-Vorwahl wird zwischen beiden Geraeten uebernommen; die reale Buehne reagiert auf den Auftrag.
- **Beobachtet:** Bei gleichzeitigen Abfragen durch Handy und Laptop stockt die grafische Buehnenanimation gelegentlich. Nach Nutzerbeobachtung funktioniert die Steuerung dennoch. Eine Ursache ist noch **nicht messtechnisch nachgewiesen**; der synchrone `WebServer` und zwei Poller mit je 250 ms sind moegliche Einflussfaktoren.
- **Nicht behaupten:** Keine gemessene HTTP-Latenz, kein Nachweis einer motorischen Unregelmaessigkeit.
- **Entscheidung:** Am heutigen, funktionierenden Stand keine kurzfristige Optimierung der Pollfrequenz. Zunaechst die gerade eingecheckte LOCAL-Keyboard-Menuesynchronisation nach Build/Flash pruefen.
- **Spaeter:** Parallel-Clients unter Last messen (Antwortzeiten/Poll-Luecken), bei Bedarf HTTP-Status und Animation entkoppeln bzw. Pollverhalten anpassen.

### LOCAL-Keyboard-Synchronisation nach Browserbefehlen (noch Hardwaretest offen)

Beobachtung beim ersten Web-Fahrtest: Webfahrt und STOP funktionieren, aber die Nutzerin konnte anschließend keine normalen LOCAL-Kommandos am Keyboard auslösen. Ursache im Code: Das lokale OLED-Menü blieb beim eigenen `LOCKED`-/`REF_MENU`-/`TRACK_MENU`-Ablauf, obwohl Muninn Befehle aus HTTP bereits ausgeführt hatte.

**Korrektur im Branch:** `Keyboard_poll()` ruft nach dem Einsammeln lokaler Motorantworten `syncMenuWithMotor()` auf. Die Routine liest nur Muninns STOP-Latch-, Treiber-, Motor-, Referenz- und Gleis-Snapshots. Sie synchronisiert bei fremden Befehlen `LOCKED`, `ENABLE`, `REF_MENU`, `REF_RUN`, `TRACK_MENU` und `TRACK_RUN`. Sie verschickt **keinen** Motorbefehl. Wenn STOP über das Web **entriegelt**, aber der Motor noch **ausgeschaltet** ist, kann die Nutzerin über die grüne GO-Taste die Motorfreigabe lokal auslösen; eine automatische Freigabe ist ausgeschlossen. Die rote Keyboard-LED folgt dem echten STOP-Latch statt allein dem Motorstatus `STOPPED`. Eine von der Nutzerin lokal vorgewählte Gleisnummer bleibt bis zur nächsten bestätigten Fahrt erhalten.

**Gezielte Regression vor Freigabe:**
1. Nach POR mit verriegeltem STOP funktionieren die gewohnten lokalen Entriegelungsgriffe unverändert.
2. **WEB ENTRIEGELN → LOCAL GO**: Motorfreigabe nur nach örtlichem Tastendruck; OLED zeigt anschließend das Referenzmenü.
3. **WEB MOTOR EIN / WEB REFERENZ / WEB BG5**: Local-OLED erkennt REF- und Fahrzustände; anschließend kann über lokale Tasten ein anderes BG gewählt und gefahren werden.
4. **WEB STOP während Fahrt** und **LOCAL STOP während Webfahrt**: Bühnenstopp, STOP-Latch, REF-Verlust und LOCAL-Menü `LOCKED` stimmen überein.
5. Bei eigener lokaler Gleisvorwahl ohne neue Fahrt darf die Hintergrundsynchronisation die Vorwahl nicht ungefragt überschreiben.
6. Browser und LOCAL nicht gleichzeitig bedienen; **Bedienhoheit ist weiterhin nicht implementiert**.

**Noch offen:** Der reale HW3-Test dieser neuen Richtung und der Keyboard-Korrektur, Build-/Warnungsprüfung, Verhalten bei überfüllten Queues und Timeout, vollständige LOCAL/FERN/AUTO-Arbitrierung, Authentifizierung, STOP-Latenz über HTTP unter Last. Für JMRI ist diese Testfreigabe ausdrücklich ungeeignet.

## HTTP-Vertrag – aktueller Entwicklungsstand

| Endpunkt | Verhalten |
| --- | --- |
| `GET /` | Web-Bedienpult; Muninn-Live-Status und im Test-Build Motorsteuerung |
| `GET /diagnostics` | Rein lesende Diagnoseseite |
| `GET /api/v1/status` | Motor-/Vorwahl-Snapshots incl. `stop_latched`, `readonly` und `web_control` |
| `GET /api/v1/diagnostics` | Erweiterte System-/Sensor-/Kommunikationsdiagnose |
| `POST /api/v1/command` | `SELECT_TRACK`, `UNLOCK`, `ENABLE`, `DISABLE`, `REFERENCE`, `SET_TRACK` |
| `POST /api/v1/stop` | Priorisierter STOP über bestehende Muninn-Queue |
| `GET /api/status` | WLAN-/IP-Diagnose |
| `GET /api/system` | Flash-/Heap-Diagnose |

Im Web-Test-Build sind die Motorbefehle aktiv; ohne Test-Flag sind sie **serverseitig gesperrt**. Die Weboberfläche aktiviert ihre Bedientasten ausschließlich, wenn der Server `readonly=false` und `web_control=TEST` meldet.

### Statusfelder

`simulation=false`, `readonly=true`, `mode=LOCAL`, `state`, `busy`, `referenced`, `target_track`, `track` (nur bei bestätigter Stellung), `selected_track`, `position_steps` und `position` (grafische Gleisskala 1..5).

`position` wird ausschließlich aus einer **gültigen realen Referenzposition** und den gespeicherten Gleispositionen abgeleitet. Wenn die Position nicht sicher darstellbar ist, lautet sie **null**; es wird **keine erfundene BG3-Lage** gemeldet. Der erste Browser-Snapshot kann ohne Referenz also noch keine Bühnenstellung bestätigen. Die Web-Vorwahl bleibt unabhängig davon benutzbar.

`SELECT_TRACK` bleibt reine Web-Vorwahl im RAM; `CMD_SET_TRACK` (`0x1C`) ist der **separate echte Fahrtauftrag**. Der gemeinsame Queue-Router ist eingebaut; **Bedienhoheit und Authentifizierung fehlen weiterhin** und sind vor produktivem Einsatz zu ergänzen.

## UART-Diagnose: nur HTTP-Funkverkehr

`HTTP.cpp` legt bei jedem **HTTP-POST-Befehl** eine RX- und eine TX-Diagnose als je eine JSON-Zeile in einer nicht blockierenden Queue ab. `UART::update()` gibt bis zu vier dieser Zeilen pro Durchlauf über `Serial` aus. **Statuspolling erzeugt keinen Trace.** Beispiele:

```json
{"transport":"HTTP","dir":"RX","cmd":"SELECT_TRACK","value":4}
{"transport":"HTTP","dir":"TX","cmd":"SELECT_TRACK","result":"ACK","reason":"ACCEPTED"}
```

Das alte UART-Drahtprotokoll und dessen binäre Antworten bleiben im Code bestehen. Die JSON-Zeilen dienen ausschließlich der **Entwicklungsdiagnose**; ein späterer serieller Protokollclient müsste diese Zusatz-Ausgaben deaktivieren oder korrekt trennen. Bei voller Trace-Queue wird eine Debugzeile verworfen, **niemals** der HTTP-Auftrag aufgehalten.

## Hardware-Befund HW3 vom 02.11.2026 (Nutzerbericht)

**Beobachtet und bestätigt:**
- Der integrierte FY-Webserver läuft auf dem realen FY-ESP32; Laptop und Mobiltelefon zeigen die reale Bühnenstellung korrekt an.
- Während lokaler Bühnenfahrten stimmen die Darstellung und die Kontrollleuchte nach Beobachtung mit der realen Mechanik überein.
- Reset und STOP wurden auf der Weboberfläche als Zustandsänderungen erkannt.
- Serielle Meldung nach realer Referenz: `REF: VALID pos=1000 end=8334`.
- Zwei Vorwahlaufträge `SELECT_TRACK 5` und `SELECT_TRACK 1` wurden mit jeweils einem HTTP-RX und einem TX/ACK im UART-Monitor protokolliert.
- Gemeldeter PlatformIO-Build: RAM `48472 / 327680` Bytes (14,8 %), Flash `887921 / 1310720` Bytes (67,7 %); die angezeigte Build-Ausgabe reichte bis zur Erstellung der `firmware.bin`.

**Noch nicht nachgewiesen:**
- STOP/Abbruch **während einer laufenden Fahrt** einschließlich Web-Rückmeldung, Referenzverlust und sicherem Stillstand.
- Direkte HTTP-POST-Sperrtests für `SET_TRACK`, `REFERENCE` und `STOP` (Server-NACK/423).
- Vollständiger Clean-Build-Test *ohne* WLAN-Datei, WLAN-Verbindungsverlust und gesamte UART-/NVM-/LOCAL-Regression.
- Kein vollständiger Nachweis, dass UART-Debug-Ausgaben unter Last niemals relevante Tasks verzögern.

Dies ist eine **Teilabnahme der lesenden HTTP-Statusintegration**, keine Freigabe von HTTP-Motorbefehlen oder einer vollständigen Systemabnahme.

## Abnahmeschritte für diesen Stand

1. **Clean Build** von `FY_ESP32` mit und ohne lokaler `WiFi_Secrets.h`; Flash/RAM notieren.
2. **POR mit ausgeschaltetem WLAN:** örtliche STOP-Verriegelung und Keyboard-Funktion unverändert; kein WLAN-Starttimeout.
3. **WLAN aktiv:** beide Browser zeigen denselben echten Muninn-Zustand. Fehlende REF darf nicht als bestätigte Gleisstellung erscheinen.
4. **Vorwahl BG1–BG5:** der Fahrgeber synchronisiert sich zwischen Laptop und Handy, ohne einen STEP auszulösen.
5. **Direkter HTTP-POST SET_TRACK / REFERENCE / STOP:** muss NACK/423 liefern; nicht auf JS-Sperre verlassen.
6. **USB-Monitor:** genau ein RX und ein TX für jeden POST; **keine** periodischen GET-Statusmeldungen.
7. **LOCAL-Regression:** STOP, STOP-Freigabe, GO, kurze/lange Referenz, Gleisfahrten und UART-Telegramme nach bestehendem Vertrag prüfen.

**Nächste Stufe:** HW3-Regression dieser neuen Funktionen; LOCAL/FERN/AUTO-Bedienhoheit; sichere Freigabe/Authentifizierung; priorisierte STOP-Einbindung auch unter voller Queue-Last. Siehe [Bedienhoheit/JMRI](../../../FY_ESP32_Web/docs/BEDIENHOHEIT_JMRI.md) und [Systemarchitektur](../SYSTEM_ARCHITECTURE.md).
