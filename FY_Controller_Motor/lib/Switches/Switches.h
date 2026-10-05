#pragma once

#include <Arduino.h>
#include "Config.h"

class Switches {
public:
    enum class Id : uint8_t {
        REF = SWITCH_REF_ID,
        TRIM_LEFT = SWITCH_TRIM_LEFT_ID,
        TRIM_RIGHT = SWITCH_TRIM_RIGHT_ID,
        TIMING_BELT = SWITCH_TIMING_BELT_ID
    };

    void begin();

    int getAnalogValue(Id id) const;
    bool getDigitalValue(Id id) const;

private:
    static uint8_t analogPin(Id id);
    static uint8_t digitalPin(Id id);
};
