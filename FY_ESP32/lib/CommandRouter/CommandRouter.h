#pragma once

/**
 * @file CommandRouter.h
 * @brief Gemeinsame Uebergabe bereits decodierter Motorbefehle an Muninn.
 */
#include "FY_Bringup.h"

namespace fy {

/** @brief Legt einen geprueften Auftrag in Muninns normale oder STOP-Queue.
 * @param context Der zentrale FreeRTOS-Kontext.
 * @param command Binaerer Auftrag mit reservierter Quellen-Sequenzkennung.
 * @param priority true nur fuer priorisierte STOP-Kommandos.
 * @return true, wenn die Nachricht nicht blockierend eingereiht wurde.
 * @note Ein Queue-ACK ist NICHT Muninns Annahme-ACK.
 */
bool CommandRouter_enqueue(BringupContext* context,
                           const CommandMessage& command, bool priority);

} // namespace fy
