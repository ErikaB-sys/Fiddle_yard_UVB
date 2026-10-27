# HW2-Adapter: Nano-Anschluss → ESP32-WROOM

**Planungsstand 19.10.2026; keine Freigabe zum ungeprüften Anschließen.** Referenz: [Nano-Pinout](PINOUT_NANO.md), [ESP32-Pinout](PINOUT_ESP32.md).

| Bestehende HW2-Leitung | Nano-Anschluss | ESP32-GPIO (Vorschlag) | Erster Test |
|---|---|---|---|
| STEP / weiß | D3 | 25 | LED + Vorwiderstand |
| DIR / gelb | D4 | 26 | LED + Vorwiderstand |
| ENABLE / blau | D2 | 27 | LED + Vorwiderstand, Polarität prüfen |
| Referenz | D9 | 16 | Eingangspegel messen |
| Trim links | D10 | 17 | Eingangspegel messen |
| Trim rechts | D11 | 18 | Eingangspegel messen |
| Timing Belt | D12 | 19 | Eingangspegel messen |
| I²C SDA | A4 | 21 | Pull-up / Versorgung prüfen |
| I²C SCL | A5 | 22 | Pull-up / Versorgung prüfen |
| GND | GND | GND | Gemeinsame Masse |
| Logic VCC | 5 V (Nano/HW2 prüfen) | 3V3 **nur geeignete Verbraucher** | **Nicht blind verbinden!** |

## Vorgehen
1. HW2-Shield und vorhandene Nano-Verdrahtung fotografisch/mit Durchgangsprüfer dem Pinout zuordnen; insbesondere 5-V-Schienen und interne Pull-ups.
2. ESP32-Stromversorgung aus DC/DC-Wandler messen (3,3 V); 12-V-VMOT streng getrennt führen. Motor bei ausgeschalteter Versorgung stecken/ziehen.
3. Sicheren Ruhezustand von ENABLE durch geeignete externe Beschaltung sicherstellen (abhängig von A4988-Shield-Polarität).
4. STEP/DIR/ENABLE zuerst **nur LEDs mit Vorwiderstand**, keine Verbindung zum aktiven Motor-Treiber-Eingang; GPIO-Test.
5. I²C- und Sensoreingänge mit 3,3-V-Pegeln überprüfen, erst danach anschließen.
6. A4988-VREF/Stromgrenze anhand Sense-Widerstand und NEMA11-Daten prüfen; Pufferung an VMOT kontrollieren. Viertelschritt: MS1=LOW, MS2=HIGH, MS3=LOW.
7. Erst dann Motor mit moderater Frequenz und belastbarer Messung der STEP-Pulsbreite testen.

**Dokumentationsprinzip:** `HW_Config.h` enthält später ausschließlich die **wirklich bestätigten** ESP32-Pinwerte. Diese Markdown-Dateien dokumentieren die physische Verdrahtung und die Zuordnung unabhängig davon.
