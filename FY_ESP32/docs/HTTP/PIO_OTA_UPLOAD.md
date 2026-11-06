# FY-ESP32 – OTA direkt aus VS Code / PlatformIO

Die Firmware kann direkt aus dem vorhandenen PlatformIO-Environment
`esp32dev` gebaut und ueber WLAN hochgeladen werden. Das FY-eigene
HTTP-OTA-Protokoll wird verwendet – **kein ArduinoOTA/espota-Port**
und kein weiteres PC-Programm erforderlich.

## Einmalige Vorbereitung

1. Die Schiebebuehne benoetigt mindestens Firmware **3.2.0** mit der
   bestehenden Webseite `/ota`.
2. Die Schiebebuehne ist standardmaessig unter
   [http://fy-uvb.local/](http://fy-uvb.local/) erreichbar.
   Das ist auch das vorkonfigurierte OTA-Ziel:
   
   ```ini
   custom_ota_host = fy-uvb.local
   ```

   **Eine feste IP oder DHCP-Reservierung ist nicht notwendig.**
   Das abgeschlossene Tischmuster ist nicht mehr parallel im WLAN.
   Falls Windows/Python `fy-uvb.local` nicht aufloesen kann,
   als Rueckfalladresse die aktuelle IP aus `/network` eintragen,
   z.B. `custom_ota_host = 192.168.X.Y`.

## Update aus VS Code

1. In VS Code `PlatformIO > Project Tasks > esp32dev >
   Custom > OTA Upload` waehlen.
   Alternativ im `FY_ESP32`-Projektverzeichnis:

   ```bash
   pio run -e esp32dev -t ota
   ```

2. **Zuerst** wird die Firmware gebaut (bei unveraendertem Projekt
   wird der vorhandene Build verwendet). Deshalb die physische
   Freigabe **noch nicht vorher** ausloesen.
3. Sobald im Terminal `Jetzt an der FY die rote STOP-Taste 3 Sekunden
   halten` erscheint, an der FY den Motor stoppen und die
   **rote STOP-Taste drei Sekunden halten**. Die OTA-Freigabe
   gilt 120 Sekunden; das Programm wartet maximal 90 Sekunden auf
   die Freigabe und startet danach selbststaendig die Uebertragung.
4. Das Terminal zeigt den Ziel-Hostnamen, die per Netzwerkstatus gelesene
   STA-MAC, den Uebertragungsfortschritt und zum Abschluss den
   bestaetigten HTTP-OTA-Erfolg an.
5. Erst bei `FY OTA: ACK erhalten` gilt das Update als erfolgreich
   an den ESP32 uebergeben; der FY startet anschliessend neu.
   Bei Verbindungsfehlern wird **kein** automatischer Wiederholversuch
   vorgenommen. Den tatsaechlichen Firmware-Status am FY kontrollieren.

### Targetregistrierung pruefen

Bei `Do not know how to make File target 'ota'` ist kein Upload
gestartet. Zuerst den aktuellen Stand des Entwicklungsbranches
laden und dann die registrierten Targets auflisten:

```bash
pio run -e esp32dev --list-targets
```

Unter den benutzerdefinierten Targets muss `ota` erscheinen.
Fehlt der Eintrag, sicherstellen, dass die lokale
`platformio.ini` das `post:scripts/pio_ota_target.py`
enthält und `FY_ESP32/scripts/pio_ota_target.py` vorhanden ist.

## Abgrenzung

- `Upload` im `esp32dev`-Task bleibt **USB/seriell**.
- `OTA Upload` ist der **separate** WLAN-Task; er nutzt dieselbe
  `.pio/build/esp32dev/firmware.bin`, keine zweite Kompilierung
  unter einem eigenen Environment.
- Die Testumgebung `esp32dev_ota_bench` bleibt separat und ist
  ausdruecklich nicht fuer die eingebaute FY bestimmt.
- Der Uploader verwendet nur die Python-Standardbibliothek und
  benoetigt keine `requests`- oder `espota`-Installation.
- Der Python-Client sendet zunaechst `GET /api/v1/ota/status`,
  verlangt eine Firmwareversion ab `3.2.0`, einen vorhandenen Slot,
  gueltige Dateigroesse, physische Freigabe und CSRF-Token.
- Danach `POST /api/v1/ota/prepare` und anschliessend einen gestreamten
  `multipart/form-data`-Upload nach `/api/v1/ota/upload`.
- **Kein** Passwort, keine automatische Freigabe und keine
  Umgehung von STOP; keine neuen Serverendpunkte notwendig.
- Es handelt sich um HTTP im vertrauenswuerdigen lokalen WLAN.
  Es gibt weder Firmware-Signatur noch Transportverschluesselung.
  Nicht ins Internet weiterleiten.

## Regression vor Merge

- [ ] PlatformIO-Task `OTA Upload` wird nur bei `esp32dev` angezeigt.
- [ ] Projekt baut weiterhin mit `pio run -e esp32dev`.
- [ ] Falsche oder nicht gesetzte Ziel-IP wird **vor** einem Upload gemeldet.
- [ ] Versuch ohne physische Freigabe endet sicher mit Fehlermeldung.
- [ ] Test auf Tischmuster oder HW3: OTA ueber VS Code,
      HTTP-ACK, Neustart, Firmwareversion und NVS-Daten pruefen.
- [ ] Normaler USB-Upload funktioniert weiterhin unveraendert.
