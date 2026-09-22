#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

/**
 * FY UVB local keyboard and status display.
 *
 * The module deliberately keeps the public interface small:
 * - four local buttons generate events
 * - the controller supplies the status shown on the OLED
 * - LOCAL/REMOTE is a controller state, not a UI-only state
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
             uint8_t displayAddress = 0x3C);

    void begin();
    Event update();

    void setMode(Mode mode);
    void setTrack(uint8_t track);
    void setMoving(bool moving);
    void setError(uint8_t errorCode);
    void clearError();
    void setLastCommand(uint8_t commandId);

    bool hasError() const;
    Mode mode() const;

private:
    class Button {
    public:
        explicit Button(uint8_t pin);

        void begin();
        bool pressed();

    private:
        uint8_t _pin;
        bool _lastState;
        uint32_t _lastDebounceTime;
        static constexpr uint16_t DEBOUNCE_MS = 50;
    };

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

    void draw();
    void drawHeader();
    void drawReady();
    void drawMoving();
    void drawError();
    void drawDiagnostic();

    static const char* modeText(Mode mode);
};
