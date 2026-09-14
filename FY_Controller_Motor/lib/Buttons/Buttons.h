#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>

enum class Button
{
    NONE,
    GRUEN,
    ROT,
    GELB_1,
    GELB_2
    };

// Initialisierung
void buttons_init(const uint8_t Pin_gruen, const uint8_t Pin_rot,const uint8_t Pin_gelb_1,const uint8_t Pin_gelb_2);
void buttons_init(const uint8_t LED_gruen, const uint8_t LED_rot,const uint8_t LED_gelb_1,const uint8_t LED_gelb_2);

// Taste abfragen
Button buttons_get();


#endif