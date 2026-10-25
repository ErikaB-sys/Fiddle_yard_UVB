# ESP32-WROOM (38 Pin) – HW3-Gesamtbelegungsplan

**Stand: 23.10.2026 · Branch `feature/FY_ESP32_Port`.**
**HW3-Pinbelegung in `HW_Config.h` übernommen; Ansteuerung und elektrische Abnahme noch unvollständig.**
Die Haupttabelle entspricht den neuen Pin-Konstanten in [HW_Config.h](../../include/HW_Config.h). Die Statusspalte nennt zusätzlich die frühere HW2-Zuordnung. SLEEP wird im Setup auf LOW gesetzt und am Ende von `Muninn_initMotorMock()` nach der Motorinitialisierung **einmalig HIGH gesetzt**. Diese Startsequenz wurde am 23.10.2026 an HW3 beim Motorlauf beobachtet; Reset-/Boot-Pegelmessungen stehen weiter aus. Sensor-Schutzpfad und SPI benötigen weiterhin Implementierung und Tests. Der vorhandene HW2-Bench-Nachweis gilt nicht automatisch für HW3.

**Software-Zuständigkeit:** Muninn / Kern 1 besitzt Motor, SLEEP, Sensoren und die bisherige Diagnose-LED. Huginn / Kern 0 besitzt I²C-Bedienung, SPI-Anzeige/Licht und UART-Kommunikation. Die Zuordnung beschreibt den Software-Owner; GPIOs sind nicht elektrisch an einen Kern gebunden. `CORE_MUNINN=1` und `CORE_HUGINN=0` sowie das Task-Pinning bleiben erhalten.

**Verbindliche Orientierung (HW_TEST → HW3, physisch geprüft 25.10.2026):** **LINKS ist die Motorseite**, **RECHTS die vom Motor abgewandte Seite**. Nicht aus der Blickrichtung des Bedieners ableiten. Nano **D12 / LSL → ESP32 G36 (VP)** = Endschalter **an der Motorseite**; Nano **D9 / LSR → ESP32 G39 (VN)** = Endschalter **gegenüber**; Nano **D11 / LSREF → ESP32 G34** = Referenzlichtschranke. G35/LS4 ist reserviert und noch nicht angeschlossen. Beim HW3-Test ergaben die Rohbits 0x01 (Motorseite/LSL), 0x02 (gegenüber/LSR), 0x04 (LSREF). **Keine Leitungen tauschen.**

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
| Motor | ⚪ A4988 STEP | **G27** | Ausgang | **kein zusätzlicher Pull**; weiße STEP-Diagnose-LED (gemeinsames Plus, LOW = an); HW3-Tausch von G25 |
| Motor | 🟡 A4988 DIR | G26 | Ausgang | **kein externer Pull-down in HW3-Testbestückung**; gelbe DIR-Diagnose-LED |
| Motor | 🔵 A4988 !ENABLE / EN | **G25** | Ausgang | **kein zusätzlicher Pull**; blaue EN-Diagnose-LED (LOW = an); HW3-Tausch von G27 |
| Motor | M0 → A4988 MS1 | G13 | Ausgang | G13, unverändert |
| Motor | M1 → A4988 MS2 | G14 | Ausgang | G14, unverändert |
| Motor | M2 → A4988 MS3 | G4 | Ausgang | G4, unverändert |
| Motor | 🔴 A4988 !SLEEP / SLP, mit !RESET verbunden | **G32** | Ausgang | **10 kΩ Pull-down nach GND**; rote LED bei HIGH an (Anode an G32); Resetpegel noch nachzuweisen |
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

**Die Pull-Widerstände in dieser Tabelle sind als HW3-Bestückung festgelegt, soweit nicht ausdrücklich als weggelassen gekennzeichnet.** Sie sind damit eine **Verdrahtungsvorgabe**, keine Behauptung, dass sie bereits eingelötet oder elektrisch abgenommen sind. Am A4988-Modul wurde der Pull-up von !SLEEP nach 3,3 V mit **ca. 97 kΩ** gemessen. Die übrigen Modulwiderstände und die Wirksamkeit der gesamten HW3-Beschaltung werden beim HW3-Abnahmetest kontrolliert.

