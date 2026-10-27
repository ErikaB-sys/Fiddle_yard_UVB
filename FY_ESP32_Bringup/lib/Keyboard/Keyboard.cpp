#include "Keyboard.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HW_Config.h"
#include "Motor.h"

namespace fy {
namespace {
constexpr uint8_t KB_PCF_ADDRESS = 0x27U;
constexpr uint8_t KB_OLED_ADDRESS = 0x3CU;
constexpr uint8_t KB_BUTTON_BITS[4] = {7U, 6U, 5U, 4U};
constexpr uint8_t KB_LED_BITS[4] = {3U, 2U, 1U, 0U};
Adafruit_SSD1306 kbDisplay(128, 32, &Wire, -1);
bool kbOledReady = false;
bool kbPcfReady = false;
uint8_t kbLastRaw = 0U;
uint8_t kbStable = 0U;
uint8_t kbLedByte = 0xFFU;
uint32_t kbLastChangeMs = 0U;
uint32_t kbLastPollMs = 0U;
uint32_t kbSplashAt = 0U;
bool kbSplashDone = false;
BringupContext* kbContext = nullptr;
enum class Menu { LOCKED, UNLOCK, ENABLE, REF_MENU, REF_RUN, TRACK_MENU, TRACK_RUN, ERROR };
Menu kbMenu = Menu::LOCKED;
bool kbLongRef = false;
uint8_t kbTrack = 3U;
uint32_t kbSeq = 0U;
uint32_t kbAwaitSeq = 0U;
bool kbWaiting = false;
uint32_t kbRunAt = 0U;

void keyboardShow(uint8_t activeButtons);

// Lokale Telegramme erhalten das High-Bit als Quellenkennung.
bool sendLocal(uint8_t id, uint8_t a, uint8_t b, bool priority) {
    if (kbWaiting || kbContext == nullptr) return false;
    CommandMessage msg{};
    msg.id = id;
    msg.data[0] = a;
    msg.data[1] = b;
    msg.length = (id == CMD_REFERENCE || id == CMD_SET_TRACK) ? 2U : 3U;
    msg.sequence = 0x80000000UL | (++kbSeq & 0x7FFFFFFFUL);
    QueueHandle_t q = priority ? kbContext->priorityCommands : kbContext->commands;
    if (q == nullptr || xQueueSend(q, &msg, 0) != pdTRUE) return false;
    kbAwaitSeq = msg.sequence;
    kbWaiting = true;
    kbRunAt = millis();
    return true;
}

// Antwort nur aus der dem Keyboard zugeteilten Queue lesen.
void pollLocalReply() {
    if (kbContext == nullptr || kbContext->keyboardEvents == nullptr) return;
    MotorEvent reply{};
    while (xQueueReceive(kbContext->keyboardEvents, &reply, 0) == pdTRUE) {
        if (!kbWaiting || reply.sequence != kbAwaitSeq) continue;
        kbWaiting = false;
        if (reply.responseId != STATUS_ACK) {
            kbMenu = Menu::ERROR;
        } else if (kbMenu == Menu::UNLOCK) {
            kbMenu = Menu::ENABLE;
            if (!sendLocal(CMD_GO, 0xAAU, 0x55U, false)) kbMenu = Menu::ERROR;
        } else if (kbMenu == Menu::ENABLE) {
            kbMenu = Menu::REF_MENU;
        }
        keyboardShow(0U);
    }
}

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

// @brief Zeigt die lokale Menüführung auf dem SSD1306 an.
void keyboardShow(uint8_t activeButtons) {
    if (!kbOledReady) return;
    kbDisplay.clearDisplay();
    kbDisplay.setCursor(0, 0);
    kbDisplay.println(F("FY UVB / LOCAL"));
    if (kbMenu == Menu::LOCKED) {
        kbDisplay.println(F("STOP verriegelt"));
        kbDisplay.println(F("LINKS halten"));
        kbDisplay.println(F("dann GO"));
    } else if (kbMenu == Menu::REF_MENU) {
        kbDisplay.print(F("REF: "));
        kbDisplay.println(kbLongRef ? F("LONG") : F("SHORT"));
        kbDisplay.println(F("LINKS/RECHTS Wahl"));
        kbDisplay.println(F("GO: Start"));
    } else if (kbMenu == Menu::TRACK_MENU) {
        kbDisplay.print(F("Ziel BG"));
        kbDisplay.println(kbTrack);
        kbDisplay.println(F("LINKS/RECHTS Wahl"));
        kbDisplay.println(F("GO: Fahren"));
    } else if (kbMenu == Menu::REF_RUN) {
        kbDisplay.println(F("Referenzfahrt"));
        kbDisplay.println(F("danach BG3"));
    } else if (kbMenu == Menu::TRACK_RUN) {
        kbDisplay.println(F("Gleisfahrt aktiv"));
    } else if (kbMenu == Menu::ERROR) {
        kbDisplay.println(F("FEHLER / STOP"));
    } else {
        kbDisplay.println(F("Motorfreigabe"));
    }
    kbDisplay.display();

}

// @brief Initialisiert I2C-Geräte und das fünfsekündige Startbild.
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

// @brief Entprellt Tasten alle 25 ms und verwirft ungültige Bedienfolgen.
void keyboardTestPoll() {
    const uint32_t now = millis();
    if (!kbSplashDone && now - kbSplashAt >= 5000U) {
        kbSplashDone = true;
        keyboardShow(0U);
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
    const uint8_t previous = kbStable;
    const uint8_t pressed = static_cast<uint8_t>(active & ~previous);
    kbStable = active;
    keyboardWriteLeds(active);
    if ((pressed & 0x01U) != 0U) {
        kbWaiting = false;
        kbMenu = Menu::LOCKED;
        if (!sendLocal(CMD_STOPP, 0x55U, 0xAAU, true))
            kbMenu = Menu::ERROR;
    } else if (!kbWaiting && kbMenu == Menu::LOCKED) {
        // LINKS halten und GO neu druecken; Einzel-GO bleibt wirkungslos.
        if ((pressed & 0x04U) != 0U && (previous & 0x02U) != 0U &&
            (active & 0x08U) == 0U) {
            kbMenu = Menu::UNLOCK;
            if (!sendLocal(CMD_STOPP, 0xAAU, 0x55U, true))
                kbMenu = Menu::ERROR;
        }
    } else if (!kbWaiting && !Muninn_isMoving() &&
               (active & static_cast<uint8_t>(active - 1U)) == 0U) {
        if (kbMenu == Menu::REF_MENU) {
            if ((pressed & 0x0AU) != 0U) kbLongRef = !kbLongRef;
            else if ((pressed & 0x04U) != 0U) {
                kbMenu = Menu::REF_RUN;
                if (!sendLocal(CMD_REFERENCE, kbLongRef ? 1U : 0U, 0U, false))
                    kbMenu = Menu::ERROR;
            }
        } else if (kbMenu == Menu::TRACK_MENU) {
            if ((pressed & 0x02U) != 0U)
                kbTrack = kbTrack == 1U ? 5U : static_cast<uint8_t>(kbTrack - 1U);
            else if ((pressed & 0x08U) != 0U)
                kbTrack = kbTrack == 5U ? 1U : static_cast<uint8_t>(kbTrack + 1U);
            else if ((pressed & 0x04U) != 0U) {
                kbMenu = Menu::TRACK_RUN;
                if (!sendLocal(CMD_SET_TRACK, kbTrack, 0U, false))
                    kbMenu = Menu::ERROR;
            }
        }
    }
    keyboardShow(active);
}

void progress() {
    if (kbWaiting) return;
    const auto state = Muninn_motorState();
    const uint32_t track = Muninn_trackSnapshot();
    if (kbMenu == Menu::REF_RUN) {
        if (Muninn_referenceStatusSnapshot() == 1U &&
            ((track >> 8U) & 0xFFU) == 3U &&
            ((track >> 16U) & 0xFFU) == TRACK_STATUS_REACHED &&
            !Muninn_isMoving() && state == MotorState_t::IDLE) {
            kbTrack = 3U; kbMenu = Menu::TRACK_MENU; keyboardShow(0U);
        } else if (state == MotorState_t::ERROR) {
            kbMenu = Menu::ERROR; keyboardShow(0U);
        }
    } else if (kbMenu == Menu::TRACK_RUN) {
        if (state == MotorState_t::ERROR) {
            kbMenu = Menu::ERROR; keyboardShow(0U);
        } else if (!Muninn_isMoving() && state == MotorState_t::IDLE &&
                   ((track >> 8U) & 0xFFU) == kbTrack &&
                   ((track >> 16U) & 0xFFU) == TRACK_STATUS_REACHED) {
            kbMenu = Menu::TRACK_MENU; keyboardShow(0U);
        }
    }
}
} // namespace

/** @brief Initialisiert das I2C-Keyboard mit eigenem Kommando-Rückkanal. */
void Keyboard_begin(BringupContext* context) { kbContext = context; keyboardTestBegin(); }

/** @brief Verarbeitet lokale Bedienung und Muninn-Status. */
void Keyboard_poll() { pollLocalReply(); keyboardTestPoll(); progress(); }
} // namespace fy
