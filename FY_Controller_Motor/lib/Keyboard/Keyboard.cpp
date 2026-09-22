#include "Keyboard.h"
#include <Wire.h>

Keyboard::Button::Button(PCF8574* extender, uint8_t pin)
    : _extender(extender),
      _pin(pin),
      _lastState(HIGH),
      _lastDebounceTime(0) {
}

void Keyboard::Button::begin() {
    _extender->pinMode(_pin, INPUT);
    _lastState = _extender->digitalRead(_pin);
}

bool Keyboard::Button::pressed() {
    const bool reading = _extender->digitalRead(_pin);
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

Keyboard::Keyboard()
    : _extender(PORT_EXPANDER_ADDRESS),
      _left(&_extender, buttonToExtenderPin(BUTTON_LEFT)),
      _right(&_extender, buttonToExtenderPin(BUTTON_RIGHT)),
      _ok(&_extender, buttonToExtenderPin(BUTTON_GO)),
      _stop(&_extender, buttonToExtenderPin(BUTTON_STOP)),
      _display(U8G2_R0, U8X8_PIN_NONE),
      _displayAddress(DISPLAY_ADDRESS),
      _extenderAddress(PORT_EXPANDER_ADDRESS),
      _sdaPin(A4),
      _sclPin(A5) {
    // U8g2 expects the I2C address multiplied by two.
    _display.setI2CAddress(_displayAddress * 2);
}

bool Keyboard::i2cDevicePresent(uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

uint8_t Keyboard::buttonToExtenderPin(uint8_t virtualPin) {
    // Virtual button definitions are part of Config.h.
    // Physical assignment follows the tested FY hardware:
    // STOP=P7, GO=P6, LEFT=P5, RIGHT=P4.
    switch (virtualPin) {
        case BUTTON_STOP:
            return P7;
        case BUTTON_GO:
            return P6;
        case BUTTON_LEFT:
            return P5;
        case BUTTON_RIGHT:
            return P4;
        default:
            return P7;
    }
}

void Keyboard::begin(uint8_t leftPin,
                     uint8_t rightPin,
                     uint8_t okPin,
                     uint8_t stopPin,
                     uint8_t sdaPin,
                     uint8_t sclPin,
                     uint8_t displayAddress,
                     uint8_t extenderAddress) {
    _displayAddress = displayAddress;
    _extenderAddress = extenderAddress;
    _sdaPin = sdaPin;
    _sclPin = sclPin;

    // Button mapping is configured explicitly at system startup.
    _left = Button(&_extender, buttonToExtenderPin(leftPin));
    _right = Button(&_extender, buttonToExtenderPin(rightPin));
    _ok = Button(&_extender, buttonToExtenderPin(okPin));
    _stop = Button(&_extender, buttonToExtenderPin(stopPin));

    // U8g2 expects the I2C address multiplied by two.
    _display.setI2CAddress(_displayAddress * 2);

    Wire.begin();

    _left.begin();
    _right.begin();
    _ok.begin();
    _stop.begin();

    _displayConnected = i2cDevicePresent(_displayAddress);
    _extenderConnected = i2cDevicePresent(_extenderAddress);

    if (_extenderConnected) {
        // P0..P3 are LEDs, P4..P7 are the four buttons.
        _extender.pinMode(P0, OUTPUT);
        _extender.pinMode(P1, OUTPUT);
        _extender.pinMode(P2, OUTPUT);
        _extender.pinMode(P3, OUTPUT);

        _extender.pinMode(P4, INPUT);
        _extender.pinMode(P5, INPUT);
        _extender.pinMode(P6, INPUT);
        _extender.pinMode(P7, INPUT);

        // LEDs are active LOW on the tested hardware.
        _extender.digitalWrite(P0, HIGH);
        _extender.digitalWrite(P1, HIGH);
        _extender.digitalWrite(P2, HIGH);
        _extender.digitalWrite(P3, HIGH);

        _extenderConnected = _extender.begin();
    }

    if (_displayConnected) {
        _display.begin();
        _display.setFont(u8g2_font_6x12_tf);
    }

    _connected = _displayConnected && _extenderConnected;

    if (_displayConnected) {
        draw();
    }
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

bool Keyboard::is_connected() const {
    return _connected;
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
    if (!_displayConnected) {
        return;
    }

    _display.firstPage();

    do {
        // Page-buffer mode clears the current page automatically.
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
