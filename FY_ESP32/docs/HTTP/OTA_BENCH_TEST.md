# OTA 3.2.0 – Tischmuster-Test ueber USB-Seriell

> **Nur Entwicklungsstand im Branch** `feature/FY_ESP32_OTA`.
> Nicht auf die eingebaute FY-Schiebebuehne laden und nicht als Release verteilen.

## Vorbereitung

Das Tischmuster benoetigt **keinen Taster**, aber weiterhin die echten
Muninn-Schutzbedingungen: Power-On STOP verriegelt, Motor AUS und Stillstand.
Der spezielle Build unterscheidet sich vom spaeteren Produktiv-Build durch
`FY_OTA_BENCH_TEST_ARM=1`. Im Produktiv-Build ist der serielle Befehl
vollstaendig ausgeblendet.

1. In PlatformIO **Environment `esp32dev_ota_bench`** waehlen.
2. Per USB **Build + Upload** dieses Environments ausfuehren. Beispiel:

   ```bash
   pio run -e esp32dev_ota_bench
   pio run -e esp32dev_ota_bench -t upload
   ```

3. Seriellen Monitor mit **115200 Baud**, Zeilenende CR oder LF, oeffnen.
   Auf diese Warnung achten:

   ```text
   [OTA] WARNUNG: TISCHMUSTER-BUILD, NICHT FUER FY-HW!
   [OTA] USB-Seriell: !OTA ARM <ENTER> fuer 120 s
   ```

4. `!OTA ARM` eingeben und Enter druecken. Erwartet:

   ```text
   [OTA] BENCH: Freigabe 120 s, Browser /ota
   ```

   Wenn `Abgelehnt`: STOP-Zustand, Motorfreigabe oder laufende Bewegung
   pruefen; niemals die Motorsperre einfach ueberbruecken.

5. Im Browser **http://<IP-des-Tischmusters>/ota** aufrufen (nicht
   blind `fy-uvb.local`, falls die produktive FY ebenfalls online ist).
6. Datei **`.pio/build/esp32dev_ota_bench/firmware.bin`** auswaehlen,
   Update starten und Browser-/UART-Meldungen beobachten.
   Beim ersten Funktionstest darf exakt dasselbe Binary erneut per OTA
   geladen werden; dabei wird der inaktive OTA-Slot beschrieben.
7. Nach dem erfolgreichen Neustart erneut die FY-IP aufrufen. Die
   Wiederherstellung der WLAN-Daten und das Umschalten des laufenden
   OTA-Slots in `/ota` pruefen.

## Sicherheits-/Negativtests vor Einsatz auf HW3

- Ohne serielle/physische Freigabe wird OTA verweigert.
- Kein OTA bei aktivierter Motorfreigabe oder laufender Bewegung.
- Ungueltige oder zu grosse `.bin` und abgebrochener Upload werden abgelehnt;
  die bisherige Firmware bleibt startfaehig.
- STOP bleibt zulaessig; GO, REF und Gleisfahrt sind waehrend aktivem OTA
  gesperrt.
- Bei unterbrochener Stromversorgung darf die bisherige Firmware nicht
  vorschnell als garantiert startfaehig betrachtet werden; Recovery testen.
- Die finale `3.2.0` wird mit **Environment `esp32dev` ohne Bench-Flag**
  gebaut. Fuer HW3 ausschliesslich die physische STOP-Langdruckfreigabe nutzen.

## Grenzen

Die lokale Browser-Oberflaeche verwendet HTTP im vertrauten privaten WLAN,
und der Upload hat aktuell keine kryptographische Firmware-Signatur.
Die lokale Freigabe ist ein Sicherheits-Interlock, keine Ende-zu-Ende-
Authentifizierung. OTA bleibt ein Testfeature, bis alle HW3-Tests bestanden sind.
