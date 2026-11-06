# FY ESP32 3.2.0 – OTA (Freigabestand)

## Funktion

- Firmware-Upload im vertrauten Heim-/Modellbahn-WLAN unter `/ota`;
  Datei: `FY_ESP32/.pio/build/esp32dev/firmware.bin`.
- Aktiven und inaktiven OTA-Slot sowie groesste erlaubte Dateigroesse
  ermitteln, ungeeignete Dateien nicht aktivieren.
- Physische Freigabe: rote STOP-Taste **3 Sekunden** halten, waehrend
  die Buehne stillsteht, STOP verriegelt und der Antrieb deaktiviert ist.
  Freigabe gilt 120 Sekunden; danach erneut bestaetigen.
- Upload-Fortschritt im Browser und auf dem OLED; nach erfolgreichem
  Firmware-Schreiben und Validieren kontrollierter Neustart.
- OTA-Sperre in CommandRouter **und** Muninn: keine neuen Fahr- oder
  GO-/Freigabebefehle waehrend OTA; prioritaerer STOP bleibt verfuegbar.
- WLAN-STA-MAC auf `/network`, zur DHCP-IP-Reservierung im Router.
  Keine statische IP-Konfiguration im ESP32 erforderlich.
- `[platformio] default_envs=esp32dev`: normaler Build standardmaessig;
  `esp32dev_ota_bench` bleibt ausschliesslich fuer ausdrueckliche
  Tischmusterdiagnose vorgesehen; sein USB-Sonderkommando ist in der
  normalen Firmware **nicht** einkompiliert.

## Auf HW3 und Tischmuster erfolgreich getestet (06.11.2026)

- Tischmuster: USB-Bench-Build, serielle OTA-Testfreigabe,
  Browser-OTA, Fortschrittsanzeige, Neustart und Rueckkehr zum
  Normal-Build ohne serielle Freigabe.
- HW3: normaler Build und USB-Flash, physische Freigabe per roter
  STOP-Taste, normaler Fahrbetrieb, OTA-Upload und Neustart,
  Abweisung von Fahrbefehlen waehrend OTA und Anzeige der STA-MAC
  (Rueckmeldung der Anwenderin).
- Der normale Motorstopp deaktiviert den A4988-Treiber
  (`ENABLE=HIGH`). OTA kann erst nach bestaetigtem STOP/Stillstand
  gesperrt starten.

## Ausstehende Negativtests / akzeptiertes Restrisiko

- Absichtlicher Abbruch des Uploads bzw. Stromausfall beim Schreiben,
  ungueltige `.bin`, zu grosse Datei und Wiederanlauf im Fehlerfall
  wurden nicht vollstaendig einzeln auf HW3 abgenommen.
- STOP-Prioritaet **waehrend eines laufenden Flash-Schreibvorgangs**
  wurde nicht separat als Zeit-/Reaktionstest gemessen. STOP ist
  softwareseitig weiter erlaubt; die OTA-Sperre verlangt bereits
  vor Beginn abgeschalteten Antrieb.
- NVS-Datenpersistenz nach dem OTA auf HW3 nicht eigens als
  Wiederholtest der Motorparameter/Gleise protokolliert.
- Physische STOP-Freigabe ist kein kryptographisch authentifizierter
  oder signierter Firmware-Upload. OTA nur im vertrauten lokalen WLAN.
  Kein Zugriff aus dem Internet bereitstellen.
- Recovery per USB steht bereit. Das ersetzt keinen unabhängigen
  mechanischen Sicherheitsnachweis.

## Entscheidung

Nach erfolgreich gemeldeter HW3-Funktionspruefung wird 3.2.0 zur
Integration nach `main` vorgeschlagen. Obige Negativtests werden als
offene Regressionstests fuer ein Folge-Release dokumentiert; keine
Behauptung einer umfassenden Fehlertoleranz-Abnahme.

Siehe [Issue #190](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/190).
