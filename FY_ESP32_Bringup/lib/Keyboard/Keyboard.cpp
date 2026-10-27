#include "Keyboard.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HW_Config.h"

namespace fy {
namespace {
constexpr uint8_t KB_PCF_ADDRESS = 0x27U;
constexpr uint8_t KB_OLED_ADDRESS = 0x3CU;
constexpr uint8_t KB_BUTTON_MASK = 0xF0U;
constexpr uint8_t KB_LED_MASK = 0x0FU;
constexpr uint8_t KB_BUTTON_BITS[4] = {7U, 6U, 5U, 4U};
constexpr uint8_t KB_LED_BITS[4] = {3U, 2U, 1U, 0U};
const char* const KB_NAMES[4] = {"STOP", "LINKS", "GO", "RECHTS"};
Adafruit_SSD1306 kbDisplay(128, 32, &Wire, -1);
bool kbOledReady = false;
bool kbPcfReady = false;
uint8_t kbLastRaw = 0U;
uint8_t kbStable = 0U;
uint8_t kbSeen = 0U;
uint8_t kbLedByte = 0xFFU;
uint32_t kbLastChangeMs = 0U;
uint32_t kbLastPollMs = 0U;
uint32_t kbSplashAt = 0U;
bool kbSplashDone = false;

// @brief Schreibt LED-Zustände; alle vier Tasterleitungen bleiben HIGH.
void keyboardWriteLeds(uint8_t activeButtons) {
    uint8_t desired = 0xFFU;
    for (uint8_t i = 0U; i < 4U; ++i) {
        if ((activeButtons & (1U << i)) != 0U) {
            desired &= static_cast<uint8_t>(~(1U << KB_LED_BITS[i]));
        }
    }
    if (desired == kbLedByte) return;
    Wire.beginTransmission(KB_PCF_ADDRESS);
    Wire.write(desired);
    if (Wire.endTransmission() == 0U) kbLedByte = desired;
}

// @brief Zeigt erkannten Tastendruck und Fortschritt des HW-Tests an.
void keyboardShow(uint8_t activeButtons) {
    if (!kbOledReady) return;
    kbDisplay.clearDisplay();
    kbDisplay.setCursor(0, 0);
    kbDisplay.println(F("FY HW3 / KEYBOARD"));
    kbDisplay.print(F("Taste: "));
    if (activeButtons == 0U) {
        kbDisplay.println(F("-"));
    } else {
        bool first = true;
        for (uint8_t i = 0U; i < 4U; ++i) {
            if ((activeButtons & (1U << i)) != 0U) {
                if (!first) kbDisplay.print(F("+"));
                kbDisplay.print(KB_NAMES[i]);
                first = false;
            }
        }
        kbDisplay.println();
    }
    uint8_t count = 0U;
    for (uint8_t i = 0U; i < 4U; ++i) if ((kbSeen & (1U << i)) != 0U) ++count;
    kbDisplay.print(F("Test: "));
    kbDisplay.print(count);
    kbDisplay.println(F("/4"));
    if (count == 4U) kbDisplay.println(F("ALLE TASTEN OK"));
    else kbDisplay.println(F("Taste druecken ..."));
    kbDisplay.display();
}

// @brief Initialisiert nur die Diagnose-Hardware, ohne Motorbefehle.
void keyboardTestBegin() {
    Wire.beginTransmission(KB_PCF_ADDRESS);
    kbPcfReady = (Wire.endTransmission() == 0U);
    if (kbPcfReady) {
        Wire.beginTransmission(KB_PCF_ADDRESS);
        Wire.write(0xFFU);  // LEDs aus; P4..P7 als Eingänge frei
        kbPcfReady = (Wire.endTransmission() == 0U);
    }
    kbOledReady = kbDisplay.begin(SSD1306_SWITCHCAPVCC, KB_OLED_ADDRESS);
    if (kbOledReady) {
        kbDisplay.setRotation(2);  // HW_TEST-Orientierung
        kbDisplay.setTextSize(1);
        kbDisplay.setTextColor(SSD1306_WHITE);
        kbDisplay.clearDisplay();
        kbDisplay.setCursor(0, 0);
        kbDisplay.println(F("Hello my friend"));
        kbDisplay.println(F("FY ESP32 / HW3"));
        kbDisplay.println(F("I2C OK"));
        kbDisplay.println(F("Keyboard Test"));
        kbDisplay.display();
    }
    Serial.print(F("KEYBOARD: PCF8574="));
    Serial.print(kbPcfReady ? F("OK") : F("ERROR"));
    Serial.print(F(" OLED="));
    Serial.println(kbOledReady ? F("OK") : F("ERROR"));
    kbLastChangeMs = millis();
    kbSplashAt = millis();
}

// @brief Pollt Tasten entprellt alle 25 ms. Keine Fahrbefehle!
void keyboardTestPoll() {
    const uint32_t now = millis();
    if (!kbSplashDone && now - kbSplashAt >= 5000U) {
        kbSplashDone = true;
        if (kbOledReady) {
            kbDisplay.clearDisplay();
            kbDisplay.setCursor(0, 0);
            kbDisplay.println(F("FY UVB - LOCAL"));
            kbDisplay.println(F("STOP verriegelt"));
            kbDisplay.println(F("GO + LINKS"));
            kbDisplay.println(F("Freigabe: folgt"));
            kbDisplay.display();
        }
    }
    if (!kbPcfReady || !kbSplashDone) return;
    if ((now - kbLastPollMs) < 25U) return;
    kbLastPollMs = now;
    if (Wire.requestFrom(KB_PCF_ADDRESS, static_cast<uint8_t>(1U)) != 1U) return;
    const uint8_t raw = static_cast<uint8_t>(Wire.read());
    uint8_t active = 0U;
    for (uint8_t i = 0U; i < 4U; ++i) {
        if ((raw & (1U << KB_BUTTON_BITS[i])) == 0U) active |= static_cast<uint8_t>(1U << i);
    }
    if (active != kbLastRaw) {
        kbLastRaw = active;
        kbLastChangeMs = now;
        return;
    }
    if ((now - kbLastChangeMs) < 25U || active == kbStable) return;
    kbStable = active;
    kbSeen |= active;
    keyboardWriteLeds(active);
    // Bis zur Command-Integration bleibt die Bedienung reine Diagnose.
    keyboardShow(active);
    Serial.print(F("KEYBOARD: "));
    if (active == 0U) Serial.println(F("keine Taste"));
    else {
        for (uint8_t i = 0U; i < 4U; ++i) if ((active & (1U << i)) != 0U) {
            Serial.print(KB_NAMES[i]);
            Serial.print(' ');
        }
        Serial.println();
    }
}

} // namespace

/** @brief Initialisiert OLED und PCF8574 für den lokalen Hardwaretest. */
void Keyboard_begin() { keyboardTestBegin(); }

/** @brief Pollt entprellte Tasten ohne Motorwirkung. */
void Keyboard_poll() { keyboardTestPoll(); }
} // namespace fy
