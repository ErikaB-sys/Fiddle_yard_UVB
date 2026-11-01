#include "StepTimer.h"
#include "HW_Config.h"

#include <Arduino.h>
#include <esp_arduino_version.h>
#include <soc/gpio_struct.h>
#include <soc/gpio_reg.h>
#if ESP_ARDUINO_VERSION_MAJOR < 3
#include <soc/timer_group_struct.h> // Klassischer ESP32: TIMG-Register unter Arduino 2.x.
#endif

namespace fy {
namespace {

constexpr uint32_t kStepBit = uint32_t{1} << MOTOR_PWM_PIN;
constexpr uint16_t kMinStepHz = 40U;
constexpr uint16_t kMaxStepHz = 800U;

// Nano-Timer1-Arithmetik beibehalten: 16 MHz / 256 = 62500 Ticks/s.
// Ein Interrupt je STEP-Halbperiode; ein Nano-Reload-Tick entspricht 16 us.
// Nano-OCR1A-Werte nie direkt an den ESP32-Timer mit 1 MHz uebergeben.
constexpr uint32_t kNanoTickUs = 16U;
constexpr uint16_t kDefaultVMin = 120U;
constexpr uint16_t kDefaultVMax = 800U;
// Einfache constexpr-Ausdruecke sind auch mit Arduino GNU++11 uebersetzbar.
constexpr uint32_t nanoDenominator(uint16_t hz) {
    return 2UL * 256UL * static_cast<uint32_t>(hz);
}
constexpr uint16_t nanoReload(uint16_t hz) {
    return static_cast<uint16_t>(
        ((16000000UL + nanoDenominator(hz) / 2UL) /
         nanoDenominator(hz)) - 1UL);
}
constexpr uint16_t kDefaultSlowReload = nanoReload(kDefaultVMin);
constexpr uint16_t kDefaultFastReload = nanoReload(kDefaultVMax);
static_assert(kDefaultSlowReload > kDefaultFastReload, "Nano speed convention changed");
static_assert(kDefaultSlowReload < 65535U, "Nano reload out of range");
// Nur im Stillstand beschreibbar, unveraendert durch ISR gelesen.
volatile uint16_t kSlowReload = kDefaultSlowReload;
volatile uint16_t kFastReload = kDefaultFastReload;
volatile uint16_t kStartReload = kDefaultSlowReload;

hw_timer_t* stepTimer = nullptr;
portMUX_TYPE stepMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool isRunning = false;
volatile bool stepIsHigh = false;
volatile bool isFinished = false;
volatile bool endstopTriggered = false;
volatile uint32_t completed = 0U;
volatile uint32_t target = 0U;
// REF-Ereignisse werden wie beim Nano an der fallenden STEP-Flanke erfasst.
volatile bool referenceMode = false;
volatile bool referenceAway = false;
volatile bool referenceStopOnExit = false;
volatile bool referenceCaptureCoarse = false;
volatile bool referencePrevLevel = false;
volatile uint32_t referencePosition = 0U;
volatile uint32_t referenceCoarse = 0U;
volatile uint32_t referenceEntry = 0U;
volatile uint32_t referenceExit = 0U;
volatile bool referenceCoarseSeen = false;
volatile bool referenceEntrySeen = false;
volatile bool referenceExitSeen = false;
volatile ReferenceEnd referenceEnd = ReferenceEnd::NONE;
/**
 * @brief Liest einen Sensorpegel unmittelbar aus dem ESP32-Eingangsregister.
 * @param pin GPIO-Nummer eines Eingangs ab GPIO32.
 * @return true bei HIGH-Pegel, andernfalls false.
 * @note ISR-tauglicher direkter Registerzugriff; nicht durch Status-Queue ersetzen.
 */
inline bool IRAM_ATTR readSensor(uint8_t pin) {
    // ESP32: GPIO34/36/39 entsprechen den Registerbits 2/4/7.
    return (REG_READ(GPIO_IN1_REG) & (uint32_t{1} << (pin - 32U))) != 0U;
}
void IRAM_ATTR finishReference(ReferenceEnd end) {
    isRunning = false;
    isFinished = true;
    referenceEnd = end;
}

// Nur Muninn schreibt vor isRunning=true unter stepMux einen neuen Fahrplan.
// Waehrend der Fahrt fuehrt ausschliesslich die ISR Phasen und Rampenfortschritt.
StepProfileSegment stages[STEP_PROFILE_PHASES]{};
volatile bool profileActive = false;
volatile uint8_t phaseIndex = 0U;
volatile uint16_t phaseRemaining = 0U;
volatile uint16_t rampCounter = 0U;
volatile uint16_t currentNanoReload = kSlowReload;

void IRAM_ATTR stepTimerIsr() {
    portENTER_CRITICAL_ISR(&stepMux);
    if (!isRunning) {
        GPIO.out_w1tc = kStepBit;
        stepIsHigh = false;
    } else if (!stepIsHigh) {
        // Endschalter in Fahrtrichtung VOR der nächsten STEP-Flanke prüfen.
        if (readSensor(referenceAway ? LSR_PIN : LSL_PIN)) {
            // Der gegenüberliegende Endschalter beeinflusst die Fahrt nicht.
            // Nur die Endlage in Fahrtrichtung ist maßgeblich.
            if (referenceMode) {
                finishReference(ReferenceEnd::ENDSTOP);
            } else {
                isRunning = false;
                isFinished = true;
                endstopTriggered = true;
            }
            GPIO.out_w1tc = kStepBit;
            stepIsHigh = false;
            portEXIT_CRITICAL_ISR(&stepMux);
            return;
        }
        // Steigende Flanke loest den A4988-Motorschritt aus.
        GPIO.out_w1ts = kStepBit;
        stepIsHigh = true;
    } else {
        // Wie beim Nano beendet die fallende Flanke den aktuellen STEP-Puls.
        GPIO.out_w1tc = kStepBit;
        stepIsHigh = false;
        ++completed;
        if (referenceMode) {
            if (referenceAway) ++referencePosition;
            else if (referencePosition > 0U) --referencePosition;
            const bool sensor = readSensor(LSREF_PIN);
            if (!referencePrevLevel && sensor) {
                if (referenceAway && referenceCaptureCoarse &&
                    !referenceCoarseSeen) {
                    referenceCoarse = referencePosition;
                    referenceCoarseSeen = true;
                }
                if (!referenceEntrySeen) {
                    referenceEntry = referencePosition;
                    referenceEntrySeen = true;
                }
            } else if (referencePrevLevel && !sensor) {
                if (referenceEntrySeen) {
                    referenceExit = referencePosition;
                    referenceExitSeen = true;
                    if (referenceStopOnExit)
                        finishReference(ReferenceEnd::REF_EXIT);
                }
            }
            referencePrevLevel = sensor;
        }

        if (profileActive) {
            // Profilzeitverhalten unveraendert. Vor steigender STEP-Flanke
            // wird nur der Endschalter in Fahrtrichtung geprueft.
            // Referenzfahrten erfassen zusaetzlich LSREF-Flanken.

            if (phaseRemaining > 0U)
                --phaseRemaining;

            // Jede Phase aendert ihren Reload nach N abgeschlossenen Schritten.
            // Kleinerer Nano-Reload bedeutet hoehere STEP-Geschwindigkeit.
            const int16_t accel = stages[phaseIndex].accel;
            if (accel != 0) {
                ++rampCounter;
                if (rampCounter >= stages[phaseIndex].rampEvery) {
                    rampCounter = 0U;
                    int32_t next = static_cast<int32_t>(currentNanoReload)
                                 + static_cast<int32_t>(accel);
                    if (next < kFastReload) next = kFastReload;
                    if (next > kSlowReload) next = kSlowReload;
                    currentNanoReload = static_cast<uint16_t>(next);
                }
            }

            if (phaseRemaining == 0U) {
                do {
                    ++phaseIndex;
                } while (phaseIndex < STEP_PROFILE_PHASES &&
                         stages[phaseIndex].steps == 0U);
                if (phaseIndex < STEP_PROFILE_PHASES) {
                    phaseRemaining = stages[phaseIndex].steps;
                    rampCounter = 0U;
                }
            }
        }

        if (isRunning && completed >= target) {
            isRunning = false;
            isFinished = true;
            if (referenceMode) referenceEnd = ReferenceEnd::STEP_LIMIT;
        }
#if ESP_ARDUINO_VERSION_MAJOR < 3
        else if (profileActive) {
            // Arduino-ESP32 2.x: timerBegin(1,80,true) ist Gruppe 1 / Timer 0.
            // Der Alarm laedt automatisch nach. ISR schreibt das naechste
            // Intervall direkt; keine nicht-IRAM-sicheren Timer-APIs aufrufen.
            TIMERG1.hw_timer[0].alarm_low =
                (static_cast<uint32_t>(currentNanoReload) + 1UL) *
                kNanoTickUs;
        }
#endif
    }
    portEXIT_CRITICAL_ISR(&stepMux);
}

void haltAlarm() {
    if (stepTimer == nullptr) return;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    timerStop(stepTimer);
#else
    timerAlarmDisable(stepTimer);
#endif
}

void armAlarm(uint32_t halfPeriodUs) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    timerStop(stepTimer);
    timerWrite(stepTimer, 0U);
    timerAlarm(stepTimer, halfPeriodUs, true, 0U);
    timerStart(stepTimer);
#else
    timerAlarmDisable(stepTimer);
    timerWrite(stepTimer, 0U);
    timerAlarmWrite(stepTimer, halfPeriodUs, true);
    timerAlarmEnable(stepTimer);
#endif
}

bool startPulseTrain(uint16_t steps, bool directionRight,
                     uint32_t halfPeriodUs,
                     const StepProfileSegment* plan,
                     bool refMode = false, bool stopAtExit = false,
                     bool captureCoarse = false) {
    if (stepTimer == nullptr || steps == 0U || halfPeriodUs < 2U)
        return false;

    portENTER_CRITICAL(&stepMux);
    const bool busy = isRunning || isFinished;
    portEXIT_CRITICAL(&stepMux);
    if (busy) return false;

    // Muninn prueft STOP/GO; der Timer aktiviert niemals den Treiber.
    haltAlarm();
    digitalWrite(MOTOR_PWM_PIN, LOW);
    digitalWrite(MOTOR_DIR_PIN, directionRight ? HIGH : LOW);
    // DIR-Vorlauf vor der ersten steigenden STEP-Flanke: >= 625 us im Profilmodus.
    portENTER_CRITICAL(&stepMux);
    completed = 0U;
    target = steps;
    stepIsHigh = false;
    isFinished = false;
    endstopTriggered = false;
    profileActive = (plan != nullptr);
    phaseIndex = 0U;
    phaseRemaining = 0U;
    rampCounter = 0U;
    currentNanoReload = (plan != nullptr) ? kStartReload : kSlowReload;
    referenceMode = refMode;
    referenceAway = directionRight;
    referenceStopOnExit = stopAtExit;
    referenceCaptureCoarse = captureCoarse;
    referencePrevLevel = refMode ? readSensor(LSREF_PIN) : false;
    referenceCoarseSeen = false;
    referenceEntrySeen = false;
    referenceExitSeen = false;
    referenceEnd = ReferenceEnd::NONE;
    if (plan != nullptr) {
        for (uint8_t i = 0U; i < STEP_PROFILE_PHASES; ++i) {
            stages[i].steps = plan[i].steps;
            stages[i].accel = plan[i].accel;
            stages[i].rampEvery = plan[i].rampEvery;
        }
        while (phaseIndex < STEP_PROFILE_PHASES &&
               stages[phaseIndex].steps == 0U)
            ++phaseIndex;
        phaseRemaining = stages[phaseIndex].steps; // prevalidated nonzero
    }
    isRunning = true;
    portEXIT_CRITICAL(&stepMux);

    armAlarm(halfPeriodUs);
    return true;
}

} // anonymous namespace