| Netz / ESP32-GPIO | Externe HW3-Beschaltung | Pegel bei hochohmigem ESP32 |
|---|---|---|
| A4988 !ENABLE / G25 | **kein zusätzlicher Pull-up in der HW3-Testbestückung** | während Boot hochohmig; Motorausgänge werden durch LOW auf !SLEEP/!RESET gesperrt, sofern LOW elektrisch nachgewiesen |
| A4988 STEP / G27 | **kein zusätzlicher Pull-down in der HW3-Testbestückung** | im Bootzustand ohne Firmware nicht garantiert definiert; elektrisch prüfen |
| A4988 DIR / G26 | **kein zusätzlicher Pull-down in der ersten HW3-Bestückung** | im Bootzustand ohne Firmware nicht garantiert definiert; elektrisch prüfen |
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

## HW3-Motortreiber: Diagnose-LEDs und Farben

**HW3-Istbestückung (23.10.2026): Rot ist aktiv HIGH, die drei übrigen LEDs sind aktiv LOW.** Jede LED hat einen eigenen Vorwiderstand. Es werden keine separaten Diagnose-GPIOs benötigt.

| LED-Farbe | Signal | GPIO (HW3) | Anzeige bei leuchtender LED | Anschluss |
|---|---|---|---|---|
| **🔴 Rot** | !SLEEP + !RESET | **G32** | **HIGH: A4988 wach / Reset aufgehoben** | G32 → LED (Anode) → Vorwiderstand → GND; **LOW = AUS, Treiber schläft** |
| **🔵 Blau** | !ENABLE / EN | **G25** | **LOW: Endstufen freigegeben** | +3,3 V → Vorwiderstand und LED → G25 |
| **🟡 Gelb** | DIR | **G26** | **LOW: Richtungssignal LOW** | +3,3 V → Vorwiderstand und LED → G26 |
| **⚪ Weiß** | STEP | **G27** | **LOW: STEP LOW**; bei Ruhe ggf. dauerhaft AN | +3,3 V → Vorwiderstand und LED → G27 |

**Erwarteter Ablauf:** Im Reset und zu Beginn von `setup()` ist G32 durch den **10-kΩ-Pull-down** LOW (Rot AUS). `Muninn_initMotorMock()` initialisiert Motorpins/Mikroschritte/Timer, setzt **G32 einmalig auf HIGH (Rot AN)** und wartet **1,5 ms** vor Taskstart. Beim Start bleiben STOP verriegelt und ENABLE HIGH (Blau AUS). `STOP AA 55` entriegelt; `GO AA 55` setzt !ENABLE LOW (Blau AN). GO OFF und STOP schalten ENABLE wieder HIGH, nicht SLEEP. Die Timer-/STEP-ISR bleibt unverändert.

