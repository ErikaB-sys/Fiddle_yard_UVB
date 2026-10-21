# FY Keyboard – Pinout der Taster und LEDs

Quelle: `FY_HW_TEST/FY_HW_TEST/include/HW_Test.h` (Hardwaretest, 21.10.2026).
Die Tabelle gibt die **tatsächlich definierten Konstanten** wieder.

## I²C-Portexpander

- Baustein: PCF8574
- I²C-Adresse: `0x27`
- Vier Tastereingänge und vier LED-Ausgänge.

| Bedienelement | Taster-Eingang | LED-Ausgang | Konstanten |
| --- | --- | --- | --- |
| STOP | P7 | P3 | `BUTTON_STOP_PIN`, `LED_STOP_PIN` |
| Links / Blau 1 | P6 | P2 | `BUTTON_LEFT_PIN`, `LED_LEFT_PIN` |
| GO / Grün | P5 | P1 | `BUTTON_GO_PIN`, `LED_GO_PIN` |
| Rechts / Blau 2 | P4 | P0 | `BUTTON_RIGHT_PIN`, `LED_RIGHT_PIN` |

### Portübersicht

| PCF8574-Port | Verwendung |
| --- | --- |
| P0 | LED Rechts |
| P1 | LED GO / Grün |
| P2 | LED Links |
| P3 | LED STOP |
| P4 | Taster Rechts / Blau 2 |
| P5 | Taster GO / Grün |
| P6 | Taster Links / Blau 1 |
| P7 | Taster STOP |

> **Achtung, Quellcode-Unstimmigkeit:** Der Kommentar direkt oberhalb der Konstanten in `HW_Test.h` behauptet `STOP → LED P1` und `GREEN → LED P3`. Die Konstanten legen dagegen `LED_STOP_PIN = P3` und `LED_GO_PIN = P1` fest. Diese Dokumentation folgt den Konstanten. Beim nächsten Hardwaretest die reale LED-Zuordnung bestätigen und den veralteten Kommentar korrigieren.

## Bedienung im Hardwaretest

- STOP: Bewegung anhalten.
- Links / Rechts: lokale Bewegungsrichtung; bei deaktivierter Motorfreigabe auch Auswahl der Testfrequenz (Blau 1 niedriger, Blau 2 höher).
- GO: Motorfreigabe/Start entsprechend dem HW-Test-Ablauf.

## ESP32-Übernahme

Diese Zuordnung betrifft die **PCF8574-Ports**, nicht die GPIO-Nummern des ESP32. Sie kann deshalb bei gleichbleibender Keyboard-Platine übernommen werden. ESP32-SDA/SCL und die konkrete I²C-Initialisierung sind separat in der ESP32-Hardwarekonfiguration festzulegen. Neue Features werden nur für den ESP32 entwickelt.

Siehe `FY_ESP32_Bringup/lib/Keyboard/Keyboard.md` im Branch `feature/FY_ESP32_Port`.