bool stepTimerBegin() {
    if (stepTimer != nullptr) return true;
    digitalWrite(MOTOR_PWM_PIN, LOW);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    stepTimer = timerBegin(1000000U);
#else
    // Arduino-ESP32 2.x: Timerindex 1 -> TIMG1/T0 mit APB / 80 = 1 MHz.
    stepTimer = timerBegin(1U, 80U, true);
#endif
    if (stepTimer == nullptr) return false;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    timerAttachInterrupt(stepTimer, &stepTimerIsr);
    timerStop(stepTimer);
#else
    timerAttachInterrupt(stepTimer, &stepTimerIsr, true);
    timerAlarmWrite(stepTimer, 4167U, true);
    timerAlarmDisable(stepTimer);
#endif
    return true;
}

bool stepTimerStart(uint16_t steps, bool directionRight, uint16_t stepHz) {
    if (stepHz < kMinStepHz || stepHz > kMaxStepHz)
        return false;
    const uint32_t halfPeriodUs =
        (500000UL + static_cast<uint32_t>(stepHz / 2U)) / stepHz;
    return startPulseTrain(steps, directionRight, halfPeriodUs, nullptr);
}

bool stepTimerConfigureProfile(uint16_t vMin, uint16_t vMax, uint16_t vPos) {
    if (vMin < kMinStepHz || vMax > kMaxStepHz ||
        vPos < kMinStepHz || vMin > vMax || vPos > vMax)
        return false;
    portENTER_CRITICAL(&stepMux);
    const bool busy = isRunning || isFinished;
    if (!busy) {
        // Untere Grenze deckt sowohl Anfahrt als auch langsames POSI ab.
        kSlowReload = nanoReload(vMin < vPos ? vMin : vPos);
        kFastReload = nanoReload(vMax);
        kStartReload = nanoReload(vMin);
    }
    portEXIT_CRITICAL(&stepMux);
    return !busy;
}

