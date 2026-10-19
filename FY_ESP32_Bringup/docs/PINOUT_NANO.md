# Nano – bekannte Pinbelegung aus Produktivcode

Quelle: `FY_Controller_Motor/include/HW_Config.h` im gesicherten Nano-`main` (19.10.2026). **HW2-Shield-Verdrahtung separat am Objekt kontrollieren**: Der Code beschreibt keine vollständige Steckplatine.

| Funktion | Arduino-Nano-Pin | AVR-Portbit in HW_Config.h |
|---|---|---|
| Motor STEP/PWM (weiß) | D3 | PD3 |
| Motor DIR (gelb) | D4 | PD4 |
| Motor ENABLE (blau) | D2 | PD2 |
| Referenzsensor | D9 | PB1 |
| Trim links | D10 | PB2 |
| Trim rechts | D11 | PB3 |
| Timing-Belt-Sensor | D12 | PB4 |
| I²C SDA | A4 | – |
| I²C SCL | A5 | – |

I²C-Geräte: OLED 0x3C, PCF8574 0x20 (laut Konfiguration). Vier Tasten und vier LEDs hängen **virtuell** am PCF8574, nicht direkt an separaten Nano-Pins.

**Achtung:** Die Quellwerte `PD3`, `PB1` etc. sind AVR-Portbit-Bezeichnungen (numerisch nicht identisch mit D3/D9). Bei ESP32 dürfen sie **nicht als GPIO-Nummern kopiert** werden. Das tatsächliche Anschlussbild HW2 ist vor dem Verdrahten zu verifizieren.
