# ESP32-WROOM (38 Pin) – HW3-Gesamtbelegungsplan

**Stand: 23.10.2026 · Branch `feature/FY_ESP32_Port`.**
**HW3-Pinbelegung in `HW_Config.h` übernommen; Ansteuerung und elektrische Abnahme noch unvollständig.**
Die Haupttabelle entspricht den neuen Pin-Konstanten in [HW_Config.h](../../include/HW_Config.h). Die Statusspalte nennt zusätzlich die frühere HW2-Zuordnung. SLEEP, Sensor-Schutzpfad und SPI benötigen weiterhin Implementierung und Tests. Der vorhandene HW2-Bench-Nachweis gilt nicht automatisch für HW3.

**Software-Zuständigkeit:** Muninn / Kern 1 besitzt Motor, SLEEP, Sensoren und die bisherige Diagnose-LED. Huginn / Kern 0 besitzt I²C-Bedienung, SPI-Anzeige/Licht und UART-Kommunikation. Die Zuordnung beschreibt den Software-Owner; GPIOs sind nicht elektrisch an einen Kern gebunden. `CORE_MUNINN=1` und `CORE_HUGINN=0` sowie das Task-Pinning bleiben erhalten.

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

GPIO-Zahlen sind **GPIO-Nummern, keine Steckerpositionsnummern**. Die beiden reinen Eingänge GPIO36 und GPIO39 heißen im Espressif-Datenblatt **SENSOR_VP (SVP/VP)** und **SENSOR_VN (SVN/VN)**; auf manchen Boards steht **SP/SN**. Am vorhandenen AZ-Board wurden die Aufdrucke **„SN“ und „SV“** abgelesen. Welche physische Stiftposition welchem GPIO entspricht, wird vor dem Anschließen anhand des konkreten Board-Pinouts verifiziert – **nicht allein aus der Kurzbeschriftung abgeleitet**. Plan gilt für das vorhandene **WROOM ohne PSRAM**, nicht ungeprüft für WROVER oder andere ESP32-Varianten.

| Gruppe | Funktion / Anschluss | HW3 GPIO | Richtung am ESP32 | Code-Stand / Änderung gegenüber HW2 |
|---|---|---:|---|---|
| Motor | A4988 STEP | G25 | Ausgang | **10 kΩ Pull-down nach GND**; G25, unverändert |
| Motor | A4988 DIR | G26 | Ausgang | **10 kΩ Pull-down nach GND**; G26, unverändert |
| Motor | A4988 !ENABLE / EN | G27 | Ausgang | **10 kΩ Pull-up nach 3,3 V**; G27, unverändert; LOW = Wicklungen freigegeben |
| Motor | M0 → A4988 MS1 | G13 | Ausgang | G13, unverändert |
| Motor | M1 → A4988 MS2 | G14 | Ausgang | G14, unverändert |
| Motor | M2 → A4988 MS3 | G4 | Ausgang | G4, unverändert |
| Motor | A4988 !SLEEP / SLP, mit !RESET verbunden | **G32** | Ausgang | **10 kΩ Pull-down nach GND** am gemeinsamen SLP/RST-Netz; Ansteuerung offen; LOW = Schlafzustand/Reset |
| Sensor | Lichtschranke REF / LSREF | **G34** | nur Eingang | **10 kΩ Pull-up nach 3,3 V**; umgestellt von G32 |
| Sensor | Lichtschranke 2 / Riemen / LS4 | **G35** | nur Eingang | **10 kΩ Pull-up nach 3,3 V**; umgestellt von G33 |
| Sensor | Endschalter links / LSL | **G36 / SENSOR_VP** | nur Eingang | **10 kΩ Pull-up nach 3,3 V**; umgestellt von G16 |
| Sensor | Endschalter rechts / LSR | **G39 / SENSOR_VN** | nur Eingang | **10 kΩ Pull-up nach 3,3 V**; umgestellt von G17 |
| I²C | SDA – PCF8574 und OLED | G21 | bidirektional | **4,7 kΩ Pull-up nach 3,3 V am Bus**; G21, unverändert |
| I²C | SCL – PCF8574 und OLED | G22 | Open-Drain-Bus | **4,7 kΩ Pull-up nach 3,3 V am Bus**; G22, unverändert |
| SPI | SCLK – gemeinsam | G18 | Ausgang | Konstante reserviert; SPI-Ansteuerung offen |
| SPI | MISO – MCP23S17, ggf. Display | G19 | Eingang | Konstante reserviert; SPI-Ansteuerung offen |
| SPI | MOSI – gemeinsam | G23 | Ausgang | Konstante reserviert; SPI-Ansteuerung offen |
| SPI | !CS_DISPLAY | **G16** | Ausgang | **10 kΩ Pull-up nach 3,3 V**; SPI-Ansteuerung offen |
| SPI | !CS_EXPANDER | **G17** | Ausgang | **10 kΩ Pull-up nach 3,3 V**; SPI-Ansteuerung offen |
| SPI | Display D/C | **G33** | Ausgang | **10 kΩ Pull-down nach GND**; SPI-Ansteuerung offen |
| Service | UART0 TX → USB-UART | G1 / TX0 | Ausgang | für USB/UART0 freihalten |
| Service | UART0 RX ← USB-UART | G3 / RX0 | Eingang | für USB/UART0 freihalten |
| Diagnose | bisherige Test-LED-Funktion | G2 | Ausgang | reserviert; tatsächliche Board-Bestückung beachten |

