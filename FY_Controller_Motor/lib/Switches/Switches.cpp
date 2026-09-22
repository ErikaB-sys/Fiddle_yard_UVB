
#include "Switches.h"

void Switches::begin(uint8_t Ref_in ,uint8_t end_L_in ,uint8_t end_R_in ,uint8_t  Belt_in )
{
    pinMode(Ref_in, INPUT);
    pinMode(end_L_in, INPUT);
    pinMode(end_R_in, INPUT);
    pinMode(Belt_in, INPUT);
}

int Switches::getAnalogValue(Id id) const
{
    const uint8_t pin = analogPin(id);

    if (pin == No_ANALOG)
        return No_ANALOG;

    return analogRead(pin);
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

    return 0xFF;
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
