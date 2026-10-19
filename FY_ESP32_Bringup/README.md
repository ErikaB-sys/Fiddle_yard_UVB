# FY ESP32 – vorbereitender Bring-up

Dieser Ordner ist ein **isolierter Testaufbau** und ersetzt keine produktive FY-Firmware.

## Ausgangspunkt
- Branch `feature/FY_ESP32` von `main`.
- PlatformIO + Espressif32 + Arduino (C++).
- `esp32dev` dient nur als generisches Build-Ziel; vor Hardwaretests den tatsächlichen ESP32-Boardtyp und dessen Pinbelegung prüfen.

## Erster Test
1. Ordner `FY_ESP32_Bringup` separat mit PlatformIO öffnen.
2. Build durchführen; bei passender Hardware anschließend hochladen.
3. Seriellen Monitor mit 115200 Baud öffnen und die Startmeldung prüfen.

Der Test erzeugt **keine STEP-, DIR- oder EN-Signale**. Motor und Sensoren werden nicht angesprochen.

## Portierungsprinzip
- Nano-Firmware und Main bleiben unverändert.
- FY-UART-Protokoll einschließlich Fehler-/NACK-Codierung bleibt kompatibel.
- Bewegungsalgorithmus von Timer-/GPIO-Zugriffen trennen.
- AVR Timer1 ISR nicht direkt portieren: ESP32-spezifische Timer-Lösung mit reproduzierbarem Pulsverhalten entwickeln.
- I2C (Display/Portexpander), Referenzsensoren und Endschalter schrittweise testen.
- Pinmapping und elektrische Pegel (ESP32: 3,3 V GPIO) vor Anschluss an bestehende Hardware prüfen.
- Keine zusätzliche Weboberfläche/WLAN-Abhängigkeit im ersten Bring-up.

## Offen
- Exakter ESP32-Boardtyp und verfügbare GPIOs.
- Geeignete STEP-Impulserzeugung und Timingmessung.
- Mapping der bestehenden Module auf HAL/Adapter.
- Hardware-in-the-loop-Testplan für Referenzierung und Fahrwege.