**Bilanz:** 19 Funktions-GPIOs einschließlich Sensorik, I²C, SPI und SLEEP, zusätzlich zwei UART0-Pins und G2 für Diagnose. Keine Doppelbelegung. Die normalen, frei nutzbaren Ausgangspins sind damit verplant.

## Verbindliche HW3-Pull-Bestückung (Soll-Schaltung)

**Die Widerstände in dieser Tabelle sind als HW3-Bestückung festgelegt.** Sie sind damit eine **Verdrahtungsvorgabe**, keine Behauptung, dass sie bereits eingelötet oder elektrisch abgenommen sind. Die auf den zugekauften Modulen vorhandenen Widerstände bleiben **unvermessen**; ihre Wirkung wird beim HW3-Abnahmetest kontrolliert.

| Netz / ESP32-GPIO | Externe HW3-Beschaltung | Pegel bei hochohmigem ESP32 |
|---|---|---|
| A4988 !ENABLE / G27 | **10 kΩ Pull-up an 3,3 V** | HIGH: Motorausgänge gesperrt |
| A4988 STEP / G25 | **10 kΩ Pull-down an GND** | LOW: keine STEP-Flanke |
| A4988 DIR / G26 | **10 kΩ Pull-down an GND** | LOW: definierte Grundrichtung |
| A4988 !SLEEP + !RESET / G32 | **SLP und RST miteinander verbinden; gemeinsamer 10 kΩ Pull-down an GND** | LOW: Treiber schläft und bleibt im Reset |
| Lichtschranke REF / G34 | **10 kΩ Pull-up an 3,3 V** | HIGH |
| Lichtschranke 2 / G35 | **10 kΩ Pull-up an 3,3 V** | HIGH |
| Endschalter links / G36 | **10 kΩ Pull-up an 3,3 V** | HIGH |
| Endschalter rechts / G39 | **10 kΩ Pull-up an 3,3 V** | HIGH |
| I²C SDA / G21 | **4,7 kΩ Pull-up an 3,3 V, einmal je Bus** | HIGH |
| I²C SCL / G22 | **4,7 kΩ Pull-up an 3,3 V, einmal je Bus** | HIGH |
| SPI !CS_DISPLAY / G16 | **10 kΩ Pull-up an 3,3 V** | HIGH: Display abgewählt |
| SPI !CS_EXPANDER / G17 | **10 kΩ Pull-up an 3,3 V** | HIGH: MCP23S17 abgewählt |
| SPI Display D/C / G33 | **10 kΩ Pull-down an GND** | LOW: definierter Startpegel |

**I²C:** 4,7 kΩ sind der festgelegte **Zielwert je Busleitung**, nicht zusätzlich blind einzulötende Widerstände. Haben OLED/PCF8574 bereits wirksame Pull-ups, werden diese für den Gesamtwiderstand berücksichtigt; der tatsächlich erreichte Bus-Pull-up wird vor der Bestückung gemessen und dokumentiert.