bool stepTimerStartProfile(uint16_t steps, bool directionRight,
                           const StepProfileSegment (&segments)[STEP_PROFILE_PHASES],
                           uint16_t startHz) {
    if (startHz < kMinStepHz || startHz > kMaxStepHz) return false;
    uint32_t sum = 0U;
    for (uint8_t i = 0U; i < STEP_PROFILE_PHASES; ++i) {
        sum += segments[i].steps;
        if (segments[i].accel != 0 && segments[i].rampEvery == 0U)
            return false;
    }
    if (steps == 0U || sum != steps) return false;

#if ESP_ARDUINO_VERSION_MAJOR >= 3
    // Arduino-ESP32 3.x benoetigt eine separat gepruefte ISR-sichere
    // Alarm-Neuladung. Kein unbemerktes Profil mit konstanter Geschwindigkeit;
    // dieser Pfad meldet bis zur Portierung NOT_READY.
    (void)directionRight;
    (void)segments;
    (void)startHz;
    return false;
#else
    kStartReload = nanoReload(startHz);
    return startPulseTrain(steps, directionRight,
                           (static_cast<uint32_t>(kStartReload) + 1UL) *
                           kNanoTickUs, segments);
#endif
}

bool stepTimerStartReferenceProfile(uint16_t steps, bool awayMotor,
                           const StepProfileSegment (&segments)[STEP_PROFILE_PHASES],
                           uint16_t startHz) {
    if (startHz < kMinStepHz || startHz > kMaxStepHz) return false;
    uint32_t sum = 0U;
    for (uint8_t i = 0U; i < STEP_PROFILE_PHASES; ++i) {
        sum += segments[i].steps;
        if (segments[i].accel != 0 && segments[i].rampEvery == 0U)
            return false;
    }
    if (steps == 0U || sum != steps) return false;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    (void)awayMotor;
    (void)segments;
    (void)startHz;
    return false;
#else
    kStartReload = nanoReload(startHz);
    return startPulseTrain(steps, awayMotor,
                           (static_cast<uint32_t>(kStartReload) + 1UL) *
                           kNanoTickUs,
                           segments, true, false, false);
#endif
}

