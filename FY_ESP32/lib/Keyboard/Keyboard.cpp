#include "Keyboard.h"
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "HW_Config.h"
#include "Motor.h"

namespace fy {
namespace {
constexpr uint8_t KB_PCF_ADDRESS = 0x27U;
constexpr uint8_t KB_OLED_ADDRESS = 0x3CU;
constexpr uint8_t KB_BUTTON_BITS[4] = {7U, 6U, 5U, 4U};
constexpr uint8_t KB_LED_BITS[4] = {3U, 2U, 1U, 0U};
// 128x32, I2C, um 180 Grad gedreht wie in der HW3-Installation.
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C kbDisplay(U8G2_R2, U8X8_PIN_NONE);
bool kbOledReady = false;
bool kbPcfReady = false;
uint8_t kbLastRaw = 0U;
uint8_t kbStable = 0U;
uint8_t kbLedByte = 0xFFU;
uint32_t kbLastChangeMs = 0U;
uint32_t kbLastPollMs = 0U;
uint32_t kbSplashAt = 0U;
bool kbSplashDone = false;
bool kbStartReleased = false;
bool kbStartWarningShown = false;
BringupContext* kbContext = nullptr;
enum class Menu { LOCKED, UNLOCK, ENABLE, REF_MENU, REF_RUN, TRACK_MENU, TRACK_RUN, ERROR };
Menu kbMenu = Menu::LOCKED;
bool kbLongRef = false;
uint8_t kbTrack = 3U;
uint32_t kbSeq = 0U;
uint32_t kbAwaitSeq = 0U;
bool kbWaiting = false;

void keyboardShow();

// Lokale Telegramme erhalten das High-Bit als Quellenkennung.
/** @brief Sendet einen lokalen Auftrag über die passende Muninn-Queue.
 *  @param id Protokoll-ID des Auftrags.
 *  @param a Erstes Datenbyte.
 *  @param b Zweites Datenbyte.
 *  @param priority Verwendet bei true die Prioritätsqueue.
 *  @return true, wenn die Nachricht eingereiht wurde.
 */
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
    return true;
}

// Antwort nur aus der dem Keyboard zugeteilten Queue lesen.
/** @brief Verarbeitet korrelierte ACK/NACK-Antworten des lokalen Bedienauftrags. */
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
        keyboardShow();
    }
}

/** @brief Aktualisiert die PCF8574-LED-Ausgänge und belässt Tasteneingänge auf HIGH.
 *  @param activeButtons LED-Bitmaske der vier Tastenplätze.
 */
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

/** @brief Fordert beim Systemstart die gleichzeitige Ruhelage aller Tasten an. */
void keyboardShowReleaseHint() {
    if (!kbOledReady) return;
    kbDisplay.clearBuffer();
    kbDisplay.drawStr(0, 0,  "FY UVB / LOCAL");
    kbDisplay.drawStr(0, 8,  "Tasten gedrueckt!");
    kbDisplay.drawStr(0, 16, "Bitte alle Tasten");
    kbDisplay.drawStr(0, 24, "loslassen");
    kbDisplay.sendBuffer();
}

// LEDs zeigen den echten Betriebszustand statt nur den Tastendruck.
// Bei Bewegung hat die entsprechende blaue Richtungs-LED Vorrang.
/** @brief Überträgt STOP-, Treiber- und Richtungssnapshots auf die LEDs. */
void updateStatusLeds() {
    if (!kbPcfReady || !kbSplashDone) return;
    uint8_t leds = 0U;
    if (Muninn_motorState() == MotorState_t::STOPPED) leds |= 0x01U;
    if (Muninn_isDriverEnabled()) leds |= 0x04U;
    const uint8_t direction = Muninn_motionDirectionSnapshot();
    if (direction == 1U) leds |= 0x02U; // links (G39)
    else if (direction == 2U) leds |= 0x08U; // rechts (G36)
    keyboardWriteLeds(leds);
}

/** @brief Stellt den aktuellen lokalen Menüzustand am OLED dar. */
void keyboardShow() {
    if (!kbOledReady) return;
    kbDisplay.clearBuffer();
    kbDisplay.drawStr(0, 0, "FY UVB / LOCAL");
    if (kbMenu == Menu::LOCKED) {
        kbDisplay.drawStr(0, 8,  "STOP verriegelt");
        kbDisplay.drawStr(0, 16, "LINKS halten");
        kbDisplay.drawStr(0, 24, "dann GO");
    } else if (kbMenu == Menu::REF_MENU) {
        kbDisplay.drawStr(0, 8,  kbLongRef ? "REF: LONG" : "REF: SHORT");
        kbDisplay.drawStr(0, 16, "LINKS/RECHTS Wahl");
        kbDisplay.drawStr(0, 24, "GO: Start");
    } else if (kbMenu == Menu::TRACK_MENU) {
        char trackLine[] = "Ziel BG0";
        trackLine[7] = static_cast<char>('0' + kbTrack); // BG1..BG5
        kbDisplay.drawStr(0, 8,  trackLine);
        kbDisplay.drawStr(0, 16, "LINKS/RECHTS Wahl");
        kbDisplay.drawStr(0, 24, "GO: Fahren");
    } else if (kbMenu == Menu::REF_RUN) {
        kbDisplay.drawStr(0, 8,  "Referenzfahrt");
        kbDisplay.drawStr(0, 16, "danach BG3");
    } else if (kbMenu == Menu::TRACK_RUN) {
        kbDisplay.drawStr(0, 8, "Gleisfahrt aktiv");
    } else if (kbMenu == Menu::ERROR) {
        kbDisplay.drawStr(0, 8, "FEHLER / STOP");
    } else {
        kbDisplay.drawStr(0, 8, "Motorfreigabe");
    }
    kbDisplay.sendBuffer();
}