**A4988:** Die R100-Aufdrucke bezeichnen die beiden **Strommesswiderstände (je 0,1 Ω)** und sagen nichts über die Pull-Widerstände aus. Beim Pololu-Referenzcarrier sind 100 kΩ Pull-up an !SLEEP und 100 kΩ Pull-down an !ENABLE beschrieben. Für unser R100-Modul sind diese Werte **nicht nachgewiesen**. Die oben genannten 10-kΩ-Widerstände sind trotzdem die verbindliche HW3-Soll-Beschaltung; die tatsächlichen Pegel und die Wirksamkeit werden gemessen. !RESET ist **nicht** mit ESP32-EN verbunden.

**A4988-Grundzustand:** VDD = 3,3 V; VMOT = separat schaltbare 12 V, mit mindestens 47 µF nahe VMOT/GND. G32 hält !SLEEP und !RESET gemeinsam LOW. Erst nach sicherem Setzen von STEP LOW, ENABLE HIGH, DIR und MS1–3 setzt die Firmware G32 auf HIGH, wartet **mindestens 1 ms** und darf anschließend nach Betriebsfreigabe ENABLE auf LOW setzen. **Die SLEEP-Ansteuerung ist im Code noch nicht implementiert; mit dem geplanten Pull-down läuft der Motor bis dahin nicht.**