Die rote LED liegt **nicht von fest +3,3 V nach G32** und speist daher nicht wie zuvor fälschlich angenommen Strom in den 10-kΩ-Pull-down ein. Der am A4988 vorhandene **ca. 97-kΩ-Pull-up nach 3,3 V** wirkt hingegen weiterhin gegen diesen Pull-down; den tatsächlichen **Reset-/Boot-LOW-Pegel an G32** vor VMOT mit 3,3-V-Logikversorgung messen. Eine Anzeige-LED allein ist kein Nachweis des Pegels. Siehe [#153](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/153) und [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155).

Die weiße STEP-LED belastet das Pulssignal; Timing und HIGH/LOW-Pegel am A4988 prüfen. LED-Vorwiderstände sind keine Pull-Widerstände.

**I²C:** 4,7 kΩ sind der festgelegte **Zielwert je Busleitung**, nicht zusätzlich blind einzulötende Widerstände. Haben OLED/PCF8574 bereits wirksame Pull-ups, werden diese für den Gesamtwiderstand berücksichtigt; der tatsächlich erreichte Bus-Pull-up wird vor der Bestückung gemessen und dokumentiert.

**A4988:** Die R100-Aufdrucke bezeichnen die beiden **Strommesswiderstände (je 0,1 Ω)** und sagen nichts über die Pull-Widerstände aus. Beim Pololu-Referenzcarrier sind 100 kΩ Pull-up an !SLEEP und 100 kΩ Pull-down an !ENABLE beschrieben. Auf unserem R100-Modul wurde der !SLEEP-Pull-up nach 3,3 V inzwischen mit **ca. 97 kΩ** gemessen; die übrigen Modul-Pulls sind nicht abschließend nachgewiesen. Die **am G32-Netz und an den Sensoren vorgesehenen 10-kΩ-Widerstände** bleiben HW3-Soll-Beschaltung; für G25/G26/G27 werden vorerst **keine zusätzlichen Pulls** eingesetzt. Die tatsächlichen Pegel und ihre Wirksamkeit sind am aufgebauten HW3 zu prüfen. !RESET ist **nicht** mit ESP32-EN verbunden.

**A4988-Start und Betrieb (HW3-Softwarestand 23.10.2026):** VDD = 3,3 V; VMOT = separat schaltbare 12 V, mit mindestens 47 µF nahe VMOT/GND. In `setup()` wird G32 zunächst LOW als OUTPUT initialisiert; STEP G27 LOW, ENABLE G25 HIGH, DIR G26 und MS1–3 werden initialisiert. Am Ende von `Muninn_initMotorMock()` wird **G32 einmalig HIGH** gesetzt und anschließend **1,5 ms** gewartet, bevor die Motor-/UART-Tasks angelegt werden. Die Wartezeit liegt außerhalb der ISR und garantiert bei erfolgreicher Initialisierung die erforderliche A4988-Aufwachzeit vor einer STEP-Flanke. `STOP AA 55` entriegelt die POR-STOP-Sperre, setzt ENABLE jedoch nicht LOW. `GO AA 55` setzt ENABLE LOW (Freigabe); `GO 55 AA` und beide `STOP`-Varianten setzen ENABLE HIGH und stoppen STEP. **G32 bleibt bei GO/STOP HIGH**: Sleep/Reset wird nur als Startabsicherung eingesetzt. **Auf HW3 am 23.10.2026 mit losem Motor in Betrieb genommen; vollständige Boot-/Flash-/Reset-Sicherheitsabnahme bleibt offen.**

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

**HW3-Funktionsprüfung vom 23.10.2026:** 12 V / DC/DC-3,3-V-Versorgung, galvanisch getrennter USB-UART, G32-SLEEP-/RESET-Aufwachfolge, G25-ENABLE, G26-DIR und G27-STEP am losen A4988-Motor erfolgreich betrieben; 1/4- und 1/16-Schritte drehten, 1/16 hörbar ruhiger. 350 µF / 25 V nahe A4988-VMOT ergänzt. Firmware-Upload bei eingeschalteten 12 V verlief im beobachteten Versuch ohne Motorzucken. **Keine Aussage zur generellen Flash-/Reset-/OTA-Sicherheit**; Referenzsensoren, Endlagen und mechanische Fahrfreigabe weiterhin ungetestet.



Die bisherige HW2-Motorbelegung bleibt im Nano-/HW2-Stand erhalten. **HW3-Änderung 23.10.2026:** STEP=G27 und !ENABLE=G25 gemäß physischer HW3-Verdrahtung; DIR=G26 unverändert; !RESET/!SLEEP als gemeinsames G32-Netz, Sensor-Pin-Aliase und die unterschiedliche LED-Polarität (Rot HIGH, übrige LOW) dokumentiert. Frühere Bench-Beobachtungen: STEP am losen NEMA11 erprobt, DIR beobachtet, ENABLE-Pegel getestet; Mikroschrittpegel mit dem jeweils aktuellen Build nachmessen. Bench-Modus/Normalmodus siehe [MOTOR_TIMER_BENCH.md](../MOTOR_TIMER_BENCH.md). Das ist keine Freigabe der FY-Endlagen.

**Code-Ist (HW3-Motorpfad 23.10. bench-bestätigt):** [HW_Config.h](../../include/HW_Config.h) enthält die HW3-Sensorpins und Konstanten für SLEEP sowie SPI einschließlich beider CS und DC. Die Datei ist nach Muninn/Kern 1 und Huginn/Kern 0 gegliedert. `MOTOR_SLEEP_PIN` wird in `setup()` zunächst LOW initialisiert und in `Muninn_initMotorMock()` einmalig freigegeben; GO/STOP verändern G32 nicht. Die komplette elektrische Abnahme, insbesondere gemessene Boot-/Reset-Pegel, ist offen; Motor-/GO-/STOP-Funktionsnachweis am losen Motor ist gelungen. Sensor-Schutzpfad und SPI-Ansteuerung fehlen weiterhin. Alte HW2-Verdrahtung und neue Sensor-Konstanten dürfen nicht als kompatibel angenommen werden.

- [x] Sensor-Konstanten gezielt auf HW3 umgestellt; SLEEP und SPI-Steuersignale definiert, nach Kernen gegliedert.
- [x] SLEEP-Initialisierung und einmalige Freigabe in `Muninn_initMotorMock()` mit 1,5 ms Aufwachzeit vor Taskstart implementiert (nicht ISR); GO/STOP verändern SLEEP nicht.
- [ ] SLEEP-Pegel vor/nach Initialisierung und Aufwachzeit an HW3 prüfen; den tatsächlichen hochohmigen G32-Bootpegel nachweisen; Referenzgültigkeit nach Reset prüfen.
- [ ] G36/G39 als Pegel im unmittelbaren Motor-Schutzpfad integrieren; Stopplatenz prüfen.
- [x] SPI einschließlich CS/DC als Konstanten für #165 übernommen.
- [ ] SPI-Initialisierung und tatsächliche Geräteansteuerung für #165 implementieren.
- [ ] A4988-Modul-Pulls (SLEEP: ca. 97 kΩ gemessen), gemeinsame RESET/SLEEP-Verdrahtung, externe HW3-Pulls, Sensorpegel und tatsächliche Boardrevision prüfen.
- [ ] Vier Diagnose-LEDs Rot (HIGH=AN), Blau/Gelb/Weiß (LOW=AN) samt Pegeln, Vorwiderständen und STEP-Signalbelastung überprüfen; Boot-Sicherheit ohne zusätzliche STEP-/DIR-/ENABLE-Pulls messen.
- [ ] Versorgungsauswahl gemäß [HW3_STROMVERSORGUNG.md](HW3_STROMVERSORGUNG.md) aufbauen und messen.
- [ ] Power-on, Reset, USB-Flash und später OTA samt STEP/ENABLE/SLEEP am realen Aufbau prüfen.

**Prüfstand dieser Änderung:** HW3-GPIO-Konstanten, ISR-Bitmaske und Sleep-Zustandsfolge auf Quelltextebene abgeglichen; `StepTimer.cpp` verwendet `MOTOR_PWM_PIN` und wurde nicht geändert. **Neuer Firmwarestand wurde am 23.10.2026 auf HW3 hochgeladen und am losen Motor getestet; keine aktualisierte MISRA-/Cppcheck-Prüfung und keine vollständige elektrische Sicherheitsabnahme.** Änderungen in `HW_Config.h`, `main.cpp` und `Motor.cpp`; Nano-/HW2-Baseline unverändert. Zentrale offene Punkte: [VERIFICATION_GAPS.md](../VERIFICATION_GAPS.md).

## Technische Referenzen

- [ESP32 GPIO-Dokumentation: Eingangspins, Pulls, G36/G39-Einschränkungen](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
- [ESP32-WROOM-32-Datenblatt](https://documentation.espressif.com/esp32-wroom-32_datasheet_en.html)
- [AZ-Delivery Boardunterlagen](../../../Documents/Datasheets/ESP32_AZ_Delivery_38Pin_REFERENZEN.md)
- [Allegro A4988-Datenblatt, RESET/SLEEP/ENABLE](https://www.pololu.com/file/0J450/A4988.pdf)
- [Pololu A4988-Träger: Versorgung, Steuerpins und Entkopplung](https://www.pololu.com/product/1182) – Referenz für den Trägertyp, kein Nachweis der Bestückung unseres Shields.