void stepTimerReferenceSetPosition(uint32_t position) {
    portENTER_CRITICAL(&stepMux);
    if (!isRunning) referencePosition = position;
    portEXIT_CRITICAL(&stepMux);
}

bool stepTimerStartReference(bool awayMotor, uint16_t stepHz,
                             bool stopOnRefExit, bool captureCoarse,
                             uint16_t steps) {
    if (stepHz < kMinStepHz || stepHz > kMaxStepHz) return false;
    const uint32_t halfPeriodUs =
        (500000UL + static_cast<uint32_t>(stepHz / 2U)) / stepHz;
    // Timer-Kapazitaetsgrenze, keine sicherheitsgepruefte Wegbegrenzung.
    return startPulseTrain(steps, awayMotor, halfPeriodUs, nullptr,
                           true, stopOnRefExit, captureCoarse);
}

ReferenceTimerSample stepTimerReferenceSnapshot() {
    ReferenceTimerSample s{};
    portENTER_CRITICAL(&stepMux);
    s.position = referencePosition;
    s.coarse = referenceCoarse;
    s.entry = referenceEntry;
    s.exit = referenceExit;
    s.reason = referenceEnd;
    s.coarseSeen = referenceCoarseSeen;
    s.entrySeen = referenceEntrySeen;
    s.exitSeen = referenceExitSeen;
    portEXIT_CRITICAL(&stepMux);
    return s;
}

