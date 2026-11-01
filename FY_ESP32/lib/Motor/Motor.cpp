#include "Motor.h"
#include "StepTimer.h"
#include "FY_BenchPosition.h"
#include "HW_Config.h"
#include "Error.h"
#include "NVM.h"
#include <freertos/task.h>
#include <atomic>

#ifndef FY_STATUS_LED_PIN
#define FY_STATUS_LED_PIN 2
#endif
#ifndef FY_STATUS_LED_ACTIVE_HIGH
#define FY_STATUS_LED_ACTIVE_HIGH 1
#endif

namespace fy {

// Sechs Segmentlaengen im Motor vorbereiten; ISR/StepTimer bleiben unveraendert.
// Die vier Rampenwege sind Obergrenzen, keine vorgegebene Symmetrie.
bool Motor::calcProfile(uint16_t distance)
{
    for (uint8_t i = 0U; i < 6U; ++i) {
        Motor_profil[i].Steps = 0U;
        Motor_profil[i].Accel = 0;
    }
    if (distance == 0U) {
        _TimerValid = false;
        return false;
    }

    const uint32_t reserved =
        static_cast<uint32_t>(_ProfileParam.PosiMin) + _ProfileParam.KonstMin;
    if (distance <= _ProfileParam.PosiMin) {
        // Reine Positionierfahrt: der Timer startet direkt mit V_POS.
        Motor_profil[static_cast<uint8_t>(MotorProfile_t::POSI)].Steps = distance;
        _TimerValid = true;
        return true;
    }
    if (distance <= reserved) {
        // Kurzer Zwischenbereich: V_MIN fuer das Losbrechen, dann
        // ausnahmsweise ein Geschwindigkeitswechsel auf V_POS.
        // Die bestehende ISR setzt den Reload-Sprung nach einem BRE2-Step um.
        const uint32_t vMin = _ProfileParam.VMin;
        const uint32_t vPos = _ProfileParam.VPos;
        if (vMin == 0U || vPos == 0U) {
            _TimerValid = false;
            return false;
        }
        const uint32_t divMin = 512UL * vMin;
        const uint32_t divPos = 512UL * vPos;
        const int32_t reloadMin =
            static_cast<int32_t>((16000000UL + divMin / 2UL) / divMin) - 1;
        const int32_t reloadPos =
            static_cast<int32_t>((16000000UL + divPos / 2UL) / divPos) - 1;
        Motor_profil[static_cast<uint8_t>(MotorProfile_t::KONST)].Steps =
            static_cast<uint16_t>(distance - _ProfileParam.PosiMin - 1U);
        Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE2)].Steps = 1U;
        Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE2)].Accel =
            static_cast<int16_t>(reloadPos - reloadMin);
        Motor_profil[static_cast<uint8_t>(MotorProfile_t::POSI)].Steps =
            _ProfileParam.PosiMin;
        _TimerValid = true;
        return true;
    }

    const uint8_t a1 = static_cast<uint8_t>(MotorProfile_t::ACC1);
    const uint8_t a2 = static_cast<uint8_t>(MotorProfile_t::ACC2);
    const uint8_t ko = static_cast<uint8_t>(MotorProfile_t::KONST);
    const uint8_t b1 = static_cast<uint8_t>(MotorProfile_t::BRE1);
    const uint8_t b2 = static_cast<uint8_t>(MotorProfile_t::BRE2);
    const uint8_t po = static_cast<uint8_t>(MotorProfile_t::POSI);
    Motor_profil[po].Steps = _ProfileParam.PosiMin;
    Motor_profil[ko].Steps = _ProfileParam.KonstMin;
    Motor_profil[a1].Accel = -static_cast<int16_t>(_ProfileParam.Acc1Accel);
    Motor_profil[a2].Accel = -static_cast<int16_t>(_ProfileParam.Acc2Accel);
    Motor_profil[b1].Accel = static_cast<int16_t>(_ProfileParam.Bre1Accel);
    Motor_profil[b2].Accel = static_cast<int16_t>(_ProfileParam.Bre2Accel);

    uint32_t remaining = static_cast<uint32_t>(distance) - reserved;
    // Aeussere Phasen zuerst, dann innere. Kein Zwang A1==B2 / A2==B1.
    // Bei ungleichen Maximalstrecken wird das aktuelle Budget proportional
    // zwischen Anfahr- und Bremsweg geteilt.
    const uint32_t outerA = _ProfileParam.Acc1Steps;
    const uint32_t outerB = _ProfileParam.Bre2Steps;
    const uint32_t outerSum = outerA + outerB;
    const uint32_t outerBudget = remaining < outerSum ? remaining : outerSum;
    uint32_t usedA = outerSum == 0U ? 0U : (outerBudget * outerA) / outerSum;
    uint32_t usedB = outerBudget - usedA;
    // Maximalgrenzen auch bei ganzzahliger Rundung einhalten.
    if (usedB > outerB) {
        usedB = outerB;
        usedA = outerBudget - usedB;
    }
    Motor_profil[a1].Steps = static_cast<uint16_t>(usedA);
    Motor_profil[b2].Steps = static_cast<uint16_t>(usedB);
    remaining -= outerBudget;

    const uint32_t innerA = _ProfileParam.Acc2Steps;
    const uint32_t innerB = _ProfileParam.Bre1Steps;
    const uint32_t innerSum = innerA + innerB;
    const uint32_t innerBudget = remaining < innerSum ? remaining : innerSum;
    usedA = innerSum == 0U ? 0U : (innerBudget * innerA) / innerSum;
    usedB = innerBudget - usedA;
    if (usedB > innerB) {
        usedB = innerB;
        usedA = innerBudget - usedB;
    }
    Motor_profil[a2].Steps = static_cast<uint16_t>(usedA);
    Motor_profil[b1].Steps = static_cast<uint16_t>(usedB);
    remaining -= innerBudget;

    // Restlicher Weg wird bei der erreichten Geschwindigkeit gefahren.
    Motor_profil[ko].Steps = static_cast<uint16_t>(
        static_cast<uint32_t>(Motor_profil[ko].Steps) + remaining);
    uint32_t sum = 0U;
    for (uint8_t i = 0U; i < 6U; ++i) sum += Motor_profil[i].Steps;
    _TimerValid = (sum == distance);
    return _TimerValid;
}

// Motorzustandswechsel erfolgen nur in Muninn/Core 1. Die Task wertet
// ISR-Zustaende aus; es werden keine ungefragten UART-Telegramme erzeugt.
void Motor::onGoEnabled() {
    if (!_StopActive && _State == MotorState_t::STOPPED)
        _State = MotorState_t::IDLE;
}
void Motor::onStop(bool latched) {
    _StopActive = latched;
    _ReferenceValid = false;
    _State = MotorState_t::STOPPED;
    // STOP und GO AUS duerfen keine neue Referenzposition behaupten.
    invalidateBenchPosition();
}
void Motor::onMoveStarted() {
    if (!_StopActive && _State == MotorState_t::IDLE)
        _State = MotorState_t::MOVING;
}
void Motor::onMoveFinished() {
    if (_State == MotorState_t::MOVING)
        _State = MotorState_t::IDLE;
}
void Motor::onProfileError() {
    _ReferenceValid = false;
    invalidateBenchPosition();
    _State = MotorState_t::ERROR;
}
void Motor::onReferenceStarted() {
    _ReferenceValid = false;
    invalidateBenchPosition();
    _State = MotorState_t::REFRUN;
}
void Motor::onReferenceFinished(bool success) {
    _ReferenceValid = success;
    _State = success ? MotorState_t::IDLE : MotorState_t::ERROR;
}
bool Motor::referenceBegin(bool full) {
        if (_active || (!full && !_calibrated)) return false;
        _full = full;
        _stage = RefRunState::SEEK_MOTOR;
        _active = true;
        _position = 0U; // Before finding motor-side endstop, coordinate unknown.
        stepTimerReferenceSetPosition(0U);
        if (!referenceLaunch(false, 160U, false, false)) {
            referenceFail();
            return false;
        }
        return true;
    }

void Motor::referenceCancel() {
        if (_active) stepTimerStop();
        _active = false;
        _stage = RefRunState::IDLE;
        _calibrationPending = false; // Abgebrochene BG3-Fahrt nicht nachtraeglich persistieren.
        // Die Kalibrierung bleibt im Gegensatz zur aktuellen Referenz bei STOP erhalten.
    }

