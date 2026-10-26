#pragma once
#include <stdint.h>

namespace fy {
/**
 * @brief Verbindliche Bereichs-IDs fuer alle NVM-bezogenen Operationen.
 *
 * Die Nummern bleiben auf dem Draht und im NVM stabil.
 * Die Zulassung eines Befehls wird getrennt vom Bereich geprueft.
 * MOTOR_PARAMETERS ist reserviert, bis ein Descriptor existiert.
 */
enum class NvmArea : uint8_t {
    CALIBRATION = 0x00U,
    TRACK_POSITIONS = 0x01U,
    MOTOR_PARAMETERS = 0x02U
};
} // namespace fy
