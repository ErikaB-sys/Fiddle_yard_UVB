# FY ESP32-WROOM – Dual-Core-Bring-up

Isolierter Testaufbau im Branch `feature/FY_ESP32`, **kein Ersatz** für die produktive Nano-Firmware.

## Plattform
- Klassischer Dual-Core ESP32-WROOM (kein S3); PlatformIO `esp32dev` als vorläufiges generisches Board-Profil.
- Arduino-Framework/C++ mit ESP-IDF FreeRTOS.
- Bei abweichendem konkretem WROOM-Entwicklungsboard Board-Konfiguration und USB-UART-Port prüfen.

## Verbindliche Aufgabentrennung

| Core | Aufgabe | Besitzer |
|---|---|---|
| 1 (APP_CPU) | Motor/Bewegungsablauf, Referenzierung, Sensoren, Motor-GPIO, harte Abschaltbedingungen | `FY_Motor` |
| 0 (PRO_CPU) | UART-Protokoll, Antworten/NACKs, spätere externe Kommunikation und Diagnoseschnittstellen | `FY_Comm` |

**Wichtig:** Core 0 ist bei aktivem Wi-Fi/Bluetooth zusätzlich mit ESP32-Systemaufgaben belegt. Die Zuordnung reserviert nicht physikalisch einen ganzen Core exklusiv für unsere Anwendung. Motorpulse daher **hardwaregestützt** (Timer-/Peripherie) erzeugen, nicht durch Task-Delay oder Busy-Wait.

## Kern-zu-Kern-Schnittstelle
- Zwei typisierte FreeRTOS-Queues: `Command` (Kommunikation → Motor) und `Response` (Motor → Kommunikation).
- Ein *einziger Besitzer* für Bewegungszustand, Motor-GPIO und Sensorzugriff: Motor-Task.
- Kommunikations-Task wartet nicht blockierend auf Motor-Hardware; kein direkter Zugriff auf interne Bewegungsvariablen.
- Später definierte Befehle: Referenzieren, Gleis anfahren, Position setzen, Stopp, Status. Das **bestehende binäre FY-UART-Protokoll** einschließlich NACK bleibt an der Außengrenze erhalten.
- Priorisierung von Not-Stopp, Überlaufverhalten, asynchronen Ereignissen und Sequenz-/Korrelations-IDs vor echter Motoransteuerung spezifizieren.
- Externe STOP-Befehle müssen auch bei voller Normal-Queue sicher funktionieren; physische Endschalter dürfen nicht allein von Kommunikations-Queues abhängen.
- Keine dynamischen Allokationen im regulären Bewegungsablauf.

## Erster isolierter Test
1. Ordner `FY_ESP32_Bringup` in PlatformIO öffnen und Build starten.
2. Geeignetes ESP32-WROOM-Board anschließen, hochladen.
3. Seriellen Monitor (115200 Baud) öffnen.
4. Sekündliche Meldungen `comm core=0, motor core=1` bestätigen den Nachrichtenaustausch über Queues.

**Noch keine Motor-, Sensor-, UART-Protokoll- oder Timingtests.** Das Programm schaltet **keine** STEP/DIR/ENABLE-Ausgänge.

## Nächste Schritte
1. Reale ESP32-WROOM-Pinbelegung und 3,3-V-Pegel zu TB6600, I²C und Sensoren prüfen.
2. Hardware-Abstraktion für Motorpulse, GPIO und Zeitbasis definieren.
3. Reales FY-Binärprotokoll nur in Core 0 integrieren; Core 1 empfängt typisierte Kommandos.
4. Motorpulse per ESP32-Peripherie und Messung unter Kommunikationslast validieren.
5. Schutz-/Not-Stopp- und Timeout-Fälle testen. Main/Nano bleiben davon unberührt.