void Motor::referenceUpdate() {
        if (!_active) return;
        ReferenceTimerSample s{};
        if (!stepTimerTakeReferenceDone(s)) return;
        _position = s.position;
        switch (_stage) {
        case RefRunState::SEEK_MOTOR:
            if (s.reason != ReferenceEnd::ENDSTOP) return referenceFail();
            // Die Nano-Kurzreferenz setzt diese Position an LSL.
            _position = 1000U;
            stepTimerReferenceSetPosition(_position);
            if (!_full) return referenceFinish();
            _stage = RefRunState::SEEK_OPPOSITE;
            if (!referenceLaunch(true, 300U, false, true)) referenceFail();
            return;
        case RefRunState::SEEK_OPPOSITE:
            if (s.reason != ReferenceEnd::ENDSTOP)
                return referenceFail();
            _oppositeEnd = s.position;
            _coarse = s.coarse;
            // Nano-Anfahrt mit Grobmesswert + 1200 und Abstand zur rechten Endlage.
            {
                uint32_t target = _coarse + 1200U;
                if (target >= _oppositeEnd)
                    target = _oppositeEnd > 100U ? _oppositeEnd - 100U : 0U;
                if (target >= _position) return referenceFail();
                _stage = RefRunState::APPROACH;
                if (!referenceLaunch(false, 200U, false, false,
                            static_cast<uint16_t>(_position - target))) referenceFail();
            }
            return;
        case RefRunState::APPROACH:
            if (s.reason != ReferenceEnd::STEP_LIMIT) return referenceFail();
            _stage = RefRunState::FIRST_PASS;
            if (!referenceLaunch(false, 80U, true, false)) referenceFail();
            return;
        case RefRunState::FIRST_PASS:
            if (s.reason != ReferenceEnd::REF_EXIT)
                return referenceFail();
            _refRight = s.entry;
            _refLeft = s.exit;
            {
                const uint32_t target = _refLeft > 500U ? _refLeft - 500U : 0U;
                if (target >= _position || _position - target > UINT16_MAX)
                    return referenceFail();
                _stage = RefRunState::BACKOFF;
                if (!referenceLaunch(false, 80U, false, false,
                            static_cast<uint16_t>(_position - target))) referenceFail();
            }
            return;
        case RefRunState::BACKOFF:
            if (s.reason != ReferenceEnd::STEP_LIMIT) return referenceFail();
            _stage = RefRunState::SECOND_PASS;
            if (!referenceLaunch(true, 80U, true, false)) referenceFail();
            return;
        case RefRunState::SECOND_PASS:
            if (s.reason != ReferenceEnd::REF_EXIT)
                return referenceFail();
            _refLeftReverse = s.entry;
            _refRightReverse = s.exit;
            // Je zwei Messungen derselben raeumlichen Flanke.
            if (!(_refLeft < _refRight && _refLeftReverse < _refRightReverse &&
                  _refLeft > 1000U && _refRight < _oppositeEnd &&
                  _refLeftReverse > 1000U && _refRightReverse < _oppositeEnd))
                return referenceFail();
            _measuredCalibration[0] = _oppositeEnd;
            _measuredCalibration[1] = (_refLeft + _refLeftReverse) / 2U;
            _measuredCalibration[2] = (_refRight + _refRightReverse) / 2U;
            _calibrated = true;
            _calibrationPending = true; // Erst nach BG3-Stillstand NVM schreiben.
            return referenceFinish();
        default:
            return referenceFail();
        }
    }

bool Motor::referenceLaunch(bool away, uint16_t hz, bool stopAtExit, bool coarse,
                uint16_t steps) {
        return stepTimerStartReference(away, hz, stopAtExit, coarse, steps);
    }

void Motor::referenceFinish() { _active = false; _stage = RefRunState::DONE; }

void Motor::referenceFail() {
    // Eine Diagnosezeile nur im Fehlerfall; keine Ausgabe in der STEP-ISR.
    Serial.print(F("REF: ERROR phase="));
    Serial.print(static_cast<uint8_t>(_stage));
    Serial.print(F(" pos="));
    Serial.println(_position);
    stepTimerStop();
    _active = false;
    _stage = RefRunState::ERROR;
}

MotorCommandResult_t Motor::prepareReference(uint8_t mode) {
    if (mode > 1U) return MotorCommandResult_t::INVALID_DATA;
    if (_StopActive || _State != MotorState_t::IDLE)
        return MotorCommandResult_t::STOP_ACTIVE;
    return MotorCommandResult_t::ACCEPTED;
}
void Motor::invalidateBenchPosition() {
    _BenchReferenceValid = false;
    _BenchPositionValid = false;
    _MotionPositionTracked = false;
    _MotionStepCount = 0U;
}
void Motor::resetBenchPosition() { invalidateBenchPosition(); }

MotorCommandResult_t Motor::prepareSetTrack(uint8_t track, MoveRequest& request) {
    uint32_t destination = 0U;
    if (!trackPosition(track, destination))
        return MotorCommandResult_t::INVALID_DATA;
    if (_StopActive || _State != MotorState_t::IDLE)
        return MotorCommandResult_t::STOP_ACTIVE;
    if (_ReferenceValid) {
        // Die reale REF ist massgeblich, nicht die simulierte Testreferenz.
        // Die ISR fuehrt die absolute Position waehrend der Profilfahrt.
        const uint32_t current = stepTimerReferenceSnapshot().position;
        if (current <= 1000U || current >= _oppositeEnd ||
            destination <= 1000U || destination >= _oppositeEnd)
            return MotorCommandResult_t::STOP_ACTIVE;
        const uint32_t distance = current >= destination
            ? current - destination : destination - current;
        if (distance > UINT16_MAX)
            return MotorCommandResult_t::INVALID_DATA;
        request.mode = FY_MOTOR_MICROSTEP_MODE;
        request.directionRight = destination > current;
        request.steps = static_cast<uint16_t>(distance);
        if (request.steps > 0U &&
            (!calcProfile(request.steps) || !isProfileValid(request.steps)))
            return MotorCommandResult_t::INVALID_DATA;
        return MotorCommandResult_t::ACCEPTED;
    }
    // Eine simulierte Referenz darf SET_TRACK nicht freigeben.
    return MotorCommandResult_t::STOP_ACTIVE;
}

MotorCommandResult_t Motor::prepareSetPosition(uint16_t target,
                                                MoveRequest& request) {
    if (_StopActive || _State != MotorState_t::IDLE)
        return MotorCommandResult_t::STOP_ACTIVE;
    if (!_ReferenceValid || target <= 1000U || target >= _oppositeEnd)
        return MotorCommandResult_t::INVALID_DATA;
    const uint32_t current = stepTimerReferenceSnapshot().position;
    if (current <= 1000U || current >= _oppositeEnd)
        return MotorCommandResult_t::STOP_ACTIVE;
    const uint32_t delta = target > current ? target - current : current - target;
    if (delta > UINT16_MAX) return MotorCommandResult_t::INVALID_DATA;
    request.mode = FY_MOTOR_MICROSTEP_MODE;
    request.directionRight = target > current;
    request.steps = static_cast<uint16_t>(delta);
    if (request.steps != 0U &&
        (!calcProfile(request.steps) || !isProfileValid(request.steps)))
        return MotorCommandResult_t::INVALID_DATA;
    return MotorCommandResult_t::ACCEPTED;
}

void Motor::onMotionAccepted(const MoveRequest&) {}
void Motor::onMotionProgress(uint32_t) {}
bool Motor::isProfileValid(uint16_t distance) const {
    if (!_TimerValid || distance == 0U) return false;
    uint32_t sum = 0U;
    for (const auto& phase : Motor_profil) sum += phase.Steps;
    return sum == distance;
}

// Nur Nutzdatenpruefung; Laenge und CRC prueft Huginn.
bool decodeMovePayload(const uint8_t* data, MoveRequest& result) {
    if (data == nullptr) return false;
    const uint8_t flags = data[0];
    const uint8_t mode = static_cast<uint8_t>(flags >> 4U);
    if ((flags & 0x0EU) != 0U ||
        mode > static_cast<uint8_t>(MoveMicrostep::SIXTEENTH))
        return false;

    const uint16_t steps = static_cast<uint16_t>(
        static_cast<uint16_t>(data[1]) |
        (static_cast<uint16_t>(data[2]) << 8U));
    if (steps == 0U || steps > FY_MOVE_MAX_UNREFERENCED_STEPS)
        return false;

    result.directionRight = (flags & 0x01U) != 0U;
    result.mode = static_cast<MoveMicrostep>(mode);
    result.steps = steps;
    return true;
}


// Parameterpruefung im Motor-Vorbereitungspfad, nicht in der ISR.
MotorCommandResult_t Motor::prepareMove(const uint8_t* data,
                                        MoveRequest& request) {
    if (!decodeMovePayload(data, request))
        return MotorCommandResult_t::INVALID_DATA;
    // Treiber-MS-Pins bleiben nach der Initialisierung unveraendert.
    if (request.mode != FY_MOTOR_MICROSTEP_MODE)
        return MotorCommandResult_t::INVALID_DATA;
    if (_ReferenceValid) {
        const uint32_t current = stepTimerReferenceSnapshot().position;
        if (current <= 1000U || current >= _oppositeEnd)
            return MotorCommandResult_t::STOP_ACTIVE;
        const int32_t end = static_cast<int32_t>(current) +
            (request.directionRight ? static_cast<int32_t>(request.steps)
                                    : -static_cast<int32_t>(request.steps));
        if (end <= 1000 || end >= static_cast<int32_t>(_oppositeEnd))
            return MotorCommandResult_t::INVALID_DATA;
    }
    // Alle sechs Nano-Phasen vorbereiten; die STEP-ISR fuehrt sie aus.
    // Dynamische Timerintervalle erfordern weiterhin einen Hardwaretest.
    if (!calcProfile(request.steps) || !isProfileValid(request.steps))
        return MotorCommandResult_t::INVALID_DATA;
    return MotorCommandResult_t::ACCEPTED;
}