/** @brief Prüft I2C-Geräte und zeigt das fünfsekündige Startbild an.
 *  @note Erwartet einen bereits initialisierten Wire-Bus.
 */
void keyboardHardwareBegin() {
    Wire.beginTransmission(KB_PCF_ADDRESS);
    kbPcfReady = (Wire.endTransmission() == 0U);
    if (kbPcfReady) {
        Wire.beginTransmission(KB_PCF_ADDRESS);
        Wire.write(0xFFU);  // LEDs aus; P4..P7 als Eingänge frei
        kbPcfReady = (Wire.endTransmission() == 0U);
    }

    // U8g2::begin() meldet auch ohne physisches Display Erfolg.
    // Deshalb zuerst den tatsächlichen I2C-ACK des OLED prüfen.
    Wire.beginTransmission(KB_OLED_ADDRESS);
    kbOledReady = (Wire.endTransmission() == 0U);
    if (kbOledReady) {
        kbDisplay.setI2CAddress(static_cast<uint8_t>(KB_OLED_ADDRESS << 1U));
        kbOledReady = kbDisplay.begin();
    }
    if (kbOledReady) {
        kbDisplay.setFont(u8g2_font_helvB08_tr);
        kbDisplay.setFontPosTop();
        kbDisplay.clearBuffer();
        kbDisplay.drawStr(0, 0,  "Hello my friend");
        kbDisplay.drawStr(0, 8,  "FY ESP32 / HW3");
        kbDisplay.drawStr(0, 16, "I2C OK");
        kbDisplay.drawStr(0, 24, "Keyboard Test");
        kbDisplay.sendBuffer();
    }
    Serial.print(F("KEYBOARD: PCF8574="));
    Serial.print(kbPcfReady ? F("OK") : F("ERROR"));
    Serial.print(F(" OLED="));
    Serial.println(kbOledReady ? F("OK") : F("ERROR"));
    kbLastChangeMs = millis();
    kbSplashAt = millis();
}

/** @brief Liest und entprellt Tasten zyklisch und verarbeitet gültige Bedienflanken.
 *  @note Beim Start wird zunächst die gleichzeitige Ruhelage aller Tasten verlangt.
 */
void keyboardScanButtons() {
    const uint32_t now = millis();
    if (!kbSplashDone && now - kbSplashAt >= 5000U) {
        kbSplashDone = true;
        keyboardShow();
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
    // Vor der ersten Bedienflanke muessen alle vier Tasten gleichzeitig frei sein.
    // Absoluter Pegel statt einer Flankenerkennung: gehaltene Starttasten
    // koennen damit weder STOP noch Freigabe- oder Menueaktionen ausloesen.
    if (!kbStartReleased) {
        if (active != 0U) {
            if (!kbStartWarningShown) {
                keyboardShowReleaseHint();
                kbStartWarningShown = true;
            }
            return;
        }
        kbStartReleased = true;
        kbLastRaw = 0U;
        kbStable = 0U;
        kbLastChangeMs = now;
        keyboardShow();
        return;
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
    keyboardShow();
}

/** @brief Prüft den bestätigten Fahrtabschluss und schaltet das Menü weiter. */
void progress() {
    if (kbWaiting) return;
    const auto state = Muninn_motorState();
    const uint32_t track = Muninn_trackSnapshot();
    if (kbMenu == Menu::REF_RUN) {
        if (Muninn_referenceStatusSnapshot() == 1U &&
            ((track >> 8U) & 0xFFU) == 3U &&
            ((track >> 16U) & 0xFFU) == TRACK_STATUS_REACHED &&
            !Muninn_isMoving() && state == MotorState_t::IDLE) {
            kbTrack = 3U; kbMenu = Menu::TRACK_MENU; keyboardShow();
        } else if (state == MotorState_t::ERROR) {
            kbMenu = Menu::ERROR; keyboardShow();
        }
    } else if (kbMenu == Menu::TRACK_RUN) {
        if (state == MotorState_t::ERROR) {
            kbMenu = Menu::ERROR; keyboardShow();
        } else if (!Muninn_isMoving() && state == MotorState_t::IDLE &&
                   ((track >> 8U) & 0xFFU) == kbTrack &&
                   ((track >> 16U) & 0xFFU) == TRACK_STATUS_REACHED) {
            kbMenu = Menu::TRACK_MENU; keyboardShow();
        }
    }
}
} // namespace

/** @brief Initialisiert I2C-Keyboard und lokalen Befehlsrückkanal.
 *  @param context Gemeinsamer Kontext mit Befehls- und Antwortqueues.
 *  @note Der Kontext muss während der gesamten Betriebszeit gültig bleiben.
 */
void Keyboard_begin(BringupContext* context) { kbContext = context; keyboardHardwareBegin(); }

/** @brief Pollt lokale Antworten, Tasten, Fahrstatus und LEDs.
 *  @note Regelmäßig aus dem Arduino-Hauptloop aufrufen; kein direkter Motor-GPIO-Zugriff.
 */
void Keyboard_poll() { pollLocalReply(); keyboardScanButtons(); progress(); updateStatusLeds(); }
} // namespace fy
