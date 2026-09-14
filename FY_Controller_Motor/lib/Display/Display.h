#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

// Initialisierung
void display_init();

// Anzeige aktualisieren
void display_update();

// Anzeigeinformationen
void display_setStatus(const char* status);
void display_setPosition(long position);
void display_setTarget(long position);
void display_setTrack(uint8_t track);

#endif