#pragma once

/**
 * @file HTTP.h
 * @brief HTTP-Eingang auf Huginn mit getrennter Motorantwort-Queue und Diagnose.
 */
#include "FY_Bringup.h"

namespace fy {

/** @brief Bedienhoheit: LOCAL, Browser oder spaetere Automatik. */
enum class ControlMode : uint8_t { LOCAL = 0U, REMOTE = 1U, AUTO = 2U };
/** @brief Atomarer Snapshot der vom HTTP-Task verwalteten Bedienhoheit. */
ControlMode HTTP_controlMode();

/** @brief Bereitet Kontext und nicht blockierende HTTP-Diagnosequeue vor.
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