namespace {
// Nur Muninn schreibt; Huginn liest atomare C++-Snapshots.
std::atomic<bool> driverEnabled{false};
std::atomic<bool> motorMoving{false};
std::atomic<uint8_t> motorStateSnapshot{static_cast<uint8_t>(MotorState_t::STOPPED)};
// Bit 16 kennzeichnet eine simulierte Testposition; 0x7FFF allein
// behaelt die Nano-Bedeutung POSITION_UNKNOWN.
constexpr uint32_t kPositionUnknown = 0x00007FFFU;
constexpr uint32_t kPositionBenchFlag = 0x00010000U;
std::atomic<uint32_t> positionSnapshot{kPositionUnknown};
std::atomic<uint32_t> realReferencePosition{kPositionUnknown};
std::atomic<uint8_t> referenceStatusSnapshot{0U}; // 0 none, 1 real, 2 fake
std::atomic<uint16_t> nvmStatusSnapshot{0U}; // high byte valid mask, low byte fault mask
std::atomic<uint32_t> nvmTrackValues[5]{};
std::atomic<uint32_t> nvmRampValues[17]{};
std::atomic<uint32_t> nvmCalibrationValues[3]{};
std::atomic<uint32_t> trackSnapshot{
    static_cast<uint32_t>(TRACK_STATUS_ERROR) << 16U
}; // target, confirmed, status encoded atomically

void publishPosition(const Motor& motor) {
    if (motor.referenced()) {
        positionSnapshot.store(realReferencePosition.load(std::memory_order_acquire),
                               std::memory_order_release);
        return;
    }
    (void)motor;
    positionSnapshot.store(kPositionUnknown, std::memory_order_release);
}

void publishReference(const Motor& motor) {
    const uint8_t status = motor.referenced() ? 1U : 0U;
    referenceStatusSnapshot.store(status, std::memory_order_release);
}

void publishTrack(uint8_t target, uint8_t confirmed, uint8_t status) {
    const uint32_t packed = static_cast<uint32_t>(target) |
        (static_cast<uint32_t>(confirmed) << 8U) |
        (static_cast<uint32_t>(status) << 16U);
    trackSnapshot.store(packed, std::memory_order_release);
}

// Das sechsstufige Profil steuert die STEP-Frequenz in der Timer-ISR.

void applyMicrostepMode(MoveMicrostep mode) {
    // A4988-Belegung MS1/MS2/MS3; nur bei stillstehendem STEP setzen.
    // Moduswechsel koennen die absolute Position entwerten; nur Testbetrieb.
    constexpr uint8_t masks[] = {0x00U, 0x01U, 0x02U, 0x03U, 0x07U};
    const uint8_t value = masks[static_cast<uint8_t>(mode)];
    digitalWrite(MOTOR_MS0_PIN, (value & 0x01U) ? HIGH : LOW);
    digitalWrite(MOTOR_MS1_PIN, (value & 0x02U) ? HIGH : LOW);
    digitalWrite(MOTOR_MS2_PIN, (value & 0x04U) ? HIGH : LOW);
}

void setLed(bool on) {
#if FY_STATUS_LED_PIN >= 0
    digitalWrite(FY_STATUS_LED_PIN,
                 (on == (FY_STATUS_LED_ACTIVE_HIGH != 0)) ? HIGH : LOW);
#else
    (void)on;
#endif
}

uint8_t nackData(NackReason reason) {
    return static_cast<uint8_t>(
        FY_ERROR_LOCATION_MOTOR | static_cast<uint8_t>(reason));
}

void emit(BringupContext* context, const CommandMessage& cmd,
          bool accepted, uint8_t data) {
    const MotorEvent event{
        cmd.sequence, cmd.id,
        static_cast<uint8_t>(xPortGetCoreID()),
        accepted ? STATUS_ACK : STATUS_NACK,
        data
    };
    const bool local = (cmd.sequence & 0x80000000UL) != 0UL;
    QueueHandle_t destination = local ? context->keyboardEvents : context->events;
    // Integrationspruefung UART/Motor:
    // portMAX_DELAY verhindert den stillen Verlust einer Antwort bei voller
    // Event-Queue, kann aber die gesamte Muninn-Task blockieren.
    // Solange emit() wartet, wird auch die STOP-Prioritaetsqueue nicht gelesen.
    // UART gibt _pending-Eintraege nur nach passender ACK/NACK-Antwort frei;
    // ein Timeout fehlt. Queue-Groessen allein loesen dieses Risiko nicht.
    if (destination != nullptr) (void)xQueueSend(destination, &event, portMAX_DELAY);
}
} // anonymous namespace

bool Muninn_isDriverEnabled() {
    return driverEnabled.load(std::memory_order_acquire);
}

bool Muninn_isMoving() {
    return motorMoving.load(std::memory_order_acquire);
}

uint8_t Muninn_motionDirectionSnapshot() {
    if (!motorMoving.load(std::memory_order_acquire) || !stepTimerRunning())
        return 0U;
    // HW3: DIR HIGH fährt vom Motor weg (= physisch links/G39).
    // DIR LOW fährt zur Motorseite (= physisch rechts/G36).
    return digitalRead(MOTOR_DIR_PIN) == HIGH ? 1U : 2U;
}

uint32_t Muninn_positionSnapshot() {
    return positionSnapshot.load(std::memory_order_acquire);
}

uint8_t Muninn_referenceStatusSnapshot() {
    return referenceStatusSnapshot.load(std::memory_order_acquire);
}

uint32_t Muninn_trackSnapshot() {
    return trackSnapshot.load(std::memory_order_acquire);
}

bool Muninn_getParamSnapshot(uint8_t id, uint32_t& value) {
    const uint16_t status = nvmStatusSnapshot.load(std::memory_order_acquire);
    const uint8_t valid = static_cast<uint8_t>(status >> 8U);
    if (id >= static_cast<uint8_t>(ParamId::POS_LSR) &&
        id <= static_cast<uint8_t>(ParamId::POS_REF_B) && (valid & 0x01U) != 0U) {
        value = nvmCalibrationValues[id - static_cast<uint8_t>(ParamId::POS_LSR)]
            .load(std::memory_order_acquire);
        return value <= 0xFFFFFFU;
    }
    if (id >= static_cast<uint8_t>(ParamId::TRACK_BG1) &&
        id <= static_cast<uint8_t>(ParamId::TRACK_BG5) && (valid & 0x02U) != 0U) {
        value = nvmTrackValues[id - static_cast<uint8_t>(ParamId::TRACK_BG1)]
            .load(std::memory_order_acquire);
        return value <= 0xFFFFFFU;
    }
    if ((valid & 0x04U) != 0U) {
        uint8_t idx=0U;
        if(id>=static_cast<uint8_t>(ParamId::ACC1)&&id<=static_cast<uint8_t>(ParamId::B2_ST))
            idx=static_cast<uint8_t>(id-static_cast<uint8_t>(ParamId::ACC1));
        else if(id>=static_cast<uint8_t>(ParamId::V_MIN)&&id<=static_cast<uint8_t>(ParamId::V_POS))
            idx=static_cast<uint8_t>(8U+id-static_cast<uint8_t>(ParamId::V_MIN));
        else if(id>=static_cast<uint8_t>(ParamId::A1_LEN)&&id<=static_cast<uint8_t>(ParamId::KONST_MIN))
            idx=static_cast<uint8_t>(11U+id-static_cast<uint8_t>(ParamId::A1_LEN));
        else return false;
        value=nvmRampValues[idx].load(std::memory_order_acquire);
        return true;
    }
    return false; // Unangebundene IDs liefern keinen Scheinwert.
}

uint16_t Muninn_nvmStatusSnapshot() {
    return nvmStatusSnapshot.load(std::memory_order_acquire);
}

MotorState_t Muninn_motorState() {
    return static_cast<MotorState_t>(motorStateSnapshot.load(std::memory_order_acquire));
}

namespace {
// Die persistenten Payloads sind explizit in Little-Endian kodiert.
uint32_t decodeLe32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
        (static_cast<uint32_t>(p[1]) << 8U) |
        (static_cast<uint32_t>(p[2]) << 16U) |
        (static_cast<uint32_t>(p[3]) << 24U);
}
void encodeLe32(uint8_t* p, uint32_t value) {
    p[0] = static_cast<uint8_t>(value);
    p[1] = static_cast<uint8_t>(value >> 8U);
    p[2] = static_cast<uint8_t>(value >> 16U);
    p[3] = static_cast<uint8_t>(value >> 24U);
}
bool calibrationPlausible(const uint32_t values[3]) {
    return values[0] > 1000U && values[0] <= 0xFFFFFFU &&
           values[1] > 1000U && values[1] < values[2] &&
           values[2] < values[0];
}
void publishNvmSnapshot(const Motor& motor, uint8_t faults,
                        bool calibValid, bool tracksValid) {
    uint8_t valid = 0U;
    if (calibValid) valid |= 0x01U;
    if (tracksValid) valid |= 0x02U;
    if (motor.rampParametersValid()) valid |= 0x04U;
    nvmStatusSnapshot.store(
        static_cast<uint16_t>(faults) |
        (static_cast<uint16_t>(valid) << 8U),
        std::memory_order_release);
}
} // namespace

