#include "OtaGuard.h"
#include "Motor.h"
#include <atomic>

namespace fy {
namespace {
constexpr uint32_t kArmWindowMs = 120000UL;
std::atomic<uint32_t> armDeadline{0U};
std::atomic<bool> otaLocked{false};
std::atomic<uint8_t> otaPercent{0U};

bool stoppedAndDisabled() {
    return Muninn_isStopLatched() &&
           !Muninn_isDriverEnabled() &&
           !Muninn_isMoving() &&
           Muninn_motorState() == MotorState_t::STOPPED;
}
} // namespace

bool OtaGuard_armFromStop() {
    if (otaLocked.load(std::memory_order_acquire) || !stoppedAndDisabled())
        return false;
    armDeadline.store(millis() + kArmWindowMs, std::memory_order_release);
    return true;
}

#if defined(FY_OTA_BENCH_TEST_ARM) && FY_OTA_BENCH_TEST_ARM
bool OtaGuard_armFromBenchSerial() {
    // Exakt dieselben Motorbedingungen wie bei physischem STOP.
    // Lediglich die Quellenfreigabe (USB statt Taste) unterscheidet sich.
    return OtaGuard_armFromStop();
}
#endif

uint32_t OtaGuard_armRemainingMs() {
    if (otaLocked.load(std::memory_order_acquire)) return 0U;
    const uint32_t until = armDeadline.load(std::memory_order_acquire);
    if (until == 0U || !stoppedAndDisabled()) return 0U;
    const int32_t remaining = static_cast<int32_t>(until - millis());
    return remaining > 0 ? static_cast<uint32_t>(remaining) : 0U;
}

bool OtaGuard_isArmed() {
    return OtaGuard_armRemainingMs() != 0U;
}

bool OtaGuard_tryLock() {
    if (!OtaGuard_isArmed()) return false;
    bool expected = false;
    if (!otaLocked.compare_exchange_strong(expected, true,
                                          std::memory_order_acq_rel))
        return false;
    // Nach dem Setzen der Sperre ein zweites Mal pruefen: keine
    // Freigabe/Race zwischen Browserpruefung und Motorsperre.
    if (!stoppedAndDisabled()) {
        otaLocked.store(false, std::memory_order_release);
        return false;
    }
    armDeadline.store(0U, std::memory_order_release);
    otaPercent.store(0U, std::memory_order_release);
    return true;
}

void OtaGuard_unlock() {
    otaLocked.store(false, std::memory_order_release);
    armDeadline.store(0U, std::memory_order_release);
}

void OtaGuard_setProgress(uint8_t percent) {
    otaPercent.store(percent > 100U ? 100U : percent,
                     std::memory_order_release);
}

uint8_t OtaGuard_progress() {
    return otaPercent.load(std::memory_order_acquire);
}

bool OtaGuard_active() {
    return otaLocked.load(std::memory_order_acquire);
}
} // namespace fy
