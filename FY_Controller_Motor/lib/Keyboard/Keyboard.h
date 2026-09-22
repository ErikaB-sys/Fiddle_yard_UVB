#pragma once

#include <Arduino.h>

enum class Button_ID_t
{
    NONE,
    GRUEN,
    ROT,
    GELB_1,
    GELB_2
    };

enum class LED_ID_t 
{
    NONE,
    GRUEN,
    ROT,
    GELB_1,
    GELB_2
    };



uint8_t KeyboardAdress;
// Keybord  describtor 
struct Button_t 
{
    LED_ID_t LED_ID;
    Button_ID_t Button_ID;
    uint8_t LED_Pin;
    uint8_t Button_Pin;
};




// Initialisierung
void Buttons_init(const uint8_t Pin_gruen, const uint8_t Pin_rot,const uint8_t Pin_gelb_1,const uint8_t Pin_gelb_2);
void LED_init(const uint8_t LED_gruen, const uint8_t LED_rot,const uint8_t LED_gelb_1,const uint8_t LED_gelb_2);

// Get  funktions 
uint8_t  Get_KB_Adress(); 

Button_t buttons_get();
bool Set_LED(LED_ID_t ID);


