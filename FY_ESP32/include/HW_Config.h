#pragma once
// HW3: ESP32-WROOM (38 Pin), GPIO-Belegung vom 22.10.2026.
// Gxx bezeichnet die Board-Aufschrift, nicht die Steckerposition.
// Zentraler Verdrahtungsplan: docs/pinouts/01_PINOUT_ESP32.md
// Die Kernzuordnung beschreibt den Software-Owner, keine elektrische Bindung.
// Core-IDs und Task-Pinning: FY_Bringup.h, lib/Motor/ und lib/UART/.
// Pin-Konstanten allein initialisieren keine Hardware.

#include <Arduino.h>

// ============================================================================
// MUNINN - Kern 1: Motor, Sensoren und unmittelbarer Hardwareschutz
// ============================================================================

// A4988: HW3-Belegung gemaess physischer Verdrahtung G27/G26/G25.
// Firmware-Startzustand: STEP LOW, ENABLE HIGH, SLEEP/RESET LOW.
// Nach initialisiertem Motor/Timer wird SLEEP einmal HIGH und bleibt es.
// Auf HW3 wurden STEP und ENABLE gegenueber HW2 vertauscht: G27/G25.
constexpr uint8_t MOTOR_PWM_PIN    = 27; // G27 -> STEP / PULS (Ausgang), weisse LED
constexpr uint8_t MOTOR_DIR_PIN    = 26; // G26 -> DIR (Ausgang)
constexpr uint8_t MOTOR_ENABLE_PIN = 25; // G25 -> !ENABLE (LOW = Freigabe), blaue LED

// Mikroschritte: M0/M1/M2 entsprechen beim A4988 MS1/MS2/MS3.
// HIGH/HIGH/HIGH = 1/16; Bench-Aufloesung siehe Motor-Modul.
constexpr uint8_t MOTOR_MS0_PIN = 13; // G13 -> MS1 (Ausgang)
constexpr uint8_t MOTOR_MS1_PIN = 14; // G14 -> MS2 (Ausgang)
constexpr uint8_t MOTOR_MS2_PIN =  4; // G4  -> MS3 (Ausgang)

// HW3-SLEEP: G32 LOW waehrend Boot/Setup, nach Motorinitialisierung
// EINMAL HIGH mit 1,5 ms tWAKE-Wartezeit vor UART-/Muninn-Taskstart.
// GO/STOP schaltet nur ENABLE und STEP, nicht SLEEP.
// HW3: !RESET und !SLEEP verbunden an G32, gemeinsamer 10-kOhm-Pulldown.
// HW3: G25 (!ENABLE), G26 (DIR), G27 (STEP) ohne Zusatz-Pulls.
// ROTE LED ist aktiv HIGH: Anode am G32 (!SLEEP/!RESET),
// Kathode ueber Vorwiderstand an GND. LOW = AUS/Schlaf,
// HIGH = AN/wach. Eigener 10-kOhm-Pulldown nach GND.
// BLAU/GELB/WEISS: gemeinsames Plus +3,3 V, aktiv LOW:
// blau !ENABLE G25, gelb DIR G26, weiss STEP G27.
// HW3-Bootpegel bei GPIO32 im Reset mit 3,3 V nachmessen (#153).
// Pulls ersetzen keinen geprueften Boot-/Flash-/OTA-Interlock.
constexpr uint8_t MOTOR_SLEEP_PIN = 32; // G32 -> !SLEEP (HIGH = wach)

// HW3-Sensorpins: ersetzt die bisherige Zuordnung G32/G33/G16/G17.
// G34/G35/G36/G39 sind reine Eingaenge OHNE interne Pull-Widerstaende.
// Nur 0..3,3-V-Signale; je Sensor 10-kOhm-Pullup nach 3,3 V (HW3-Soll).
// Sensoren werden im Muninn-Motorpfad gelesen; separate Switches-Klasse nicht erforderlich.
constexpr uint8_t LSREF_PIN = 34; // G34 -> Referenz-Lichtschranke
constexpr uint8_t LS4_PIN   = 35; // G35 -> zweite Lichtschranke / Riemen
constexpr uint8_t LSL_PIN   = 36; // G36 / VP -> Endschalter links
constexpr uint8_t LSR_PIN   = 39; // G39 / VN -> Endschalter rechts
// G36/G39: Pegelabfrage im direkten Motor-Schutzpfad, keine GPIO-Interrupts
// (ESP32-Einschraenkungen bei ADC bzw. Funk mit Schlafmodi beachten).

// Diagnose G2 gehoert aktuell ebenfalls Muninn: setLed() in Motor.cpp.
// Tatsächliche Auswahl ueber FY_STATUS_LED_PIN (platformio.ini; Default 2).
// Hier keine zweite unabhaengige LED-Pinkonstante anlegen.

// ============================================================================
// HUGINN - Kommunikations-/Bedienpfad: I2C und UART
// ============================================================================

// I2C: PCF8574 mit vier LED-Tastern und OLED; Polling, kein INT-GPIO.
// SDA/SCL je 4,7-kOhm-Pullup nach 3,3 V als Bus-Gesamtzielwert.
// Vorhandene Modul-Pulls anrechnen; Keyboard/OLED-Modul auf HW3 funktionsgeprueft.
constexpr uint8_t I2C_SDA_PIN = 21; // G21 -> SDA
constexpr uint8_t I2C_SCL_PIN = 22; // G22 -> SCL

// GPIO16/17/18/19/23/33 sind derzeit NC. Kein SPI-Bus und
// keine Display- oder Portexpander-SPI-Anschluesse fuer HW3.
// Eine moegliche spaetere TWAI/CAN-Nutzung von GPIO16/17 ist nur
// Konzept, nicht als aktive Belegung implementiert.

// UART0: vorhandene USB-UART-Verbindung, nicht fuer andere Geraete vergeben.
// Serial.begin() nutzt derzeit die Board-Standardpins; keine neue UART.
constexpr uint8_t UART0_TX_PIN = 1; // G1 / TX0 -> USB-UART
constexpr uint8_t UART0_RX_PIN = 3; // G3 / RX0 <- USB-UART
// WLAN/Web/JMRI benoetigen keine zusaetzlichen Header-GPIOs.

// ============================================================================
// BOARD / VERSORGUNG - keine frei vergebenen Task-GPIOs
// ============================================================================
// G12: ausdruecklich frei lassen.
// G0: Boot/Download; G5/G15: unbeschaltet, Reserve nur nach Strapping-Pruefung.
// G6..G11: Flash, nicht benutzen. G37/G38: am WROOM-Header nicht verfuegbar.
// EN: ESP32-Chipfreigabe/Reset, kein Benutzer-GPIO.
// 3V3/5V/GND: Versorgung; USB/externe Einspeisung gemaess HW3_STROMVERSORGUNG.md.
// Unbenutzte Pins bleiben NC; keine SPI-CS-Leitungen.
