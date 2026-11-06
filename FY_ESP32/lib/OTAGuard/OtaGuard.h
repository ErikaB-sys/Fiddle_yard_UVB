#pragma once

/** @file OtaGuard.h
 *  @brief Physische OTA-Freigabe und kernuebergreifende Motorsperre.
 */
#include <Arduino.h>

namespace fy {
/** @brief Schaltet nach langem physischen STOP OTA fuer 120 s frei.
 * @return true, falls STOP-Verriegelung und ausgeschalteter Motor bestaetigt.
 */
bool OtaGuard_armFromStop();
/** @brief Meldet, ob die physische OTA-Freigabe noch gueltig ist. */
bool OtaGuard_isArmed();
/** @brief Restlaufzeit der Freigabe in Millisekunden. */
uint32_t OtaGuard_armRemainingMs();
/** @brief Sperrt alle Fahr-/Entriegelungsbefehle vor OTA-Schreibbeginn. */
bool OtaGuard_tryLock();
/** @brief Hebt die Motorsperre nach Fehler/Abbruch auf, nie automatisch GO. */
void OtaGuard_unlock();
/** @brief Motor-Task und CommandRouter duerfen nur STOP waehrend OTA annehmen. */
bool OtaGuard_active();
} // namespace fy
