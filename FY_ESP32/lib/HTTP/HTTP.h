#pragma once

/**
 * @file HTTP.h
 * @brief HTTP-Eingang auf Huginn; erster Integrationsstand ohne Motorbefehle.
 */
#include "FY_Bringup.h"

namespace fy {

/** @brief Legt die Diagnosequeue vor dem Start der Kommunikationstasks an.
 * @param context Gemeinsame FreeRTOS-Strukturen (nicht Eigentum des HTTP-Moduls).
 * @return true, wenn die HTTP-Diagnose vorbereitet ist.
 */
bool HTTP_prepare(BringupContext* context);

/** @brief Startet WLAN/HTTP neben UART auf Huginn/Core 0, niemals im Motor-Task.
 * @param context Bereits initialisierter Bringup-Kontext.
 * @return pdPASS bei gestartetem Task oder bewusst deaktiviertem WLAN.
 */
BaseType_t Huginn_startHTTP(BringupContext* context);

/** @brief Gibt nur neue HTTP-RX-/TX-Diagnosen ueber UART aus.
 * @note Ausschliesslich vom bestehenden UART-Task aufrufen; kein Status-Poll-Trace.
 */
void HTTP_flushTrace();

} // namespace fy
