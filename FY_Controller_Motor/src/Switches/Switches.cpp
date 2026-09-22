#include "Switches.h"

void Switches::begin()
{
    pinMode(SWITCH_REF_D, INPUT);
    pinMode(SWITCH_TRIM_LEFT_D, INPUT);
    pinMode(SWITCH_TRIM_RIGHT_D, INPUT);
    pinMode(SWITCH_TIMING_BELT_D, INPUT);
}

int Switches::getAnalogValue(Id id) const
{
    return analogRead(analogPin(id));
}

bool Switches::getDigitalValue(Id id) const
{
    return digitalRead(digitalPin(id)) == HIGH;
}

uint8_t Switches::analogPin(Id id)
{
    switch (id) {
        case Id::REF:         return SWITCH_REF_A;
        case Id::TRIM_LEFT:   return SWITCH_TRIM_LEFT_A;
        case Id::TRIM_RIGHT:  return SWITCH_TRIM_RIGHT_A;
        case Id::TIMING_BELT: return SWITCH_TIMING_BELT_A;
    }

    return A0;
}

uint8_t Switches::digitalPin(Id id)
{
    switch (id) {
        case Id::REF:         return SWITCH_REF_D;
        case Id::TRIM_LEFT:   return SWITCH_TRIM_LEFT_D;
        case Id::TRIM_RIGHT:  return SWITCH_TRIM_RIGHT_D;
        case Id::TIMING_BELT: return SWITCH_TIMING_BELT_D;
    }

    return SWITCH_REF_D;
}
