# ESP32-WROOM (38 Pin) – HW3-Gesamtbelegungsplan

**Stand: 22.10.2026 · Branch `feature/FY_ESP32_Port`.**
**HW3-Zielplan für die Verdrahtung; noch nicht elektrisch abgenommen und noch nicht vollständig in der Firmware umgesetzt.**
Die Spalte „Code bisher“ zeigt den beim Abgleich gelesenen Stand von [HW_Config.h](../../include/HW_Config.h). Insbesondere die vier Sensorpins müssen vor Nutzung der neuen Verdrahtung im Code umgestellt werden. SLEEP benötigt zusätzlich eine neue Ansteuerung. Der vorhandene HW2-Bench-Nachweis gilt nicht automatisch für HW3.

## Anforderungen und Quellen

- A4988 mit STEP/DIR/ENABLE und einzeln steuerbaren MS1–3; G13/G14/G4 bleiben erhalten, **G12 bleibt unbeschaltet** (Hardware-Abstimmung 22.10.2026).
- Zwei Lichtschranken und zwei Endschalter unmittelbar an Muninn: [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155).
- Boot-/Reset-Grundzustände durch Hardware-Pulls; separat abschaltbares VMOT; Schutzmessungen noch offen: [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155), [#161](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/161).
- I²C-Keyboard mit vier LED-Tastern und OLED; PCF8574 wird gepollt, **kein INT-GPIO**, kein zusätzlicher Keyboard-Nano/UART: [Keyboard](../Keyboard/README.md), [#157](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/157).
- Optionales SPI-Schaufensterdisplay und MCP23S17 für 16 Lichtkreise: zwei CS plus Display-DC, separates 12-V-Kabel: [#165](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/165).
- USB/UART0 bleibt für Programmierung und vorhandenes Protokoll verfügbar. WLAN benötigt keine zusätzlichen Header-GPIOs.
- Kein eigener ESP32-Ausgang für Display-Reset oder Backlight eingeplant. Lokale Beschaltung hängt vom gewählten Display ab.
- Versorgung nach [HW3_STROMVERSORGUNG.md](HW3_STROMVERSORGUNG.md): externe 3,3-V-Einspeisung und USB-Versorgung umschaltbar, keine ungeprüfte Parallelspeisung.

## Vollständige Funktionsbelegung

GPIO-Zahlen beziehen sich auf die Board-Aufschrift Gxx, **nicht auf Steckerpositionsnummern**. G36 kann VP, G39 VN heißen. Plan gilt für das vorhandene **WROOM ohne PSRAM**, nicht ungeprüft für WROVER oder andere ESP32-Varianten.

| Gruppe | Funktion / Anschluss | HW3 GPIO | Richtung am ESP32 | Code bisher / Änderung |
|---|---|---:|---|---|
| Motor | A4988 STEP | G25 | Ausgang | G25, unverändert |
| Motor | A4988 DIR | G26 | Ausgang | G26, unverändert |
| Motor | A4988 !ENABLE / EN | G27 | Ausgang | G27, unverändert; LOW = Wicklungen freigegeben |
| Motor | M0 → A4988 MS1 | G13 | Ausgang | G13, unverändert |
| Motor | M1 → A4988 MS2 | G14 | Ausgang | G14, unverändert |
| Motor | M2 → A4988 MS3 | G4 | Ausgang | G4, unverändert |
| Motor | A4988 !SLEEP / SLP | **G32** | Ausgang | **Neu**; LOW = Schlafzustand |
| Sensor | Lichtschranke REF / LSREF | **G34** | nur Eingang | **bisher G32** |
| Sensor | Lichtschranke 2 / Riemen / LS4 | **G35** | nur Eingang | **bisher G33** |
| Sensor | Endschalter links / LSL | **G36 / VP** | nur Eingang | **bisher G16** |
| Sensor | Endschalter rechts / LSR | **G39 / VN** | nur Eingang | **bisher G17** |
| I²C | SDA – PCF8574 und OLED | G21 | bidirektional | G21, unverändert |
| I²C | SCL – PCF8574 und OLED | G22 | Open-Drain-Bus | G22, unverändert |
| SPI | SCLK – gemeinsam | G18 | Ausgang | bisher reserviert |
| SPI | MISO – MCP23S17, ggf. Display | G19 | Eingang | bisher reserviert |
| SPI | MOSI – gemeinsam | G23 | Ausgang | bisher reserviert |
| SPI | !CS_DISPLAY | **G16** | Ausgang | **Neu reserviert** |
| SPI | !CS_EXPANDER | **G17** | Ausgang | **Neu reserviert** |
| SPI | Display D/C | **G33** | Ausgang | **Neu reserviert** |
| Service | UART0 TX → USB-UART | G1 / TX0 | Ausgang | für USB/UART0 freihalten |
| Service | UART0 RX ← USB-UART | G3 / RX0 | Eingang | für USB/UART0 freihalten |
| Diagnose | bisherige Test-LED-Funktion | G2 | Ausgang | reserviert; tatsächliche Board-Bestückung beachten |

**Bilanz:** 19 Funktions-GPIOs einschließlich Sensorik, I²C, SPI und SLEEP, zusätzlich zwei UART0-Pins und G2 für Diagnose. Keine Doppelbelegung. Die normalen, frei nutzbaren Ausgangspins sind damit verplant.

## A4988: RESET, SLEEP und definierte Startpegel

Die folgenden Widerstandswerte sind **Bestückungsvorschläge**, keine Messwerte des vorhandenen Shields. Vorhandene Pulls und Brücken zuerst erfassen; parallele bzw. gegensätzliche Widerstände nicht blind ergänzen.

| A4988-Netz | HW3-Beschaltung | Gewünschter Zustand ohne aktive Firmware |
|---|---|---|
| VDD | gemeinsame 3,3-V-Logikversorgung | definierte Logikversorgung |
| !RESET / RST | ca. 10 kΩ nach VDD; zugänglicher Testpunkt | HIGH, kein Reset |
| !SLEEP / SLP | G32; ca. 10 kΩ nach GND, vorhandenen Modul-Pull-up berücksichtigen | LOW, Treiber schläft |
| !ENABLE / EN | G27; ca. 10 kΩ nach VDD | HIGH, Ausgänge gesperrt |
| STEP | G25; ca. 10 kΩ nach GND | LOW |
| DIR | G26; ca. 10 kΩ nach GND, sofern nicht bereits vorhanden | definierte Grundrichtung |
| MS1 / MS2 / MS3 | G13 / G14 / G4 | interne A4988-Pulldowns; Modus erst nach Initialisierung verbindlich |
| VMOT | separat abschaltbare 12 V; mindestens 47 µF nahe VMOT/GND | beim Flashen/Umverdrahten AUS |
| GND | gemeinsame Signalmasse; Motorstrom mit geeignetem Rückweg | gemeinsamer Pegelbezug |

**RESET und SLEEP bleiben getrennt.** Keine Brücke beider Pins auf fest HIGH. RESET wird nicht mit ESP32-EN verbunden. G32/SLEEP darf am Shield nicht bereits fest auf VDD liegen; LOW-Pegel am tatsächlichen Treiberpin messen.

Geplanter Firmwareablauf: ENABLE HIGH und STEP LOW halten, SLEEP zunächst LOW; MS1–3 und DIR setzen; SLEEP kontrolliert HIGH setzen und **mindestens 1 ms vor dem ersten STEP** warten. ENABLE erst nach gültiger Betriebsfreigabe LOW. SLEEP ist keine normale STOP-/GO-Umschaltung: Aufwachen/Reset bzw. stromloser Motor können die Positionsgültigkeit beeinflussen; erneute Referenzierung entsprechend Motorzustand erzwingen.

**Grenze:** Pulls definieren hochohmige GPIOs, unterdrücken aber keine aktiv getriebenen Gegenpegel. Diese Beschaltung ist noch kein nachgewiesener Boot-/OTA-Interlock. VMOT-Trennung und Messungen nach #155/#161 bleiben erforderlich; bei OTA mit anliegendem VMOT ist der sichere Reset-/Bootpfad gesondert nachzuweisen.

## Sensoren: Eingangspins sinnvoll nutzen

G34/G35/G36/G39 besitzen keine internen Pull-ups/Pull-downs. Für Schaltkontakte oder bestätigte Open-Collector-/Open-Drain-Ausgänge sind beispielsweise **4,7 kΩ nach 3,3 V auf der ESP32-Seite** vorgesehen. Bei Push-Pull-Sensoren die tatsächliche Ausgangsschaltung beachten. Signalpegel immer 0…3,3 V; keine 5-V-Sensorausgänge direkt anschließen. In der Firmware `INPUT` statt vermeintlich wirksamem `INPUT_PULLUP` verwenden.

- G34/G35 sind für die Lichtschranken vorgesehen, damit eine spätere Flankenerfassung nicht an den besonderen G36/G39-Einschränkungen hängt.
- G36/G39 werden für Endschalter **als Pegel gelesen, ohne GPIO-Flankeninterrupts**. Grund: Espressif nennt Interrupt-Einschränkungen bei ADC-Nutzung bzw. WLAN/Bluetooth mit Schlafmodi.
- Der Endlagen-Schutzpfad muss auf Muninn unmittelbar vor weiteren STEP-Flanken prüfen; keine Abhängigkeit von Keyboard-Polling, Web, UART oder Core-0-Queues. Ein langsames UI-Polling ist hierfür nicht ausreichend. Implementierung und maximale Stopplatenz stehen noch aus.
- Kontaktart NO/NC und aktiv-HIGH/LOW werden anhand der vorhandenen Schalter festgelegt. Wo Öffner vorhanden sind, ist NC gegen GND mit Pull-up zweckmäßig: offen/Leitungsbruch = HIGH/Sperre. Das ist ein Vorschlag, keine Behauptung über die vorhandene Verdrahtung.
- Keine pauschalen großen Entstörkondensatoren an Referenzsignale ergänzen; Filterung muss zum erforderlichen Schaltzeitpunkt passen.

## I²C und SPI-Anschluss

Keyboard/OLED benötigen am ESP32 nur **3,3 V, GND, G21/SDA, G22/SCL**. Bus-Pull-ups liegen auf 3,3 V; vorhandene Modulwiderstände berücksichtigen. LED-Taster werden über den PCF8574 bedient und verbrauchen keine weiteren ESP32-GPIOs.

SPI-CS jeweils mit etwa **10 kΩ nach 3,3 V** auf inaktiv HIGH halten, auch wenn das Gebäude abgenommen ist. Beide CS vor Businitialisierung HIGH. Display-DC kann lokal mit etwa 10 kΩ nach GND definiert werden. MCP23S17-Adresspins und !RESET lokal definieren; sein INT wird für die geplanten Lichtausgänge nicht gebraucht. Keine 5-V-Pegel auf MISO.

### DE-9 für das Gebäude – Entwurf aus #165 mit GPIO-Zuordnung

| DE-9 Kontakt | Signal | ESP32-Zuordnung |
|---:|---|---|
| 1 | GND, Signalmasse | GND |
| 2 | Logikversorgung | 3,3 V |
| 3 | SCLK | G18 |
| 4 | MOSI | G23 |
| 5 | MISO | G19 |
| 6 | !CS_DISPLAY | G16 |
| 7 | !CS_EXPANDER | G17 |
| 8 | Display-DC | G33 |
| 9 | Reserve | **unbeschaltet**, kein CS3-GPIO zugeteilt |

Kontaktzahlen gelten nach Steckernummerierung; Ansicht auf Lötseite und Steckseite nicht verwechseln. **12 V für Gebäudelicht separat und unverwechselbar führen**, keine Last-Rückströme über die SPI-Signalmasse. Kein Hot-Plug vorgesehen. Endgültige Steckerschaltung erst mit den gewählten Modulen festlegen.

### Display-Reset ohne eigenen GPIO

Das Display darf einen RESET-Anschluss besitzen; es muss ihn nicht „weglassen“. Er wird lokal nach Herstellerangabe versorgt bzw. durch eine passende Power-on-Reset-Schaltung bedient. Software-Reset per SPI kann ergänzen, ersetzt aber nicht automatisch die vorgeschriebene Einschaltsequenz. Backlight wird lokal versorgt/geschaltet; es erhält hier keinen eigenen ESP32-PWM-Pin.

Eine Kopplung an **ESP32-EN/CHIP_PU** ist nur nach Prüfung von Pegeln, Last, Resetdauer und USB-Autoprogrammierung eine Option. EN ist kein gewöhnlicher GPIO, und ein Software-Neustart erzeugt dort nicht zwingend einen externen LOW-Puls. **Dieser Plan enthält deshalb keine direkte EN–Display-RESET-Brücke.**

## Freie und gesperrte Anschlüsse

| Anschluss | Verwendung |
|---|---|
| G12 | ausdrücklich frei lassen |
| G0 | Boot/Download, nicht für Peripherie verplanen |
| G5 / G15 | unbeschaltet; mögliche spätere Reserve nur nach Strapping-Prüfung |
| G6–G11 / CLK, SD0–SD3, CMD | Flash-Leitungen, keine Benutzer-I/Os |
| G37 / G38 | am vorhandenen WROOM-Header nicht verfügbar |
| EN | ESP32-Reset/Chipfreigabe, kein freier GPIO |
| 3V3 / 5V / GND | Versorgung, keine GPIOs |

Ein drittes CS ist **nicht zugesichert**. Dafür wäre eine gesonderte Entscheidung über G5/G15, den Diagnosepin oder eine Erweiterung nötig. Für den vereinbarten Umfang mit zwei SPI-Geräten reicht die Belegung.

## Umstellung und Nachweis

Die bisherige HW2-Motorbelegung bleibt erhalten. Frühere Bench-Beobachtungen: STEP am losen NEMA11 erprobt, DIR beobachtet, ENABLE-Pegel getestet; Mikroschrittpegel mit dem jeweils aktuellen Build nachmessen. Bench-Modus/Normalmodus siehe [MOTOR_TIMER_BENCH.md](../MOTOR_TIMER_BENCH.md). Das ist keine Freigabe der FY-Endlagen.

**Code-Ist:** [HW_Config.h](../../include/HW_Config.h) enthält noch die in der Haupttabelle genannten alten Sensorpins; SLEEP/CS/DC sind noch nicht umgesetzt. Nur die neue Tabelle zu verdrahten macht den alten Build nicht HW3-kompatibel.

- [ ] Sensor-Konstanten umstellen; Boardprofil HW2/HW3 oder gezielte Migration festlegen.
- [ ] SLEEP-Initialisierung, Aufwachzeit und Referenzgültigkeit implementieren.
- [ ] G36/G39 als Pegel im unmittelbaren Motor-Schutzpfad integrieren; Stopplatenz prüfen.
- [ ] CS/DC für #165 übernehmen, wenn die Erweiterung implementiert wird.
- [ ] Shield-Pulls, RESET/SLEEP-Trennung, Sensorpegel und tatsächliche Boardrevision prüfen.
- [ ] Versorgungsauswahl gemäß [HW3_STROMVERSORGUNG.md](HW3_STROMVERSORGUNG.md) aufbauen und messen.
- [ ] Power-on, Reset, USB-Flash und später OTA samt STEP/ENABLE/SLEEP am realen Aufbau prüfen.

**Prüfstand dieser Änderung:** GPIO-Eindeutigkeit, Ein-/Ausgangseignung, Anforderungen und Soll/Ist-Abweichungen dokumentarisch abgeglichen. Keine Firmwareänderung, kein Build und kein Hardwaretest. Zentrale offene Punkte: [VERIFICATION_GAPS.md](../VERIFICATION_GAPS.md).

## Technische Referenzen

- [ESP32 GPIO-Dokumentation: Eingangspins, Pulls, G36/G39-Einschränkungen](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
- [ESP32-WROOM-32-Datenblatt](https://documentation.espressif.com/esp32-wroom-32_datasheet_en.html)
- [AZ-Delivery Boardunterlagen](../../../Documents/Datasheets/ESP32_AZ_Delivery_38Pin_REFERENZEN.md)
- [Allegro A4988-Datenblatt, RESET/SLEEP/ENABLE](https://www.pololu.com/file/0J450/A4988.pdf)
- [Pololu A4988-Träger: Versorgung, Steuerpins und Entkopplung](https://www.pololu.com/product/1182) – Referenz für den Trägertyp, kein Nachweis der Bestückung unseres Shields.
