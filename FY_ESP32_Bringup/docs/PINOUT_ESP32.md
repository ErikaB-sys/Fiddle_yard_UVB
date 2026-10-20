# ESP32-WROOM (38 Pin) – HW2 Pinout

**Stand: 20.10.2026, Branch `feature/FY_ESP32_Port`.** Die nachfolgend als „verdrahtet“ angegebenen Verbindungen wurden an HW2 hergestellt. Nicht alle Anschluesse wurden bereits elektrisch oder mit laufendem Motor getestet.

## Motor / A4988 auf HW2

| Funktion | ESP32-Pin | Treiber-Eingang | Status |
|---|---:|---|---|
| STEP | G25 | STEP | Verdrahtet / Boot-GPIO-Test, keine Motor-ISR |
| DIR | G26 | DIR | Verdrahtet / Boot-GPIO-Test |
| ENABLE | G27 | EN (LOW = Treiber ein) | Verdrahtet / GPIO-Pegel getestet |
| M0 | G13 | MS1 | Neu verdrahtet; 1/16-Konfiguration noch testen |
| M1 | G14 | MS2 | Neu verdrahtet; 1/16-Konfiguration noch testen |
| M2 | G4 | MS3 | Neu verdrahtet; 1/16-Konfiguration noch testen |

`Muninn_initMotorMock()` setzt bei deaktiviertem Treiber vor den FreeRTOS-Tasks alle drei Mikroschritt-Ausgaenge auf HIGH. **MS1=MS2=MS3=HIGH bedeutet 1/16-Mikroschrittauflösung.** Der Wert ist zunaechst fest fuer den Bring-up. Keine dynamische Umschaltung waehrend des Motorbetriebs.

**Boot-Hinweis:** G4 wird beim ESP32 als Strap-Status erfasst, ist laut Espressif aber fuer die unterstuetzten Bootmodi nicht entscheidend. Vor Betrieb dennoch Power-on/Reset/Flash mit der realen HW2-Verdrahtung pruefen. Die A4988-MS-Eingaenge haben interne Pulldowns: Vor Initialisierung der Firmware ist die 1/16-Konfiguration nicht garantiert.

## Andere Anschluesse (geplant / noch nicht vollstaendig integriert)

| Funktion | GPIO | Status |
|---|---:|---|
| Lichtschranke REF | G32 | geplant |
| Lichtschranke 2 / Riemen | G33 | geplant |
| Endschalter links | G16 | geplant |
| Endschalter rechts | G17 | geplant |
| I2C SDA | G21 | fuer spaetere Tastatur/OLED reserviert |
| I2C SCL | G22 | fuer spaetere Tastatur/OLED reserviert |
| SPI SCLK / MISO / MOSI | G18 / G19 / G23 | fuer Erweiterung reserviert |
| Onboard-Test-LED | G2 | getestet |

Die Pin-Konstanten im Code stehen verbindlich in `FY_ESP32_Bringup/include/HW_Config.h`.

## Sicherheits- und Versorgungsregeln

- Motorversorgung (VMOT) **vor Reset, Flashen und Umverdrahten abschalten**. Die Firmware kontrolliert GPIO-Pegel nicht vor `setup()`.
- A4988-Logikversorgung VDD muss zur ESP32-Logik (3,3 V) passen; gemeinsame GND-Verbindung sicherstellen. Keine 12 V auf den ESP32.
- Hardware-Failsafe fuer ENABLE (beispielsweise passender Pull-up nach VDD) ist noch nicht als getestet abgenommen.
- A4988-Stromgrenze ueber VREF und Sense-Widerstaende einstellen; Netzteilstrom allein ist kein Wicklungsstrom.
- MS1/MS2/MS3 nach dem Start mit Multimeter auf ~3,3 V pruefen, G27 bis zu ausdruecklichem GO auf HIGH; STEP ist ausserhalb des aktuellen Boot-Tests LOW.
- Der Motor-STEP-Timer und Sensor-/Endlagenabsicherung sind weiterhin **nicht implementiert**. Keine Bewegungstestfahrt an der FY-Mechanik ohne die vereinbarten Begrenzungen.
