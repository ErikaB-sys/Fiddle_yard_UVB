# 3.2.0_OTA – FY ESP32

**Release-Tag:** `3.2.0`  
**Release-Name:** `3.2.0_OTA`  
**Stand:** 06.11.2026  
**Basis:** `3.1.0_Network`

Mit Version 3.2.0 kann die Fiddle-Yard-Schiebebuehne ihre ESP32-Firmware
ueber das lokale WLAN aktualisieren – ohne fuer jedes Update ein USB-Kabel
anschliessen zu muessen.

## Neu

- **OTA-Weboberflaeche unter `/ota`:** `firmware.bin` aus PlatformIO
  auswaehlen, aktuelle Firmware/OTA-Partition anzeigen, Groesse pruefen,
  Upload-Fortschritt und Ergebnis verfolgen. Nach bestaetigtem,
  erfolgreichem Schreiben startet die Steuerung neu.
- **Physische Freigabe:** Rote STOP-Taste mindestens drei Sekunden
  halten. OTA ist anschliessend fuer 120 Sekunden freigegeben, sofern
  STOP verriegelt, Motor stillstehend und A4988-Treiber deaktiviert ist.
- **Motorverriegelung waehrend OTA:** Fahr- und GO-Befehle werden
  sowohl an der Befehlsannahme als auch in der Motor-Task abgewiesen.
  Prioritaerer STOP bleibt verfuegbar. Ein Update setzt bereits vor
  Beginn den gestoppten, deaktivierten Antrieb voraus.
- **Diagnose:** OTA-Status, freier Updateslot und Fortschritt werden
  im Browser angezeigt; Freigabe und Fortschritt auch am OLED.
- **Netzwerkseite `/network`:** Zeigt die WLAN-STA-MAC-Adresse zum
  Einrichten einer optionalen festen DHCP-Zuordnung im Router.
- **OTA direkt aus VS Code/PlatformIO:** Neuer Custom-Task
  `esp32dev > Custom > OTA Upload` bzw.
  `pio run -e esp32dev -t ota`. Der Python-Uploader baut die normale
  Firmware, findet die FY standardmaessig unter `fy-uvb.local`,
  wartet *nach dem Build* auf die physische STOP-Freigabe, laedt die
  Firmware ueber das bestehende HTTP-Protokoll und meldet das ACK.
  Kein zweites OTA-Build-Environment und keine externen Python-Pakete
  erforderlich.
- **USB bleibt nutzbar:** Der normale PlatformIO-Task `Upload`
  schreibt weiterhin seriell. Das separate
  `esp32dev_ota_bench`-Environment ist nur fuer Tischmustertests
  vorgesehen; seine Testfreigabe ist im normalen `esp32dev`-Build
  nicht vorhanden.

## Erfolgreich getestete Funktionen

**Tischmuster:** Browser-OTA, Fortschritt, Reboot, Umschaltung der
OTA-Slots und Entfernung der seriellen Testfreigabe beim Wechsel zur
normalen Firmware.

**Echte HW3-Schiebebuehne:** Normaler Build/USB-Flash, physische
STOP-Freigabe, Fahrfunktion, Browser-OTA, Restart, gesperrte
Fahrbefehle waehrend OTA sowie Anzeige der WLAN-STA-MAC-Adresse
laut praktischer Rueckmeldung erfolgreich.

**VS Code/PlatformIO auf Windows:** OTA-Upload **946.432 Bytes** bis
100 %, Antwort `FY OTA: ACK erhalten` und PlatformIO
`[SUCCESS] Took 41.47 seconds`; Firmware-Neustart bestaetigt.

## Update – so geht's

**Browser:** An der stillstehenden Buehne rote STOP-Taste drei
Sekunden halten. `http://fy-uvb.local/ota` oeffnen, Datei
`FY_ESP32/.pio/build/esp32dev/firmware.bin` auswaehlen und
Update starten. Nach erfolgreicher Uebertragung/Pruefung startet
die FY neu.

**VS Code:** `FY_ESP32` als PlatformIO-Projekt oeffnen.
`Project Tasks > esp32dev > Custom > OTA Upload` starten.
**Erst wenn das Terminal dazu auffordert**, die rote STOP-Taste
drei Sekunden halten. Der Task verwendet den normalen
`esp32dev`-Build und wartet danach auf das Update-ACK.

## Hinweise und bekannte Grenzen

- OTA nur im **vertrauenswuerdigen lokalen WLAN** verwenden.
  Das HTTP-Verfahren bietet keine TLS-Verschluesselung und keine
  kryptographische Firmware-Signatur. Die physische Freigabe
  ersetzt keine formelle Admin-Authentifizierung.
- Das 120-Sekunden-Fenster gilt fuer die Einleitung des Updates;
  die Motorverriegelung bleibt waehrend des eigentlichen Updates aktiv.
- Ein erstmaliges Flashen eines noch nicht OTA-faehigen Controllers
  erfolgt weiterhin ueber USB. USB bleibt auch als Recovery-Weg.
- Bei fehlgeschlagenem Upload, abgebrochener Funkverbindung oder
  Stromunterbrechung den Startzustand kontrollieren. Kein
  automatischer Rollback auf eine vorherige Anwendungsversion
  wird zugesichert.
- Gezielte Negativtests (Upload-Abbruch, ungueltiges Firmware-Image,
  Stromausfall, STOP-Reaktionszeit waehrend Flash-Schreiben,
  vollstaendige NVS-Regression) sind weiterhin offen und werden
  in [Issue #187](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/187)
  nachgehalten; sie sind keine behaupteten Abnahmeergebnisse.
- mDNS `fy-uvb.local` muss vom Windows-Rechner erreichbar sein.
  Alternativ in `platformio.ini` `custom_ota_host` auf die
  derzeitige FY-IP setzen. Eine statische IP ist nicht notwendig.

## Entwicklungsreferenzen

- [OTA-Grundfunktion – Issue #190](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/190)
- [OTA aus VS Code – Issue #192](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/192)
- [Pruef- und Restrisiko-Protokoll](https://github.com/ErikaB-sys/Fiddle_yard_UVB/blob/main/FY_ESP32/docs/HTTP/OTA_RELEASE_3.2.0.md)
- [VS-Code-OTA-Anleitung](https://github.com/ErikaB-sys/Fiddle_yard_UVB/blob/main/FY_ESP32/docs/HTTP/PIO_OTA_UPLOAD.md)