namespace {
constexpr uint16_t kRampDefaults[17]{2,1,1,2,1,2,2,1,120,800,120,100,100,100,100,160,400};
void getRamp(const MotorProfileParam_t& p,uint16_t (&v)[17]) {
    v[0]=p.Acc1Accel;v[1]=p.Acc2Accel;v[2]=p.Bre1Accel;v[3]=p.Bre2Accel;
    v[4]=p.Acc1RampInterval;v[5]=p.Acc2RampInterval;v[6]=p.Bre1RampInterval;v[7]=p.Bre2RampInterval;
    v[8]=p.VMin;v[9]=p.VMax;v[10]=p.VPos;
    v[11]=p.Acc1Steps;v[12]=p.Acc2Steps;v[13]=p.Bre1Steps;v[14]=p.Bre2Steps;
    v[15]=p.PosiMin;v[16]=p.KonstMin;
}
void putRamp(MotorProfileParam_t& p,const uint16_t (&v)[17]) {
    p.Acc1Accel=v[0];p.Acc2Accel=v[1];p.Bre1Accel=v[2];p.Bre2Accel=v[3];
    p.Acc1RampInterval=v[4];p.Acc2RampInterval=v[5];p.Bre1RampInterval=v[6];p.Bre2RampInterval=v[7];
    p.VMin=v[8];p.VMax=v[9];p.VPos=v[10];
    p.Acc1Steps=v[11];p.Acc2Steps=v[12];p.Bre1Steps=v[13];p.Bre2Steps=v[14];
    p.PosiMin=v[15];p.KonstMin=v[16];
}
bool rampValid(const uint16_t (&v)[17]) {
    for(uint8_t i=0;i<8;++i)if(v[i]==0U||v[i]>(i<4U?64U:128U))return false;
    return v[8]>=40U&&v[8]<=800U&&v[9]>=40U&&v[9]<=800U&&
           v[10]>=40U&&v[10]<=800U&&v[8]<=v[9]&&v[10]<=v[9];
}
void publishRamp(const uint16_t (&v)[17]) {
    for(uint8_t i=0;i<17;++i)nvmRampValues[i].store(v[i],std::memory_order_release);
}
void packRamp(const uint16_t (&v)[17],uint8_t (&raw)[34]) {
    for(uint8_t i=0;i<17;++i){raw[2*i]=static_cast<uint8_t>(v[i]);raw[2*i+1]=static_cast<uint8_t>(v[i]>>8U);}
}
void unpackRamp(const uint8_t (&raw)[34],uint16_t (&v)[17]) {
    for(uint8_t i=0;i<17;++i)v[i]=static_cast<uint16_t>(raw[2*i]|(static_cast<uint16_t>(raw[2*i+1])<<8U));
}
bool paramIndex(uint8_t id,uint8_t& idx) {
    if(id>=static_cast<uint8_t>(ParamId::ACC1)&&id<=static_cast<uint8_t>(ParamId::B2_ST))
        idx=static_cast<uint8_t>(id-static_cast<uint8_t>(ParamId::ACC1));
    else if(id>=static_cast<uint8_t>(ParamId::V_MIN)&&id<=static_cast<uint8_t>(ParamId::V_POS))
        idx=static_cast<uint8_t>(8U+id-static_cast<uint8_t>(ParamId::V_MIN));
    else if(id>=static_cast<uint8_t>(ParamId::A1_LEN)&&id<=static_cast<uint8_t>(ParamId::KONST_MIN))
        idx=static_cast<uint8_t>(11U+id-static_cast<uint8_t>(ParamId::A1_LEN));
    else return false;
    return true;
}
} // namespace
bool Motor::getRampParameter(uint8_t id,uint32_t& value) const {
    uint8_t idx=0;if(!_rampParametersValid||!paramIndex(id,idx))return false;
    uint16_t v[17]{};getRamp(_ProfileParam,v);value=v[idx];return true;
}
bool Motor::setRampParameter(uint8_t id,uint32_t value) {
    uint8_t idx=0;if(!_rampParametersValid||!paramIndex(id,idx)||value>UINT16_MAX)return false;
    uint16_t v[17]{};getRamp(_ProfileParam,v);v[idx]=static_cast<uint16_t>(value);
    if(!rampValid(v))return false;
    putRamp(_ProfileParam,v);publishRamp(v);return true;
}
bool Motor::saveRampParameters(bool& changed) {
    changed=false;if(!_rampParametersValid)return false;
    uint16_t v[17]{};getRamp(_ProfileParam,v);if(!rampValid(v))return false;
    uint8_t raw[34]{};packRamp(v,raw);
    const NVM nvm{};const NvmResult result=nvm.save(NvmRecord::MOTOR_PARAMETERS,raw);
    if(result!=NvmResult::OK&&result!=NvmResult::UNCHANGED){
        _nvmFaultMask|=0x04U;publishNvmSnapshot(*this,_nvmFaultMask,_storedCalibValid,_tracksValid);return false;
    }
    changed=(result==NvmResult::OK);_nvmFaultMask&=static_cast<uint8_t>(~0x04U);
    publishNvmSnapshot(*this,_nvmFaultMask,_storedCalibValid,_tracksValid);return true;
}
bool Motor::restoreRampDefaults(bool& changed) {
    uint16_t old[17]{};getRamp(_ProfileParam,old);changed=false;
    for(uint8_t i=0;i<17;++i)if(old[i]!=kRampDefaults[i])changed=true;
    uint8_t raw[34]{};packRamp(kRampDefaults,raw);
    const NVM nvm{};const NvmResult result=nvm.save(NvmRecord::MOTOR_PARAMETERS,raw);
    if(result!=NvmResult::OK&&result!=NvmResult::UNCHANGED)return false;
    putRamp(_ProfileParam,kRampDefaults);publishRamp(kRampDefaults);
    _rampParametersValid=true;_nvmFaultMask&=static_cast<uint8_t>(~0x04U);
    publishNvmSnapshot(*this,_nvmFaultMask,_storedCalibValid,_tracksValid);return true;
}
void Motor::initializeStorage() {
    const NVM nvm{};
    uint8_t rawCalib[12]{};
    const NvmResult cr = nvm.load(NvmRecord::CALIBRATION, rawCalib);
    if (cr == NvmResult::OK) {
        uint32_t c[3]{};
        for (uint8_t i = 0U; i < 3U; ++i)
            c[i] = decodeLe32(&rawCalib[4U * i]);
        if (calibrationPlausible(c)) {
            for (uint8_t i = 0U; i < 3U; ++i) {
                _savedCalibration[i] = c[i];
                nvmCalibrationValues[i].store(c[i], std::memory_order_release);
            }
            _oppositeEnd = c[0]; // Auch Kurz-REF nach Reboot braucht LSR!
            _calibrated = true;
            _storedCalibValid = true;
        } else {
            _nvmFaultMask |= 0x01U;
        }
    } else if (cr != NvmResult::NOT_FOUND) {
        _nvmFaultMask |= 0x01U; // CRC-/Format-/I/O-Fehler, keine Defaults.
    }

    const NvmResult tr = [&]() {
        uint8_t raw[20]{};
        const NvmResult read = nvm.load(NvmRecord::TRACK_POSITIONS, raw);
        if (read == NvmResult::NOT_FOUND) {
            for (uint8_t i = 0U; i < FY_BENCH_TRACK_COUNT; ++i)
                encodeLe32(&raw[4U * i], FY_BENCH_TRACK_POSITIONS[i]);
            const NvmResult first = nvm.save(NvmRecord::TRACK_POSITIONS, raw);
            if (first != NvmResult::OK && first != NvmResult::UNCHANGED)
                return first;
        } else if (read != NvmResult::OK) {
            return read; // Niemals kaputten Record mit Defaults ueberschreiben.
        }
        for (uint8_t i = 0U; i < FY_BENCH_TRACK_COUNT; ++i) {
            const uint32_t v = decodeLe32(&raw[4U * i]);
            if (v <= 1000U || v >= 0xFFFFFFU)
                return NvmResult::INVALID_DATA;
            _tracks[i] = v;
        }
        return NvmResult::OK;
    }();
    if (tr == NvmResult::OK) {
        _tracksValid = true;
        for (uint8_t i = 0U; i < FY_BENCH_TRACK_COUNT; ++i)
            nvmTrackValues[i].store(_tracks[i], std::memory_order_release);
    } else {
        _nvmFaultMask |= 0x02U;
    }
    uint8_t rawRamp[34]{};
    NvmResult mr=nvm.load(NvmRecord::MOTOR_PARAMETERS,rawRamp);
    if(mr==NvmResult::NOT_FOUND) {
        packRamp(kRampDefaults,rawRamp);
        mr=nvm.save(NvmRecord::MOTOR_PARAMETERS,rawRamp);
    }
    if(mr==NvmResult::OK || mr==NvmResult::UNCHANGED) {
        uint16_t v[17]{}; unpackRamp(rawRamp,v);
        if(rampValid(v)) {
            putRamp(_ProfileParam,v);
            publishRamp(v);
            _rampParametersValid=true;
        } else _nvmFaultMask |= 0x04U;
    } else _nvmFaultMask |= 0x04U;
    publishNvmSnapshot(*this, _nvmFaultMask, _storedCalibValid, _tracksValid);
}

