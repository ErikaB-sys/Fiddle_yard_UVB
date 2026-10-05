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
 * Fixed FY hardware configuration is taken from Config.h.
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

    Keyboard();

    void begin();

    Event update();

    void setMode(Mode mode);
    void setTrack(uint8_t track);
    void setMoving(bool moving);
    void setError(uint8_t errorCode);
    void clearError();
    void setLastCommand(uint8_t commandId);
    void Write_to_display(const int8_t* string, uint8_t pos_x, uint8_t pos_y);
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