bool stepTimerTakeReferenceDone(ReferenceTimerSample& sample) {
    portENTER_CRITICAL(&stepMux);
    const bool done = isFinished && referenceMode;
    if (done) {
        sample.position = referencePosition;
        sample.coarse = referenceCoarse;
        sample.entry = referenceEntry;
        sample.exit = referenceExit;
        sample.reason = referenceEnd;
        sample.coarseSeen = referenceCoarseSeen;
        sample.entrySeen = referenceEntrySeen;
        sample.exitSeen = referenceExitSeen;
        isFinished = false;
        referenceMode = false;
    }
    portEXIT_CRITICAL(&stepMux);
    if (done) haltAlarm();
    return done;
}

void stepTimerStop() {
    haltAlarm();
    portENTER_CRITICAL(&stepMux);
    isRunning = false;
    stepIsHigh = false;
    isFinished = false;
    endstopTriggered = false;
    profileActive = false;
    referenceMode = false;
    portEXIT_CRITICAL(&stepMux);
    digitalWrite(MOTOR_PWM_PIN, LOW);
}

bool stepTimerRunning() {
    portENTER_CRITICAL(&stepMux);
    const bool result = isRunning;
    portEXIT_CRITICAL(&stepMux);
    return result;
}

uint32_t stepTimerCompletedSteps() {
    portENTER_CRITICAL(&stepMux);
    const uint32_t result = completed;
    portEXIT_CRITICAL(&stepMux);
    return result;
}

bool stepTimerTakeDone(uint32_t& completedSteps, bool& endstopHit) {
    portENTER_CRITICAL(&stepMux);
    const bool done = isFinished;
    if (done) {
        completedSteps = completed;
        endstopHit = endstopTriggered;
        isFinished = false;
        endstopTriggered = false;
        profileActive = false;
    }
    portEXIT_CRITICAL(&stepMux);
    if (done) haltAlarm();
    return done;
}

} // namespace fy
