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

// A4988: bestehende STEP/DIR/ENABLE-Belegung bleibt erhalten.
// Firmware-Grundzustand: STEP LOW, ENABLE HIGH; SLEEP/RESET LOW.
// Auf HW3 wurden STEP und ENABLE gegenueber HW2 vertauscht: G27/G25.
constexpr uint8_t MOTOR_PWM_PIN    = 27; // G27 -> STEP / PULS (Ausgang), weisse LED
constexpr uint8_t MOTOR_DIR_PIN    = 26; // G26 -> DIR (Ausgang)
constexpr uint8_t MOTOR_ENABLE_PIN = 25; // G25 -> !ENABLE (LOW = Freigabe), blaue LED

// Mikroschritte: M0/M1/M2 entsprechen beim A4988 MS1/MS2/MS3.
// HIGH/HIGH/HIGH = 1/16; Bench-Aufloesung siehe Motor-Modul.
constexpr uint8_t MOTOR_MS0_PIN = 13; // G13 -> MS1 (Ausgang)
constexpr uint8_t MOTOR_MS1_PIN = 14; // G14 -> MS2 (Ausgang)
constexpr uint8_t MOTOR_MS2_PIN =  4; // G4  -> MS3 (Ausgang)

// HW3-SLEEP: G32 startet LOW, wird erst bei freigegebenem GO HIGH.
// Nach Aufwecken mindestens 1 ms vor erstem STEP warten.
// HW3: !RESET und !SLEEP verbunden an G32, gemeinsamer 10-kOhm-Pulldown.
// HW3: G25 (!ENABLE), G26 (DIR), G27 (STEP) ohne Zusatz-Pulls.
// HW3-LEDs haben gemeinsames PLUS, sind bei GPIO LOW AN:
// rot !SLEEP/!RESET G32, blau !ENABLE G25, gelb DIR G26, weiss STEP G27.
// ACHTUNG: Rote LED von +3,3 V nach G32 kann den 10-kOhm-Pulldown
// bei hochohmigem GPIO uebersteuern. LOW-Pegel vor VMOT pruefen (#153).
// Pulls ersetzen keinen geprueften Boot-/Flash-/OTA-Interlock.
constexpr uint8_t MOTOR_SLEEP_PIN = 32; // G32 -> !SLEEP (HIGH = wach)

// HW3-Sensorpins: ersetzt die bisherige Zuordnung G32/G33/G16/G17.
// G34/G35/G36/G39 sind reine Eingaenge OHNE interne Pull-Widerstaende.
// Nur 0..3,3-V-Signale; je Sensor 10-kOhm-Pullup nach 3,3 V (HW3-Soll).
// Das ausfuehrbare Switches-Modul und der Endlagenschutz sind noch offen.
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
// HUGINN - Kern 0: Bedienung, Anzeige und Kommunikation
// ============================================================================

// I2C: PCF8574 mit vier LED-Tastern und OLED; Polling, kein INT-GPIO.
// SDA/SCL je 4,7-kOhm-Pullup nach 3,3 V als Bus-Gesamtzielwert.
// Vorhandene Modul-Pulls anrechnen; Keyboard/OLED-Modul noch nicht implementiert.
constexpr uint8_t I2C_SDA_PIN = 21; // G21 -> SDA
constexpr uint8_t I2C_SCL_PIN = 22; // G22 -> SCL

// SPI-Gebaeudeerweiterung #165: reserviert, noch NICHT implementiert.
// Gemeinsamer Bus, je ein CS fuer Display und MCP23S17.
// Beide CS hardwareseitig HIGH halten und vor Businitialisierung deaktivieren.
constexpr uint8_t SPI_SCLK_PIN       = 18; // G18 -> SCLK (Ausgang)
constexpr uint8_t SPI_MISO_PIN       = 19; // G19 <- MISO (Eingang)
constexpr uint8_t SPI_MOSI_PIN       = 23; // G23 -> MOSI (Ausgang)
constexpr uint8_t SPI_CS_DISPLAY_PIN = 16; // G16 -> !CS Display
constexpr uint8_t SPI_CS_EXPANDER_PIN = 17; // G17 -> !CS MCP23S17
constexpr uint8_t SPI_DISPLAY_DC_PIN = 33; // G33 -> Display D/C
// Display-RESET/Backlight und MCP23S17-RESET lokal nach Moduldatenblatt.
// Kein eigener Reset-GPIO, keine ungepruefte Bruecke zu ESP32-EN.
// DE-9: GND, 3V3, SCLK, MOSI, MISO, CS_DISPLAY, CS_EXPANDER, DC, Reserve.
// Kontakt 9 bleibt unbeschaltet. 12-V-Lichtversorgung separat fuehren.

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
// Kein drittes CS zugeteilt. G16/G17 gelten hier fuer WROOM ohne PSRAM.
