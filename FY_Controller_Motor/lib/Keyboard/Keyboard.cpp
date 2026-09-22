#include "Keyboard.h"
#include <Wire.h>

Keyboard::Button::Button(uint8_t pin)
    : _pin(pin),
      _lastState(HIGH),
      _lastDebounceTime(0) {
}

void Keyboard::Button::begin() {
    pinMode(_pin, INPUT_PULLUP);
}

bool Keyboard::Button::pressed() {
    const bool reading = digitalRead(_pin);
    bool pressedNow = false;

    if (reading != _lastState) {
        const uint32_t now = millis();

        if ((now - _lastDebounceTime) > DEBOUNCE_MS) {
            // Generate one event only on HIGH -> LOW.
            if (_lastState == HIGH && reading == LOW) {
                pressedNow = true;
            }
            _lastState = reading;
        }

        _lastDebounceTime = now;
    }

    return pressedNow;
}

Keyboard::Keyboard(uint8_t leftPin,
                   uint8_t rightPin,
                   uint8_t okPin,
                   uint8_t stopPin,
                   uint8_t sdaPin,
                   uint8_t sclPin,
                   uint8_t displayAddress)
    : _left(leftPin),
      _right(rightPin),
      _ok(okPin),
      _stop(stopPin),
      _display(U8G2_R0, U8X8_PIN_NONE) {
    (void)sdaPin;
    (void)sclPin;

    // The current Nano implementation uses the AVR Wire pins.
    // The constructor keeps SDA/SCL explicit so the hardware mapping
    // can be changed later without changing the public module concept.
    _display.setI2CAddress(displayAddress * 2);
}

void Keyboard::begin() {
    _left.begin();
    _right.begin();
    _ok.begin();
    _stop.begin();

    Wire.begin();

    _display.begin();
    _display.setFont(u8g2_font_6x12_tf);
    draw();
}

Keyboard::Event Keyboard::update() {
    // STOP has highest local priority.
    if (_stop.pressed()) {
        return Event::STOP;
    }

    if (_left.pressed()) {
        return Event::TRACK_PREVIOUS;
    }

    if (_right.pressed()) {
        return Event::TRACK_NEXT;
    }

    if (_ok.pressed()) {
        return Event::OK;
    }

    return Event::NONE;
}

void Keyboard::setMode(Mode mode) {
    if (_mode != mode) {
        _mode = mode;
        draw();
    }
}

void Keyboard::setTrack(uint8_t track) {
    if (_track != track) {
        _track = track;
        draw();
    }
}

void Keyboard::setMoving(bool moving) {
    if (_moving != moving) {
        _moving = moving;
        draw();
    }
}

void Keyboard::setError(uint8_t errorCode) {
    _errorCode = errorCode;
    _hasError = true;
    _moving = false;
    draw();
}

void Keyboard::clearError() {
    if (_hasError) {
        _hasError = false;
        _errorCode = 0;
        draw();
    }
}

void Keyboard::setLastCommand(uint8_t commandId) {
    _lastCommand = commandId;
}

bool Keyboard::hasError() const {
    return _hasError;
}

Keyboard::Mode Keyboard::mode() const {
    return _mode;
}

const char* Keyboard::modeText(Mode mode) {
    return mode == Mode::LOCAL ? "LOCAL" : "REMOTE";
}

void Keyboard::drawHeader() {
    _display.setFont(u8g2_font_6x12_tf);
    _display.setCursor(0, 10);
    _display.print(modeText(_mode));
    _display.print(" | FY UVB");
    _display.drawHLine(0, 13, 128);
}

void Keyboard::drawReady() {
    _display.setFont(u8g2_font_helvR08_tf);

    _display.setCursor(0, 34);
    _display.print("TRACK ");

    if (_track < 10) {
        _display.print('0');
    }
    _display.print(_track);

    _display.setCursor(0, 53);
    _display.print("READY");
}

void Keyboard::drawMoving() {
    _display.setFont(u8g2_font_helvR08_tf);

    _display.setCursor(0, 34);
    _display.print("TRACK ");

    if (_track < 10) {
        _display.print('0');
    }
    _display.print(_track);

    _display.setCursor(0, 53);
    _display.print("MOVING");

    // Intentionally no animation yet.
}

void Keyboard::drawError() {
    _display.setFont(u8g2_font_helvR08_tf);

    _display.setCursor(0, 36);
    _display.print("ERROR");

    _display.setCursor(0, 54);
    _display.print('E');
    if (_errorCode < 10) {
        _display.print('0');
    }
    _display.print(_errorCode);
}

void Keyboard::drawDiagnostic() {
    // Reserved for the development view.
    // The normal display deliberately does not show the last command.
}

void Keyboard::draw() {
    _display.firstPage();

    do {
        _display.clearBuffer();
        drawHeader();

        if (_hasError) {
            drawError();
        } else if (_moving) {
            drawMoving();
        } else {
            drawReady();
        }
    } while (_display.nextPage());
}