**Sicherheitsgrenze:** Pull-Widerstände bestimmen Pegel nur bei hochohmigen Ausgängen; sie überstimmen keinen aktiv falsch getriebenen ESP32-Pin. Die HW3-Beschaltung ist noch kein geprüfter Boot-/Flash-/OTA-Interlock. VMOT-Trennung und Messungen nach [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155) / [#161](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/161) bleiben erforderlich.

## Sensoren: Eingangspins sinnvoll nutzen

G34/G35/G36/G39 besitzen keine internen Pull-ups/Pull-downs. **Jeder der vier Eingänge erhält in HW3 einen eigenen externen 10-kΩ-Pull-up nach 3,3 V auf der ESP32-Seite.** Die Sensorausgänge werden vor dem Anschluss auf 3,3-V-Verträglichkeit geprüft; bei aktivem Push-Pull-Ausgang muss der HIGH/LOW-Treiber die Beschaltung sicher vertragen. Signalpegel immer 0…3,3 V; keine 5-V-Sensorausgänge direkt anschließen. In der Firmware `INPUT` statt vermeintlich wirksamem `INPUT_PULLUP` verwenden.

- G34/G35 sind für die Lichtschranken vorgesehen, damit eine spätere Flankenerfassung nicht an den besonderen G36/G39-Einschränkungen hängt.
- G36/G39 werden für Endschalter **als Pegel gelesen, ohne GPIO-Flankeninterrupts**. Grund: Espressif nennt Interrupt-Einschränkungen bei ADC-Nutzung bzw. WLAN/Bluetooth mit Schlafmodi.
- Der Endlagen-Schutzpfad muss auf Muninn unmittelbar vor weiteren STEP-Flanken prüfen; keine Abhängigkeit von Keyboard-Polling, Web, UART oder Core-0-Queues. Ein langsames UI-Polling ist hierfür nicht ausreichend. Implementierung und maximale Stopplatenz stehen noch aus.
- Kontaktart NO/NC und aktiv-HIGH/LOW werden anhand der vorhandenen Schalter festgelegt. Für die Endschalter ist **NC gegen GND mit den festgelegten Pull-ups** das vorgesehene Schutzprinzip: offen/Leitungsbruch = HIGH/Sperre. Ob die vorhandenen Schalter als Öffner verdrahtet sind, wird vor Inbetriebnahme geprüft; bis dahin ist die elektrische Funktionsprüfung offen.
- Keine pauschalen großen Entstörkondensatoren an Referenzsignale ergänzen; Filterung muss zum erforderlichen Schaltzeitpunkt passen.

## I²C und SPI-Anschluss

Keyboard/OLED benötigen am ESP32 nur **3,3 V, GND, G21/SDA, G22/SCL**. **SDA und SCL erhalten je einen wirksamen 4,7-kΩ-Bus-Pull-up nach 3,3 V (Sollwert); vorhandene Modulwiderstände werden angerechnet und nicht blind parallel bestückt.** LED-Taster werden über den PCF8574 bedient und verbrauchen keine weiteren ESP32-GPIOs.

**Beide SPI-CS erhalten je 10 kΩ Pull-up nach 3,3 V** und bleiben dadurch auch ohne Gebäude auf inaktiv HIGH. Beide CS vor Businitialisierung HIGH. **Display-DC erhält 10 kΩ Pull-down nach GND.** MCP23S17-Adresspins und !RESET lokal definieren; sein INT wird für die geplanten Lichtausgänge nicht gebraucht. Keine 5-V-Pegel auf MISO.

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

Die bisherige HW2-Motorbelegung bleibt erhalten. **HW3-Änderung 23.10.2026:** Verbindliche Pull-up-/Pull-down-Bestückung ergänzt; !RESET/!SLEEP als gemeinsames G32-Netz festgelegt; Sensor-Pin-Aliase dokumentiert. Frühere Bench-Beobachtungen: STEP am losen NEMA11 erprobt, DIR beobachtet, ENABLE-Pegel getestet; Mikroschrittpegel mit dem jeweils aktuellen Build nachmessen. Bench-Modus/Normalmodus siehe [MOTOR_TIMER_BENCH.md](../MOTOR_TIMER_BENCH.md). Das ist keine Freigabe der FY-Endlagen.

**Code-Ist:** [HW_Config.h](../../include/HW_Config.h) enthält die HW3-Sensorpins und Konstanten für SLEEP sowie SPI einschließlich beider CS und DC. Die Datei ist nach Muninn/Kern 1 und Huginn/Kern 0 gegliedert. `MOTOR_SLEEP_PIN` allein steuert den Ausgang noch nicht: Bei angeschlossenem SLEEP-Pulldown bleibt der A4988 mit der bisherigen Laufzeitlogik im Schlafzustand. Sensor-Schutzpfad und SPI-Ansteuerung fehlen ebenfalls weiterhin. Alte HW2-Verdrahtung und neue Sensor-Konstanten dürfen nicht als kompatibel angenommen werden.

- [x] Sensor-Konstanten gezielt auf HW3 umgestellt; SLEEP und SPI-Steuersignale definiert, nach Kernen gegliedert.
- [ ] SLEEP-Initialisierung, Aufwachzeit und Referenzgültigkeit implementieren.
- [ ] G36/G39 als Pegel im unmittelbaren Motor-Schutzpfad integrieren; Stopplatenz prüfen.
- [x] SPI einschließlich CS/DC als Konstanten für #165 übernommen.
- [ ] SPI-Initialisierung und tatsächliche Geräteansteuerung für #165 implementieren.
- [ ] A4988-Modul-Pulls, gemeinsame RESET/SLEEP-Verdrahtung, externe HW3-Pulls, Sensorpegel und tatsächliche Boardrevision messen und prüfen.
- [ ] Versorgungsauswahl gemäß [HW3_STROMVERSORGUNG.md](HW3_STROMVERSORGUNG.md) aufbauen und messen.
- [ ] Power-on, Reset, USB-Flash und später OTA samt STEP/ENABLE/SLEEP am realen Aufbau prüfen.

**Prüfstand dieser Änderung:** Pin-Konstanten und Kernzuständigkeiten gegen Plan und vorhandenes Task-Pinning abgeglichen; keine GPIO-Doppelbelegung. Isolierte C++11-Syntaxprüfung des Headers mit g++ und minimalem Arduino-Typstub bestanden. Kein vollständiger ESP32-/PlatformIO-Build (Toolchain hier nicht installiert), keine MISRA-/Cppcheck-Prüfung und kein Hardwaretest. Laufzeitlogik unverändert. Zentrale offene Punkte: [VERIFICATION_GAPS.md](../VERIFICATION_GAPS.md).

## Technische Referenzen

- [ESP32 GPIO-Dokumentation: Eingangspins, Pulls, G36/G39-Einschränkungen](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
- [ESP32-WROOM-32-Datenblatt](https://documentation.espressif.com/esp32-wroom-32_datasheet_en.html)
- [AZ-Delivery Boardunterlagen](../../../Documents/Datasheets/ESP32_AZ_Delivery_38Pin_REFERENZEN.md)
- [Allegro A4988-Datenblatt, RESET/SLEEP/ENABLE](https://www.pololu.com/file/0J450/A4988.pdf)
- [Pololu A4988-Träger: Versorgung, Steuerpins und Entkopplung](https://www.pololu.com/product/1182) – Referenz für den Trägertyp, kein Nachweis der Bestückung unseres Shields.
