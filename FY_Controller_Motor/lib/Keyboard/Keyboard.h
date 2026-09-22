#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include <PCF8574.h>
#include "Config.h"

/**
 * FY UVB local keyboard and status display.
 *
 * The module deliberately keeps the public interface small:
 * - four local buttons generate events
 * - the controller supplies the status shown on the OLED
 * - LOCAL/REMOTE is a controller state, not a UI-only state
 *
 * Button arguments use the virtual button definitions from Config.h.
 * The physical buttons are connected to the I2C port expander.
 *
 * The implementation uses a page-buffered U8g2 display to keep RAM usage
 * suitable for the ATmega328P.
 */
class Keyboard {
public:
    enum class Mode : uint8_t {
        LOCAL,
        REMOTE
    };

    enum class Event : uint8_t {
        NONE,
        TRACK_PREVIOUS,
        TRACK_NEXT,
        OK,
        STOP
    };

    Keyboard(uint8_t leftPin,
             uint8_t rightPin,
             uint8_t okPin,
             uint8_t stopPin,
             uint8_t sdaPin,
             uint8_t sclPin,
             uint8_t displayAddress = DISPLAY_ADDRESS,
             uint8_t extenderAddress = PORT_EXPANDER_ADDRESS);

    void begin();
    Event update();

    void setMode(Mode mode);
    void setTrack(uint8_t track);
    void setMoving(bool moving);
    void setError(uint8_t errorCode);
    void clearError();
    void setLastCommand(uint8_t commandId);

    bool hasError() const;
    bool is_connected() const;
    Mode mode() const;

private:
    class Button {
    public:
        Button(PCF8574* extender, uint8_t pin);

        void begin();
        bool pressed();

    private:
        PCF8574* _extender;
        uint8_t _pin;
        bool _lastState;
        uint32_t _lastDebounceTime;
        static constexpr uint16_t DEBOUNCE_MS = 50;
    };

    PCF8574 _extender;

    Button _left;
    Button _right;
    Button _ok;
    Button _stop;

    U8G2_SSD1306_128X64_NONAME_1_HW_I2C _display;

    uint8_t _displayAddress;
    uint8_t _extenderAddress;
    uint8_t _sdaPin;
    uint8_t _sclPin;

    Mode _mode = Mode::LOCAL;
    uint8_t _track = 0;
    uint8_t _errorCode = 0;
    uint8_t _lastCommand = 0;
    bool _moving = false;
    bool _hasError = false;
    bool _connected = false;
    bool _displayConnected = false;
    bool _extenderConnected = false;

    void draw();
    void drawHeader();
    void drawReady();
    void drawMoving();
    void drawError();
    void drawDiagnostic();

    static bool i2cDevicePresent(uint8_t address);
    static uint8_t buttonToExtenderPin(uint8_t virtualPin);
    static const char* modeText(Mode mode);
};
