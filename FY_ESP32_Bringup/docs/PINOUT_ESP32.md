# ESP32-WROOM 38-Pin – Vorschlag für HW2-Adapter

**Status: PLAN, NICHT VERDRAHTET ODER ELEKTRISCH GEPRÜFT.** GPIOs sind eine erste konfliktarme Belegung des klassischen ESP32-WROOM (nicht S3). Durchgang/Pinbeschriftung des AZ-Delivery-Boards prüfen.

| Funktion | ESP32 GPIO (Vorschlag) | Hinweise |
|---|---:|---|
| Motor STEP | 25 | Test-LED mit eigenem Widerstand, später STEP |
| Motor DIR | 26 | Test-LED mit Widerstand |
| Motor ENABLE | 27 | Beim Boot sicher deaktiviert halten (aktive Polarität prüfen) |
| Referenzsensor | 16 | 3,3-V-Signal |
| Trim links | 17 | 3,3-V-Signal |
| Trim rechts | 18 | 3,3-V-Signal |
| Timing-Belt-Sensor | 19 | 3,3-V-Signal |
| I²C SDA | 21 | Pull-ups **nur nach 3,3 V** |
| I²C SCL | 22 | Pull-ups **nur nach 3,3 V** |
| Eingebaute Test-LED | 2 | Bereits getestet; Boot-Strapping-Pin, nicht extern belasten |

Stromversorgung: **geregelte 3,3 V an 3V3** nach Messung und Prüfung der Board-Versorgung, GND gemeinsam. 12 V **nur an VMOT** des A4988/Shield (nicht an ESP32). A4988-VDD muss zur 3,3-V-Logik passen; vorhandenes Shield auf 5-V-Verbindungen prüfen.

Die GPIO-Festlegung wird erst nach Sichtprüfung/Schaltbildprüfung und Spannungsmessung in die künftige ESP32-`HW_Config` übernommen. Sicheres ENABLE-Default-Hardwarekonzept zuerst bestimmen. Keine Motorlast beim ersten GPIO-Test.
