#pragma once
#include "FY_Bringup.h"
namespace fy {
/** @brief Initialisiert das lokale Keyboard und seine Befehlsanbindung.
 *  @param context Dauerhaft gültiger Bringup-Kontext mit Befehls- und Antwortqueues.
 */
void Keyboard_begin(BringupContext* context);
/** @brief Pollt Tasten und Status ohne direkten Motorzugriff. */
void Keyboard_poll();
}
