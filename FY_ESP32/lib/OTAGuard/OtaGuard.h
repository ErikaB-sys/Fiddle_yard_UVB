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
#if defined(FY_OTA_BENCH_TEST_ARM) && FY_OTA_BENCH_TEST_ARM
/** @brief Tischmuster-Freigabe nur per lokalem USB-UART und Test-Build.
 *  @return true nur bei STOP-Verriegelung, abgeschaltetem Treiber und Stillstand.
 *  @warning Dieser Aufruf existiert in Produktiv-Builds nicht.
 */
bool OtaGuard_armFromBenchSerial();
#endif
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
/** @brief Nur Anzeigewert 0..100, sicher von beiden Kernen lesbar. */
void OtaGuard_setProgress(uint8_t percent);
uint8_t OtaGuard_progress();
} // namespace fy
