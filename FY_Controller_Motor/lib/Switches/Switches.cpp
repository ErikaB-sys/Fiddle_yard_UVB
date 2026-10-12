#include "Switches.h"

void Switches::begin()
{
    // Centralized input initialization for all physical sensors.
    // REF, TRIM_LEFT and TRIM_RIGHT are also read directly by the motor ISR.
    pinMode(LSREF.pin, INPUT);
    pinMode(LSL.pin, INPUT);
    pinMode(LSR.pin, INPUT);
    pinMode(LS4.pin, INPUT);
}

int Switches::getAnalogValue(Id id) const
{
    const uint8_t pin = analogPin(id);

    if (pin == NO_ANALOG) {
        return NO_ANALOG;
    }

    return analogRead(pin);
}

bool Switches::getDigitalValue(Id id) const
{
    return digitalRead(digitalPin(id)) == HIGH;
}

uint8_t Switches::analogPin(Id id)
{
    switch (id) {
        case Id::REF:
            return SWITCH_REF_A;
        case Id::TRIM_LEFT:
            return SWITCH_TRIM_LEFT_A;
        case Id::TRIM_RIGHT:
            return SWITCH_TRIM_RIGHT_A;
        case Id::TIMING_BELT:
            return SWITCH_TIMING_BELT_A;
    }

    return NO_ANALOG;
}

uint8_t Switches::digitalPin(Id id)
{
    switch (id) {
        case Id::REF:
            return LSREF.pin;
        case Id::TRIM_LEFT:
            return LSL.pin;
        case Id::TRIM_RIGHT:
            return LSR.pin;
        case Id::TIMING_BELT:
            return LS4.pin;
    }

    return LSREF.pin;
}