bool Motor::setTrackPosition(uint8_t track, uint32_t position) {
    if (!_tracksValid || track == 0U || track > FY_BENCH_TRACK_COUNT ||
        position <= 1000U || position >= 0xFFFFFFU)
        return false;
    // Bei gueltiger Kalibrierung muss das Ziel innerhalb der Endlagen liegen.
    if (_storedCalibValid && position >= _savedCalibration[0])
        return false;
    _tracks[track - 1U] = position;
    nvmTrackValues[track - 1U].store(position, std::memory_order_release);
    return true;
}

bool Motor::saveTrackPositions() {
    if (!_tracksValid) return false;
    uint8_t raw[20]{};
    for (uint8_t i = 0U; i < FY_BENCH_TRACK_COUNT; ++i)
        encodeLe32(&raw[4U * i], _tracks[i]);
    const NVM nvm{};
    const NvmResult result = nvm.save(NvmRecord::TRACK_POSITIONS, raw);
    if (result != NvmResult::OK && result != NvmResult::UNCHANGED) {
        _nvmFaultMask |= 0x02U;
        publishNvmSnapshot(*this, _nvmFaultMask, _storedCalibValid, false);
        return false;
    }
    _nvmFaultMask &= static_cast<uint8_t>(~0x02U);
    publishNvmSnapshot(*this, _nvmFaultMask, _storedCalibValid, true);
    return true;
}

bool Motor::restoreTrackDefaults() {
    // Defaults gehoeren dem Fachmodul; NVM bleibt ein reiner Byte-Speicherdienst.
    uint8_t raw[20]{};
    for (uint8_t i = 0U; i < FY_BENCH_TRACK_COUNT; ++i)
        encodeLe32(&raw[4U * i], FY_BENCH_TRACK_POSITIONS[i]);

    const NVM nvm{};
    const NvmResult result = nvm.save(NvmRecord::TRACK_POSITIONS, raw);
    if (result != NvmResult::OK && result != NvmResult::UNCHANGED)
        return false; // Aktive RAM-Gleise und Fault-Maske unveraendert lassen.

    for (uint8_t i = 0U; i < FY_BENCH_TRACK_COUNT; ++i) {
        _tracks[i] = FY_BENCH_TRACK_POSITIONS[i];
        nvmTrackValues[i].store(_tracks[i], std::memory_order_release);
    }
    _tracksValid = true;
    _nvmFaultMask &= static_cast<uint8_t>(~0x02U);
    publishNvmSnapshot(*this, _nvmFaultMask, _storedCalibValid, true);
    return true;
}

bool Motor::trackPosition(uint8_t track, uint32_t& out) const {
    if (!_tracksValid || track == 0U || track > FY_BENCH_TRACK_COUNT)
        return false;
    out = _tracks[track - 1U];
    return true;
}

bool Motor::persistCalibration() {
    if (!_calibrationPending) return true;
    if (!_calibrated || !calibrationPlausible(_measuredCalibration))
        return false;
    bool changed = !_storedCalibValid;
    for (uint8_t i = 0U; i < 3U && !changed; ++i) {
        const uint32_t a = _measuredCalibration[i];
        const uint32_t b = _savedCalibration[i];
        if ((a > b ? a - b : b - a) > 10U) changed = true;
    }
    if (changed) {
        uint8_t raw[12]{};
        for (uint8_t i = 0U; i < 3U; ++i)
            encodeLe32(&raw[4U * i], _measuredCalibration[i]);
        const NVM nvm{};
        const NvmResult result = nvm.save(NvmRecord::CALIBRATION, raw);
        if (result != NvmResult::OK && result != NvmResult::UNCHANGED) {
            _nvmFaultMask |= 0x01U;
            _storedCalibValid = false;
            publishNvmSnapshot(*this, _nvmFaultMask, false, _tracksValid);
            return false;
        }
    }
    for (uint8_t i = 0U; i < 3U; ++i) {
        // NVM-Vergleichsbasis bei <=10 Steps NICHT verschieben.
        if (changed) _savedCalibration[i] = _measuredCalibration[i];
        nvmCalibrationValues[i].store(_measuredCalibration[i],
                                     std::memory_order_release);
    }
    _nvmFaultMask &= static_cast<uint8_t>(~0x01U);
    _storedCalibValid = true;
    _calibrationPending = false;
    publishNvmSnapshot(*this, _nvmFaultMask, true, _tracksValid);
    return true;
}

void Muninn_initMotor() {
    // G32 (!SLEEP und !RESET) bleibt waehrend des Starts LOW.
    // Mikroschritt-Eingaenge werden bei HIGH an ENABLE konfiguriert.
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);
    digitalWrite(MOTOR_PWM_PIN, LOW);
    pinMode(MOTOR_MS0_PIN, OUTPUT);
    pinMode(MOTOR_MS1_PIN, OUTPUT);
    pinMode(MOTOR_MS2_PIN, OUTPUT);
    applyMicrostepMode(FY_MOTOR_MICROSTEP_MODE);
    // Timer nur nach Annahme eines begrenzten Fahrbefehls aktivieren.
    (void)stepTimerBegin();
#if FY_STATUS_LED_PIN >= 0
    pinMode(FY_STATUS_LED_PIN, OUTPUT);
    setLed(false);
#endif
    // HW3: Sleep/Reset dient ausschliesslich der Einschaltverriegelung.
    // A4988 erst bei definierten Steuerleitungen einmalig aufwecken.
    // UART- und Muninn-Tasks laufen hier noch nicht; die Wartezeit blockiert keine Befehle.
    digitalWrite(MOTOR_SLEEP_PIN, HIGH);
    delayMicroseconds(1500U); // A4988 tWAKE >= 1 ms, outside timer ISR.
}

namespace {
/**
 * @brief Liest die vier Sensor-Rohpegel für die Statusmeldung 40.
 * @return Gepackte Eingangspegel: Bit 0 G36, Bit 1 G39, Bit 2 G34, Bit 3 G35.
 * @note Aufruf ausschließlich im Muninn-Task, nicht in der STEP-ISR.
 *       G35 liefert nur Riemeninformation und beeinflusst die Fahrfreigabe nicht.
 *       Die vier Abfragen sind keine zeitgleiche Messung.
 */
uint8_t readSensorLevels() {
    return static_cast<uint8_t>(
        ((digitalRead(LSL_PIN) == HIGH) ? 0x01U : 0U) |
        ((digitalRead(LSR_PIN) == HIGH) ? 0x02U : 0U) |
        ((digitalRead(LSREF_PIN) == HIGH) ? 0x04U : 0U) |
        ((digitalRead(LS4_PIN) == HIGH) ? 0x08U : 0U));
}
} // anonymous namespace

