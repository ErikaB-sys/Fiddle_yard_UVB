# FY-ESP32 – OTA direkt aus VS Code / PlatformIO

Die Firmware kann direkt aus dem vorhandenen PlatformIO-Environment
`esp32dev` gebaut und ueber WLAN hochgeladen werden. Das FY-eigene
HTTP-OTA-Protokoll wird verwendet – **kein ArduinoOTA/espota-Port**
und kein weiteres PC-Programm erforderlich.

## Einmalige Vorbereitung

1. Die Schiebebuehne benoetigt mindestens Firmware **3.2.0** mit der
   bestehenden Webseite `/ota`.
2. Die **aktuelle** IP der FY auf `http://<FY-IP>/network` ablesen.
   Eine feste DHCP-Reservierung im Router ist fuer OTA **nicht erforderlich**,
   kann aber spaeter eingerichtet werden. Wenn der Router die IP aendert,
   muss `custom_ota_host` entsprechend angepasst werden.
3. In `FY_ESP32/platformio.ini` unter `[env:esp32dev]` **genau eine**
   Zeile bearbeiten:

   ```ini
   custom_ota_host = 192.168.X.Y
   ```

   Dabei `192.168.X.Y` durch die tatsaechliche IP der **echten FY**
   ersetzen. Nicht die IP des Tischmusters verwenden. Der Platzhalter
   `SET_FY_IP` fuehrt absichtlich zu einer Fehlermeldung. Hostname
   `fy-uvb.local` ist nur geeignet, wenn nicht zwei Boards damit
   gleichzeitig im LAN sind.

## Update aus VS Code

1. Falls notwendig zuerst `PlatformIO: Build` ausfuehren; der OTA-Task
   baut die Firmware auch selbst bei Aenderungen.
2. Direkt an der FY: Motor stoppen (STOP verriegelt, Treiber AUS),
   die **rote STOP-Taste drei Sekunden halten**. Das gibt OTA 120 s
   frei. Die Physik-Interlocks bleiben unveraendert.
3. In VS Code `PlatformIO > Project Tasks > esp32dev >
   Custom > OTA Upload` waehlen.
   Alternativ im `FY_ESP32`-Projektverzeichnis:

   ```bash
   pio run -e esp32dev -t ota
   ```

4. Das Terminal zeigt die Ziel-IP, die per Netzwerkstatus gelesene
   STA-MAC, den Uebertragungsfortschritt und zum Abschluss den
   bestaetigten HTTP-OTA-Erfolg an.
5. Erst bei `FY OTA: ACK erhalten` gilt das Update als erfolgreich
   an den ESP32 uebergeben; der FY startet anschliessend neu.
   Bei Verbindungsfehlern wird **kein** automatischer Wiederholversuch
   vorgenommen. Den tatsaechlichen Firmware-Status am FY kontrollieren.

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