void Muninn_motorTask(void* arg) {
    auto* context = static_cast<BringupContext*>(arg);
    CommandMessage cmd{};
    Motor motor{};
    motor.initializeStorage(); // Kein Motorschritt, POR-STOP bleibt gesetzt.
    bool realTrackProfileActive = false; // Real profiled motion: REF->BG3 / SET_TRACK
    bool ledState = false;
    // Nach POR/Reset bleibt STOP verriegelt; nur STOP AA 55 entriegelt.
    bool stopActive = true; // Mirrors Motor state for the existing STOP path.
    uint32_t expectedTargetPosition = 0U;
    uint8_t targetTrack = 0U; // 0 = unknown, only Muninn changes these.
    uint8_t confirmedTrack = 0U;

    // Sensoreingänge gehören Muninn; externe Pull-ups mit 10 kOhm.
    pinMode(LSL_PIN, INPUT);
    pinMode(LSR_PIN, INPUT);
    pinMode(LSREF_PIN, INPUT);
    pinMode(LS4_PIN, INPUT); // Riemenspannung: reine Statusinformation, kein Endschalter.
    uint8_t lastSensorLevels = readSensorLevels();
    uint8_t sensorChangeCount = 0U; // high nibble 0..E; F reserved
    uint8_t sensorMessage = lastSensorLevels;
    (void)xQueueOverwrite(context->sensorSnapshots, &sensorMessage); // initial snapshot

    // STOP bleibt nach POR verriegelt; Treiber ist wach, aber deaktiviert.
    // !SLEEP/!RESET (G32) bleibt bei GO/STOP HIGH.
    digitalWrite(MOTOR_PWM_PIN, LOW);
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);
    driverEnabled.store(false, std::memory_order_release);
    motorMoving.store(false, std::memory_order_release);
    // Beim Start keine simulierte Referenz; REFERENCE 00 nur auf ausdruecklichen Auftrag.
    publishPosition(motor);
    publishReference(motor);
    publishTrack(0U, 0U, TRACK_STATUS_ERROR);
    motorStateSnapshot.store(static_cast<uint8_t>(motor.state()), std::memory_order_release);

    for (;;) {
        // Nur den letzten Sensorzustand bereitstellen; STOP und Motorereignisse nicht blockieren.
        const uint8_t sensorLevels = readSensorLevels();
        if (sensorLevels != lastSensorLevels) {
            lastSensorLevels = sensorLevels;
            sensorChangeCount = (sensorChangeCount == 0x0EU)
                ? 0U : static_cast<uint8_t>(sensorChangeCount + 1U);
            sensorMessage = static_cast<uint8_t>(
                (sensorChangeCount << 4U) | sensorLevels);
            (void)xQueueOverwrite(context->sensorSnapshots, &sensorMessage);
        }
        // STOP hat eine eigene Prioritaetsqueue und wird vor normalen Auftraegen geprueft.
        // Integrationsrisiko: Ein blockierendes emit() verhindert diese Pruefung.
        if (xQueueReceive(context->priorityCommands, &cmd, 0) == pdTRUE) {
            const bool activate = cmd.data[0] == 0x55U && cmd.data[1] == 0xAAU;
            const bool release = cmd.data[0] == 0xAAU && cmd.data[1] == 0x55U;
            if (cmd.id != CMD_STOPP || (!activate && !release)) {
                emit(context, cmd, false,
                     nackData(NackReason::INVALID_PARAMETER));
                continue;
            }
            // Zuerst Treiber deaktivieren, danach STEP anhalten.
            // !SLEEP bleibt HIGH; STOP setzt den A4988-Translator nicht zurueck.
            // STOP und Freigabe setzen eine abgebrochene Fahrt nicht fort.
            digitalWrite(MOTOR_ENABLE_PIN, HIGH);
            driverEnabled.store(false, std::memory_order_release);
            motor.referenceCancel();
            realTrackProfileActive = false;
            stepTimerStop();
            motorMoving.store(false, std::memory_order_release);
            realReferencePosition.store(kPositionUnknown, std::memory_order_release);
            stopActive = activate;
            motor.onStop(activate);
            targetTrack = 0U;
            confirmedTrack = 0U;
            publishPosition(motor);
            publishReference(motor);
            publishTrack(0U, 0U, TRACK_STATUS_ERROR);
            motorStateSnapshot.store(static_cast<uint8_t>(motor.state()), std::memory_order_release);

            // Alte Fahrbefehle aus der normalen Queue entfernen und jeden
            // verworfenen Auftrag explizit negativ beantworten.
            // Integrationsrisiko: emit() kann bei voller Antwortqueue blockieren.
            CommandMessage pending{};
            while (xQueueReceive(context->commands, &pending, 0) == pdTRUE)
                emit(context, pending, false, nackData(NackReason::NOT_READY));

            emit(context, cmd, true, cmd.id);
            ledState = !ledState;
            setLed(ledState);
            continue;
        }

        // Physischer Nano-Referenzautomat; STOP wird vorher geprueft.
        if (motor.referenceActive()) {
            motor.referenceUpdate();
            if (!motor.referenceActive()) {
                const bool success = motor.referenceSuccessful();
                motor.onReferenceFinished(success);
                motorMoving.store(false, std::memory_order_release);
                realReferencePosition.store(
                    success ? motor.referencePosition() : kPositionUnknown,
                    std::memory_order_release);
                publishPosition(motor);
                publishReference(motor);
                motorStateSnapshot.store(static_cast<uint8_t>(motor.state()),
                                         std::memory_order_release);
                if (success) {
                    // Einmalige Diagnose beim Uebergang von Referenzierung zu BG3, ausserhalb der ISR.
                    Serial.print(F("REF: VALID pos="));
                    Serial.print(motor.referencePosition());
                    Serial.print(F(" end="));
                    Serial.println(motor.referenceOppositeEnd());
                    // Nano-Hardwaretest fuhr nach Kurzreferenz auf BG3.
                    // Dieses Ziel gilt auch nach erfolgreicher Langkalibrierung.
                    uint32_t kTrack3 = 0U;
                    const bool bg3Valid = motor.trackPosition(3U, kTrack3);
                    const uint32_t start = motor.referencePosition();
                    const uint32_t rightEnd = motor.referenceOppositeEnd();
                    const uint32_t distance = start >= kTrack3
                        ? start - kTrack3 : kTrack3 - start;
                    if (!bg3Valid || rightEnd <= kTrack3 || kTrack3 <= 1000U ||
                        distance > UINT16_MAX) {
                        Serial.print(F("REF: BG3 RANGE ERROR end="));
                        Serial.print(rightEnd);
                        Serial.print(F(" from="));
                        Serial.print(start);
                        Serial.print(F(" distance="));
                        Serial.println(distance);
                        motor.onProfileError();
                    } else if (distance == 0U) {
                        targetTrack = 3U;
                        confirmedTrack = 3U;
                        publishTrack(3U, 3U, TRACK_STATUS_REACHED);
                    } else if (motor.calcProfile(static_cast<uint16_t>(distance)) &&
                               motor.isProfileValid(static_cast<uint16_t>(distance))) {
                        StepProfileSegment stages[STEP_PROFILE_PHASES]{};
                        const auto& p = motor.profileParams();
                        const uint16_t intervals[STEP_PROFILE_PHASES]{
                            p.Acc1RampInterval, p.Acc2RampInterval, 1U,
                            p.Bre1RampInterval, p.Bre2RampInterval, 1U
                        };
                        for (uint8_t i = 0U; i < STEP_PROFILE_PHASES; ++i) {
                            stages[i].steps = motor.profile()[i].Steps;
                            stages[i].accel = motor.profile()[i].Accel;
                            stages[i].rampEvery = intervals[i];
                            if (i == static_cast<uint8_t>(MotorProfile_t::BRE2) &&
                                distance > p.PosiMin &&
                                distance <= static_cast<uint32_t>(p.PosiMin) + p.KonstMin)
                                stages[i].rampEvery = 1U;
                        }
                        if (stepTimerConfigureProfile(p.VMin,p.VMax,p.VPos) &&
                            stepTimerStartReferenceProfile(
                                static_cast<uint16_t>(distance),
                                kTrack3 > start, stages,
                                distance <= p.PosiMin ? p.VPos : p.VMin)) {
                            realTrackProfileActive = true;
                            expectedTargetPosition = kTrack3;
                            targetTrack = 3U;
                            confirmedTrack = 0U;
                            motorMoving.store(true, std::memory_order_release);
                            motor.onMoveStarted();
                            publishTrack(3U, 0U, TRACK_STATUS_MOVING);
                        } else {
                            Serial.println(F("REF: BG3 TIMER START FAILED"));
                            motor.onProfileError();
                        }
                    } else {
                        Serial.print(F("REF: BG3 PROFILE INVALID distance="));
                        Serial.println(distance);
                        motor.onProfileError();
                    }
                    if (motor.state() == MotorState_t::ERROR) {
                        realReferencePosition.store(kPositionUnknown,
                                                    std::memory_order_release);
                        motorMoving.store(false, std::memory_order_release);
                        publishTrack(0U, 0U, TRACK_STATUS_ERROR);
                        publishPosition(motor);
                        publishReference(motor);
                    }
                    motorStateSnapshot.store(static_cast<uint8_t>(motor.state()),
                                             std::memory_order_release);
                }
            }
        }
        if (realTrackProfileActive) {
            ReferenceTimerSample atTrack{};
            if (stepTimerTakeReferenceDone(atTrack)) {
                realTrackProfileActive = false;
                motorMoving.store(false, std::memory_order_release);
                if (atTrack.reason == ReferenceEnd::STEP_LIMIT &&
                    atTrack.position == expectedTargetPosition) {
                    realReferencePosition.store(atTrack.position,
                                                std::memory_order_release);
                    motor.onMoveFinished();
                    confirmedTrack = targetTrack;
                    publishTrack(targetTrack, confirmedTrack,
                                 targetTrack != 0U ? TRACK_STATUS_REACHED : TRACK_STATUS_ERROR);
                } else {
                    motor.onProfileError();
                    realReferencePosition.store(kPositionUnknown,
                                                std::memory_order_release);
                    targetTrack = 0U;
                    confirmedTrack = 0U;
                    publishTrack(0U, 0U, TRACK_STATUS_ERROR);
                }
                publishPosition(motor);
                publishReference(motor);
                motorStateSnapshot.store(static_cast<uint8_t>(motor.state()),
                                         std::memory_order_release);
            } else {
                realReferencePosition.store(stepTimerReferenceSnapshot().position,
                                            std::memory_order_release);
                publishPosition(motor);
            }
        }

        // Fahrtabschluss ist ein Motorereignis, kein neues UART-ACK.
        // Haltemoment bleibt bis zum ausdruecklichen GO AUS erhalten.
        uint32_t pulsesDone = 0U;
        bool endstopHit = false;
        if (stepTimerTakeDone(pulsesDone, endstopHit)) {
            motorMoving.store(false, std::memory_order_release);
            if (endstopHit) {
                // Normale Fahrt wurde vom Endschalter unterbrochen.
                // Weder Fahrtabschluss noch gueltige absolute REF melden.
                motor.onProfileError();
                realReferencePosition.store(kPositionUnknown,
                                            std::memory_order_release);
                targetTrack = 0U;
                confirmedTrack = 0U;
                publishTrack(0U, 0U, TRACK_STATUS_ERROR);
            } else {
                motor.onMotionProgress(pulsesDone);
                motor.onMoveFinished();
                if (targetTrack != 0U && motor.benchReferenceValid()) {
                    confirmedTrack = targetTrack;
                    publishTrack(targetTrack, confirmedTrack, TRACK_STATUS_REACHED);
                }
            }
            publishPosition(motor);
            publishReference(motor);
            motorStateSnapshot.store(static_cast<uint8_t>(motor.state()), std::memory_order_release);
        } else if (motorMoving.load(std::memory_order_acquire) &&
                   !realTrackProfileActive && !motor.referenceActive()) {
            // Position wird als Core-1-Snapshot veroeffentlicht, nicht direkt von UART/ISR gelesen.
            motor.onMotionProgress(stepTimerCompletedSteps());
            publishPosition(motor);
            publishReference(motor);
        }

        // Lange REF erst NACH der angehaengten BG3-Fahrt persistieren.
        if (motor.calibrationSavePending() && !motorMoving.load(std::memory_order_acquire) &&
            !realTrackProfileActive && !motor.referenceActive() &&
            motor.state() == MotorState_t::IDLE) {
            if (!motor.persistCalibration()) {
                motor.onProfileError();
                realReferencePosition.store(kPositionUnknown,
                                            std::memory_order_release);
                publishPosition(motor);
                publishReference(motor);
                publishTrack(0U, 0U, TRACK_STATUS_ERROR);
                motorStateSnapshot.store(static_cast<uint8_t>(motor.state()),
                                         std::memory_order_release);
            }
        }
        if (xQueueReceive(context->commands, &cmd, pdMS_TO_TICKS(2)) != pdTRUE)
            continue;

        // Nano-GO/STOP-Freigabeverhalten bleibt erhalten;
        // der MOVE-Testpfad darf eine begrenzte STEP-Fahrt starten.
        if (cmd.id == CMD_GO &&
            cmd.data[0] == 0x55U && cmd.data[1] == 0xAAU) {
            // GO 55 AA: Treiber stromlos schalten; SLEEP bleibt HIGH.
            // STOP-Verriegelung bleibt unveraendert.
            digitalWrite(MOTOR_ENABLE_PIN, HIGH);
            driverEnabled.store(false, std::memory_order_release);
            motor.referenceCancel();
            realTrackProfileActive = false; // GO OFF aborts pending BG3 just like STOP.
            stepTimerStop();
            motorMoving.store(false, std::memory_order_release);
            realReferencePosition.store(kPositionUnknown, std::memory_order_release);
            motor.onStop(stopActive);
            targetTrack = 0U;
            confirmedTrack = 0U;
            publishPosition(motor);
            publishReference(motor);
            publishTrack(0U, 0U, TRACK_STATUS_ERROR);
            motorStateSnapshot.store(static_cast<uint8_t>(motor.state()), std::memory_order_release);
            emit(context, cmd, true, cmd.id);
        } else if (cmd.id == CMD_GO &&
                   cmd.data[0] == 0xAAU && cmd.data[1] == 0x55U) {
            // GO AA 55: wiederholbare Freigabe ohne STEP-Erzeugung.
            // Laufende STEP-Fahrt nicht ueberschreiben.
            if (stopActive || motor.referenceActive()) {
                emit(context, cmd, false, nackData(
                    motor.referenceActive() ? NackReason::BUSY : NackReason::NOT_READY));
            } else {
                if (!driverEnabled.load(std::memory_order_acquire)) {
                    // HW3-Treiber wurde vor Task-Erstellung in Muninn_initMotor() geweckt;
                    // STOP-Verriegelung bestimmt die Freigabe.
                    digitalWrite(MOTOR_PWM_PIN, LOW);
                    digitalWrite(MOTOR_ENABLE_PIN, LOW); // active LOW
                    driverEnabled.store(true, std::memory_order_release);
                }
                motor.onGoEnabled();
                motorStateSnapshot.store(static_cast<uint8_t>(motor.state()), std::memory_order_release);
                emit(context, cmd, true, cmd.id);
            }
        } else if (cmd.id == CMD_REFERENCE) {
            // Reale physische REF: 00=kurz (Kalibrierung erforderlich), 01=lang.
            if (stopActive || !driverEnabled.load(std::memory_order_acquire) ||
                // Mit kalibrierungsfehlerhaftem NVM nur echte lange REF
                // zur Reparatur zulassen, sonst NACK.
                (motor.hasNvmFault() &&
                 !(cmd.data[0] == 1U &&
                   (Muninn_nvmStatusSnapshot() & 0x00FFU) == 0x01U))) {
                emit(context, cmd, false, nackData(NackReason::NOT_READY));
            } else if (stepTimerRunning() || motor.referenceActive() ||
                       motor.state() != MotorState_t::IDLE) {
                emit(context, cmd, false, nackData(NackReason::BUSY));
            } else if (motor.prepareReference(cmd.data[0]) !=
                       MotorCommandResult_t::ACCEPTED ||
                       (cmd.data[0] == 0U && !motor.referenceCalibrated())) {
                emit(context, cmd, false, nackData(NackReason::NOT_READY));
            } else {
                if (!motor.referenceBegin(cmd.data[0] == 1U)) {
                    emit(context, cmd, false, nackData(NackReason::NOT_READY));
                } else {
                    motor.onReferenceStarted();
                    targetTrack = 0U;
                    confirmedTrack = 0U;
                    realReferencePosition.store(kPositionUnknown,
                                                std::memory_order_release);
                    motorMoving.store(true, std::memory_order_release);
                    publishPosition(motor);
                    publishReference(motor);
                    publishTrack(0U, 0U, TRACK_STATUS_ERROR);
                    motorStateSnapshot.store(static_cast<uint8_t>(motor.state()),
                                             std::memory_order_release);
                    emit(context, cmd, true, cmd.id);
                }
            }
        } else if (cmd.id == CMD_SET_TRACK ||
                   cmd.id == CMD_MOVE || cmd.id == CMD_SET_POSITION) {
            const bool realTrack = motor.referenced();
            if (stopActive || !driverEnabled.load(std::memory_order_acquire) ||
                motor.hasNvmFault() ||
                (cmd.id == CMD_SET_TRACK && !motor.tracksValid()) ||
                (cmd.id == CMD_SET_POSITION && !motor.referenced())) {
                emit(context, cmd, false, nackData(NackReason::NOT_READY));
            } else if (stepTimerRunning() || realTrackProfileActive ||
                       motor.referenceActive() ||
                       motor.state() != MotorState_t::IDLE) {
                emit(context, cmd, false, nackData(NackReason::BUSY));
            } else {
                MoveRequest request{};
                MotorCommandResult_t prep = MotorCommandResult_t::STOP_ACTIVE;
                if (cmd.id == CMD_SET_TRACK) {
                    prep = motor.prepareSetTrack(cmd.data[0], request);
                } else if (cmd.id == CMD_MOVE) {
                    prep = motor.prepareMove(cmd.data, request);
                } else if (cmd.id == CMD_SET_POSITION) {
                    prep = motor.prepareSetPosition(
                        static_cast<uint16_t>(cmd.data[0]) |
                        (static_cast<uint16_t>(cmd.data[1]) << 8U), request);
                }
                if (prep != MotorCommandResult_t::ACCEPTED) {
                    emit(context, cmd, false, nackData(
                        prep == MotorCommandResult_t::INVALID_DATA
                        ? NackReason::INVALID_PARAMETER
                        : NackReason::NOT_READY));
                } else if (request.steps == 0U) {
                    if (cmd.id == CMD_SET_TRACK) {
                        targetTrack = cmd.data[0];
                        confirmedTrack = targetTrack;
                        publishTrack(targetTrack, confirmedTrack, TRACK_STATUS_REACHED);
                    } else {
                        targetTrack = 0U;
                        confirmedTrack = 0U;
                        publishTrack(0U, 0U, TRACK_STATUS_ERROR);
                    }
                    publishReference(motor);
                    emit(context, cmd, true, cmd.id);
                } else {
                    // Gemeinsame Nano-Profilvorbereitung fuer reale und Testfahrten.
                    StepProfileSegment stages[STEP_PROFILE_PHASES]{};
                    const auto& p = motor.profileParams();
                    const uint16_t rampEvery[STEP_PROFILE_PHASES]{
                        p.Acc1RampInterval, p.Acc2RampInterval, 1U,
                        p.Bre1RampInterval, p.Bre2RampInterval, 1U
                    };
                    for (uint8_t i = 0U; i < STEP_PROFILE_PHASES; ++i) {
                        stages[i].steps = motor.profile()[i].Steps;
                        stages[i].accel = motor.profile()[i].Accel;
                        stages[i].rampEvery = rampEvery[i];
                        if (i == static_cast<uint8_t>(MotorProfile_t::BRE2) &&
                            request.steps > p.PosiMin &&
                            request.steps <= static_cast<uint32_t>(p.PosiMin) + p.KonstMin)
                            stages[i].rampEvery = 1U;
                    }
                    // Reale Referenz benoetigt den STEP-Pfad mit Positionsverfolgung.
                    // Test-MOVE/SET_POSITION behalten ihren bisherigen Ausfuehrungspfad.
                    const uint16_t startHz =
                        request.steps <= p.PosiMin ? p.VPos : p.VMin;
                    const bool configured =
                        stepTimerConfigureProfile(p.VMin,p.VMax,p.VPos);
                    const uint32_t motionStart = realTrack
                        ? stepTimerReferenceSnapshot().position : 0U;
                    const uint32_t motionEnd = realTrack
                        ? (request.directionRight ? motionStart + request.steps
                            : motionStart - request.steps) : 0U;
                    const bool started = configured &&
                        (realTrack
                         ? stepTimerStartReferenceProfile(
                               request.steps, request.directionRight, stages, startHz)
                         : stepTimerStartProfile(
                               request.steps, request.directionRight, stages, startHz));
                    if (!started) {
                        emit(context, cmd, false, nackData(NackReason::NOT_READY));
                    } else {
                        if (realTrack) {
                            realTrackProfileActive = true;
                            expectedTargetPosition = motionEnd;
                        }
                        else motor.onMotionAccepted(request);
                        motorMoving.store(true, std::memory_order_release);
                        motor.onMoveStarted();
                        if (cmd.id == CMD_SET_TRACK) {
                            targetTrack = cmd.data[0];
                            confirmedTrack = 0U;
                            publishTrack(targetTrack, 0U, TRACK_STATUS_MOVING);
                        } else {
                            targetTrack = 0U;
                            confirmedTrack = 0U;
                            publishTrack(0U, 0U, TRACK_STATUS_ERROR);
                        }
                        publishPosition(motor);
                        publishReference(motor);
                        motorStateSnapshot.store(
                            static_cast<uint8_t>(motor.state()),
                            std::memory_order_release);
                        emit(context, cmd, true, cmd.id);
                    }
                }
            }
        } else if (cmd.id == CMD_SET_PARAM &&
                   ((cmd.data[0]>=static_cast<uint8_t>(ParamId::ACC1) &&
                     cmd.data[0]<=static_cast<uint8_t>(ParamId::B2_ST)) ||
                    (cmd.data[0]>=static_cast<uint8_t>(ParamId::V_MIN) &&
                     cmd.data[0]<=static_cast<uint8_t>(ParamId::V_POS)) ||
                    (cmd.data[0]>=static_cast<uint8_t>(ParamId::A1_LEN) &&
                     cmd.data[0]<=static_cast<uint8_t>(ParamId::KONST_MIN)))) {
            const uint32_t value=static_cast<uint32_t>(cmd.data[1]) |
                (static_cast<uint32_t>(cmd.data[2])<<8U) |
                (static_cast<uint32_t>(cmd.data[3])<<16U);
            if(stepTimerRunning() || motorMoving.load(std::memory_order_acquire) ||
               realTrackProfileActive || motor.referenceActive() ||
               (motor.state()!=MotorState_t::IDLE && motor.state()!=MotorState_t::STOPPED)) {
                emit(context,cmd,false,nackData(NackReason::BUSY));
            } else if (!motor.rampParametersValid()) {
                emit(context,cmd,false,nackData(NackReason::NOT_READY));
            } else if (!motor.setRampParameter(cmd.data[0],value)) {
                emit(context,cmd,false,nackData(NackReason::INVALID_PARAMETER));
            } else emit(context,cmd,true,cmd.id);
        } else if (cmd.id == CMD_SET_PARAM &&
                   cmd.data[0] >= static_cast<uint8_t>(ParamId::TRACK_BG1) &&
                   cmd.data[0] <= static_cast<uint8_t>(ParamId::TRACK_BG5)) {
            const uint32_t position = static_cast<uint32_t>(cmd.data[1]) |
                (static_cast<uint32_t>(cmd.data[2]) << 8U) |
                (static_cast<uint32_t>(cmd.data[3]) << 16U);
            const uint8_t track = static_cast<uint8_t>(
                cmd.data[0] - static_cast<uint8_t>(ParamId::TRACK_BG1) + 1U);
            if (stepTimerRunning() || motorMoving.load(std::memory_order_acquire) ||
                realTrackProfileActive || motor.referenceActive() ||
                (motor.state() != MotorState_t::IDLE &&
                 motor.state() != MotorState_t::STOPPED)) {
                emit(context, cmd, false, nackData(NackReason::BUSY));
            } else if (!motor.tracksValid()) {
                emit(context, cmd, false, nackData(NackReason::NOT_READY));
            } else if (!motor.setTrackPosition(track, position)) {
                emit(context, cmd, false, nackData(NackReason::INVALID_PARAMETER));
            } else {
                targetTrack = 0U;
                confirmedTrack = 0U;
                publishTrack(0U, 0U, TRACK_STATUS_ERROR);
                emit(context, cmd, true, cmd.id);
            }
        } else if (cmd.id == CMD_SET_PARAM &&
                   cmd.data[0] == static_cast<uint8_t>(ParamId::SAVE)) {
            if (cmd.data[2] != PARAM_SAVE_PATTERN_1 ||
                cmd.data[3] != PARAM_SAVE_PATTERN_2 ||
                (cmd.data[1] != static_cast<uint8_t>(ParamSaveArea::TRACK_POSITIONS) &&
                 cmd.data[1] != static_cast<uint8_t>(ParamSaveArea::MOTOR_PARAMETERS))) {
                emit(context, cmd, false, nackData(NackReason::INVALID_PARAMETER));
            } else if (stepTimerRunning() || motorMoving.load(std::memory_order_acquire) ||
                       realTrackProfileActive || motor.referenceActive() ||
                       (motor.state() != MotorState_t::IDLE &&
                 motor.state() != MotorState_t::STOPPED)) {
                emit(context, cmd, false, nackData(NackReason::BUSY));
            } else if (cmd.data[1] == static_cast<uint8_t>(ParamSaveArea::MOTOR_PARAMETERS)) {
                bool changed=false;
                if(!motor.rampParametersValid())
                    emit(context,cmd,false,nackData(NackReason::NOT_READY));
                else if(!motor.saveRampParameters(changed))
                    emit(context,cmd,false,nackData(NackReason::EXECUTION_ERROR));
                else {
                    if(changed) { motor.invalidateCurrentReference(); publishReference(motor); }
                    emit(context,cmd,true,cmd.id);
                }
            } else if (!motor.tracksValid()) {
                emit(context, cmd, false, nackData(NackReason::NOT_READY));
            } else if (!motor.saveTrackPositions()) {
                emit(context, cmd, false, nackData(NackReason::EXECUTION_ERROR));
            } else {
                emit(context, cmd, true, cmd.id);
            }
        } else if (cmd.id == CMD_SET_PARAM &&
                   cmd.data[0] == static_cast<uint8_t>(ParamId::RESTORE_DEFAULT)) {
            if (cmd.data[2] != PARAM_SAVE_PATTERN_1 ||
                cmd.data[3] != PARAM_SAVE_PATTERN_2 ||
                (cmd.data[1] != static_cast<uint8_t>(ParamSaveArea::TRACK_POSITIONS) &&
                 cmd.data[1] != static_cast<uint8_t>(ParamSaveArea::MOTOR_PARAMETERS))) {
                // Motor-Defaults erst nach fachlicher NVM-Anbindung; Kalibrierung nie.
                emit(context, cmd, false, nackData(NackReason::INVALID_PARAMETER));
            } else if (stepTimerRunning() || motorMoving.load(std::memory_order_acquire) ||
                       realTrackProfileActive || motor.referenceActive() ||
                       (motor.state() != MotorState_t::IDLE &&
                 motor.state() != MotorState_t::STOPPED)) {
                emit(context, cmd, false, nackData(NackReason::BUSY));
            } else if (cmd.data[1] == static_cast<uint8_t>(ParamSaveArea::MOTOR_PARAMETERS)) {
                bool changed=false;
                if(!motor.restoreRampDefaults(changed))
                    emit(context,cmd,false,nackData(NackReason::EXECUTION_ERROR));
                else {
                    if(changed) { motor.invalidateCurrentReference(); publishReference(motor); }
                    emit(context,cmd,true,cmd.id);
                }
            } else if (!motor.restoreTrackDefaults()) {
                emit(context, cmd, false, nackData(NackReason::EXECUTION_ERROR));
            } else {
                // Der alte Ziel-/Gleisstatus soll keine neu gesetzten Gleise behaupten.
                targetTrack = 0U;
                confirmedTrack = 0U;
                publishTrack(0U, 0U, TRACK_STATUS_ERROR);
                emit(context, cmd, true, cmd.id);
            }
        } else {
            // Andere noch nicht implementierte Realbefehle bleiben abgewiesen.
            emit(context, cmd, false, nackData(NackReason::NOT_READY));
        }

        ledState = !ledState;
        setLed(ledState);
    }
}

BaseType_t Muninn_startMotor(BringupContext* context) {
    return xTaskCreatePinnedToCore(
        Muninn_motorTask, "Muninn_Motor", 4096,
        context, 3, nullptr, CORE_MUNINN);
}
} // namespace fy
