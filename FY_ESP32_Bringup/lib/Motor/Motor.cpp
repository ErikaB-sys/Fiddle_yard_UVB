#include "Motor.h"
#include "StepTimer.h"
#include "FY_BenchPosition.h"
#include "HW_Config.h"
#include "Error.h"
#include "NVM.h"
#include <freertos/task.h>
#include <atomic>

#ifndef FY_MOTOR_BENCH_TEST
#define FY_MOTOR_BENCH_TEST 0
#endif

#ifndef FY_STATUS_LED_PIN
#define FY_STATUS_LED_PIN 2
#endif
#ifndef FY_STATUS_LED_ACTIVE_HIGH
#define FY_STATUS_LED_ACTIVE_HIGH 1
#endif

namespace fy {

// Taken from the verified Nano Motor::calcProfile(), unchanged calculation.
bool Motor::calcProfile(uint16_t distance)
{
    // Sehr kurze Strecke:
    // Kein vollstaendiges Profil moeglich.
    // Die gesamte Strecke wird mit V_MIN gefahren.
    if (distance <= (_ProfileParam.PosiMin + _ProfileParam.KonstMin))
    {
        for (uint8_t i = 0; i < 6; ++i)
        {
            Motor_profil[i].Steps = 0;
            Motor_profil[i].Accel = 0;
        }

        Motor_profil[static_cast<uint8_t>(MotorProfile_t::KONST)].Steps =
            distance;

        _TimerValid = true;
        return true;
    }

    for (uint8_t i = 0; i < 6; ++i)
    {
        Motor_profil[i].Steps = 0;
        Motor_profil[i].Accel = 0;
    }

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::POSI)].Steps =
        _ProfileParam.PosiMin;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::KONST)].Steps =
        _ProfileParam.KonstMin;

    uint16_t remaining =
        distance
        - _ProfileParam.PosiMin
        - _ProfileParam.KonstMin;

    uint16_t outer = remaining / 2;

    if (outer > _ProfileParam.Acc1Steps)
        outer = _ProfileParam.Acc1Steps;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC1)].Steps = outer;
    Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC1)].Accel =
        -static_cast<int16_t>(_ProfileParam.Acc1Accel);

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE2)].Steps = outer;
    Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE2)].Accel =
        static_cast<int16_t>(_ProfileParam.Bre2Accel);

    remaining -= outer * 2;

    uint16_t inner = remaining / 2;

    if (inner > _ProfileParam.Acc2Steps)
        inner = _ProfileParam.Acc2Steps;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC2)].Steps = inner;
    Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC2)].Accel =
        -static_cast<int16_t>(_ProfileParam.Acc2Accel);

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE1)].Steps = inner;
    Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE1)].Accel =
        static_cast<int16_t>(_ProfileParam.Bre1Accel);

    remaining -= inner * 2;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::KONST)].Steps +=
        remaining;

    uint16_t sum = 0;

    for (uint8_t i = 0; i < 6; ++i)
        sum += Motor_profil[i].Steps;

    if (sum < distance)
    {
        Motor_profil[static_cast<uint8_t>(MotorProfile_t::POSI)].Steps +=
            distance - sum;
    }

    _TimerValid = true;
    return true;
}

// Motor state transitions run only in Muninn (core 1). ISR status is
// consumed in the task, never used for unsolicited UART telegrams.
void Motor::onGoEnabled() {
    if (!_StopActive && _State == MotorState_t::STOPPED)
        _State = MotorState_t::IDLE;
}
void Motor::onStop(bool latched) {
    _StopActive = latched;
    _ReferenceValid = false;
    _State = MotorState_t::STOPPED;
    // STOP / GO OFF must never invent a new reference position.
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
        // Calibration (as distinct from current reference) survives STOP.
    }

void Motor::referenceUpdate() {
        if (!_active) return;
        ReferenceTimerSample s{};
        if (!stepTimerTakeReferenceDone(s)) return;
        _position = s.position;
        switch (_stage) {
        case RefRunState::SEEK_MOTOR:
            if (s.reason != ReferenceEnd::ENDSTOP) return referenceFail();
            // Nano short reference establishes this position at LSL.
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
            // Nano's coarse+1200 approach with right-end clearance.
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
    // One diagnostic line only on failure (no tracing in STEP ISR).
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
void Motor::resetBenchPosition() {
#if FY_MOTOR_BENCH_TEST
    // Explicit fake short reference: assign measured HW_TEST coordinate 1000.
    // Never claim a *physical* reference without sensor confirmation.
    _BenchPosition = FY_BENCH_REFERENCE_POSITION;
    _BenchPositionValid = true;
    _BenchReferenceValid = true;
    _MotionPositionTracked = false;
    _MotionStartPosition = _BenchPosition;
    _MotionStepCount = 0U;
#else
    invalidateBenchPosition();
#endif
}

MotorCommandResult_t Motor::prepareSetTrack(uint8_t track, MoveRequest& request) {
    uint32_t destination = 0U;
    if (!trackPosition(track, destination))
        return MotorCommandResult_t::INVALID_DATA;
    if (_StopActive || _State != MotorState_t::IDLE)
        return MotorCommandResult_t::STOP_ACTIVE;
    if (_ReferenceValid) {
        // Real REF replaces synthetic benchReferenceValid() as source of truth.
        // The ISR owns the current absolute coordinate during profiled moves.
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
    // Fake reference must never enable SET_TRACK.
    return MotorCommandResult_t::STOP_ACTIVE;
}

MotorCommandResult_t Motor::prepareSetPosition(uint16_t target,
                                                MoveRequest& request) {
    if (_StopActive || _State != MotorState_t::IDLE)
        return MotorCommandResult_t::STOP_ACTIVE;
#if FY_MOTOR_BENCH_TEST
    // A synthetic position is intentionally DISTINCT from real REFRUN.
    if (!_BenchPositionValid)
        return MotorCommandResult_t::STOP_ACTIVE;
    if (target < FY_BENCH_MIN_POSITION ||
        target > FY_BENCH_MAX_POSITION)
        return MotorCommandResult_t::INVALID_DATA;
    const uint16_t steps = benchAbsoluteDistance(_BenchPosition, target);
    if (steps > FY_BENCH_MAX_ABSOLUTE_MOVE)
        return MotorCommandResult_t::INVALID_DATA;
    if (steps != 0U &&
        !benchCanMove(_BenchPosition, steps, target > _BenchPosition))
        return MotorCommandResult_t::INVALID_DATA;
    request.mode = FY_MOTOR_MICROSTEP_MODE;
    request.directionRight = target > _BenchPosition;
    request.steps = steps;
    if (steps > 0U &&
        (!calcProfile(steps) || !isProfileValid(steps)))
        return MotorCommandResult_t::INVALID_DATA;
    return MotorCommandResult_t::ACCEPTED;
#else
    // Real track/FY mode requires physical reference and checked limits.
    (void)target;
    (void)request;
    return MotorCommandResult_t::STOP_ACTIVE;
#endif
}

void Motor::onMotionAccepted(const MoveRequest& request) {
#if FY_MOTOR_BENCH_TEST
    // Eighth-step positions are meaningful only while resolution stays 1/8.
    _MotionPositionTracked =
        _BenchPositionValid && request.mode == FY_MOTOR_MICROSTEP_MODE &&
        benchCanMove(_BenchPosition, request.steps, request.directionRight);
    _MotionStartPosition = _BenchPosition;
    _MotionStepCount = request.steps;
    _MotionRight = request.directionRight;
    if (!_MotionPositionTracked)
        invalidateBenchPosition();
#else
    (void)request;
#endif
}

void Motor::onMotionProgress(uint32_t completedSteps) {
#if FY_MOTOR_BENCH_TEST
    if (!_MotionPositionTracked || !_BenchPositionValid) return;
    const uint32_t done = completedSteps > _MotionStepCount
        ? _MotionStepCount : completedSteps;
    const int32_t pos = static_cast<int32_t>(_MotionStartPosition) +
        (_MotionRight ? static_cast<int32_t>(done)
                      : -static_cast<int32_t>(done));
    if (pos < FY_BENCH_MIN_POSITION ||
        pos > FY_BENCH_MAX_POSITION) {
        invalidateBenchPosition();
        return;
    }
    _BenchPosition = static_cast<uint16_t>(pos);
#else
    (void)completedSteps;
#endif
}
bool Motor::isProfileValid(uint16_t distance) const {
    if (!_TimerValid || distance == 0U) return false;
    uint32_t sum = 0U;
    for (const auto& phase : Motor_profil) sum += phase.Steps;
    return sum == distance;
}

// Pure payload validation. Command length and CRC are checked by Huginn.
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


// Parameter validation is kept in Motor (prepare path), not in the ISR.
MotorCommandResult_t Motor::prepareMove(const uint8_t* data,
                                        MoveRequest& request) {
    if (!decodeMovePayload(data, request))
        return MotorCommandResult_t::INVALID_DATA;
    // Driver MS pins never change after initialization.
    if (request.mode != FY_MOTOR_MICROSTEP_MODE)
        return MotorCommandResult_t::INVALID_DATA;
    // Guard the virtual position against overflow/underflow.
#if FY_MOTOR_BENCH_TEST
    if (_BenchPositionValid && request.mode == FY_MOTOR_MICROSTEP_MODE &&
        !benchCanMove(_BenchPosition, request.steps, request.directionRight))
        return MotorCommandResult_t::INVALID_DATA;
#endif
    // Prepare all six Nano phases; the STEP ISR now executes those
    // phases, but the new dynamic hardware timer still needs HW testing.
    if (!calcProfile(request.steps) || !isProfileValid(request.steps))
        return MotorCommandResult_t::INVALID_DATA;
    return MotorCommandResult_t::ACCEPTED;
}

namespace {
// Only Muninn writes this; Huginn reads through a C++ atomic snapshot.
std::atomic<bool> driverEnabled{false};
std::atomic<bool> motorMoving{false};
std::atomic<uint8_t> motorStateSnapshot{static_cast<uint8_t>(MotorState_t::STOPPED)};
// Wire marker bit 16 denotes BENCH-SIMULATED position; 0x7FFF alone
// retains its original Nano meaning (POSITION_UNKNOWN).
constexpr uint32_t kPositionUnknown = 0x00007FFFU;
constexpr uint32_t kPositionBenchFlag = 0x00010000U;
std::atomic<uint32_t> positionSnapshot{kPositionUnknown};
std::atomic<uint32_t> realReferencePosition{kPositionUnknown};
std::atomic<uint8_t> referenceStatusSnapshot{0U}; // 0 none, 1 real, 2 fake
std::atomic<uint16_t> nvmStatusSnapshot{0U}; // high byte valid mask, low byte fault mask
std::atomic<uint32_t> nvmTrackValues[5]{};
std::atomic<uint32_t> nvmRampValues[8]{};
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
#if FY_MOTOR_BENCH_TEST
    const uint32_t position = motor.benchPositionValid()
        ? (kPositionBenchFlag | static_cast<uint32_t>(motor.benchPosition()))
        : kPositionUnknown;
    positionSnapshot.store(position, std::memory_order_release);
#else
    (void)motor;
    positionSnapshot.store(kPositionUnknown, std::memory_order_release);
#endif
}

void publishReference(const Motor& motor) {
    const uint8_t status = motor.referenced() ? 1U :
        (motor.benchReferenceValid() ? 2U : 0U);
    referenceStatusSnapshot.store(status, std::memory_order_release);
}

void publishTrack(uint8_t target, uint8_t confirmed, uint8_t status) {
    const uint32_t packed = static_cast<uint32_t>(target) |
        (static_cast<uint32_t>(confirmed) << 8U) |
        (static_cast<uint32_t>(status) << 16U);
    trackSnapshot.store(packed, std::memory_order_release);
}

// The six-phase profile now sets the STEP frequency in the timer ISR.

void applyMicrostepMode(MoveMicrostep mode) {
    // A4988 MS1/MS2/MS3 truth table. Apply only while STEP is stopped.
    // Mode changes can invalidate a physical absolute position; bench only.
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
    (void)xQueueSend(context->events, &event, portMAX_DELAY);
}
} // anonymous namespace

bool Muninn_isDriverEnabled() {
    return driverEnabled.load(std::memory_order_acquire);
}

bool Muninn_isMoving() {
    return motorMoving.load(std::memory_order_acquire);
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
    if (id >= static_cast<uint8_t>(ParamId::ACC1) &&
        id <= static_cast<uint8_t>(ParamId::B2_ST) && (valid & 0x04U)) {
        value = nvmRampValues[id-static_cast<uint8_t>(ParamId::ACC1)].load(std::memory_order_acquire);
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
constexpr uint16_t kRampDefaults[8]{2U,1U,1U,2U,1U,2U,2U,1U};
void getRamp(const MotorProfileParam_t& p, uint16_t (&v)[8]) {
    v[0]=p.Acc1Accel; v[1]=p.Acc2Accel; v[2]=p.Bre1Accel; v[3]=p.Bre2Accel;
    v[4]=p.Acc1RampInterval; v[5]=p.Acc2RampInterval;
    v[6]=p.Bre1RampInterval; v[7]=p.Bre2RampInterval;
}
void putRamp(MotorProfileParam_t& p, const uint16_t (&v)[8]) {
    p.Acc1Accel=v[0]; p.Acc2Accel=v[1]; p.Bre1Accel=v[2]; p.Bre2Accel=v[3];
    p.Acc1RampInterval=v[4]; p.Acc2RampInterval=v[5];
    p.Bre1RampInterval=v[6]; p.Bre2RampInterval=v[7];
}
bool rampValid(const uint16_t (&v)[8]) {
    for(uint8_t i=0U;i<8U;++i)
        if(v[i]==0U || v[i]>(i<4U?64U:128U)) return false;
    return true;
}
void publishRamp(const uint16_t (&v)[8]) {
    for(uint8_t i=0U;i<8U;++i)
        nvmRampValues[i].store(v[i],std::memory_order_release);
}
void packRamp(const uint16_t (&v)[8],uint8_t (&raw)[16]) {
    for(uint8_t i=0U;i<8U;++i) {
        raw[2U*i]=static_cast<uint8_t>(v[i]);
        raw[2U*i+1U]=static_cast<uint8_t>(v[i]>>8U);
    }
}
void unpackRamp(const uint8_t (&raw)[16],uint16_t (&v)[8]) {
    for(uint8_t i=0U;i<8U;++i)
        v[i]=static_cast<uint16_t>(raw[2U*i] | (static_cast<uint16_t>(raw[2U*i+1U])<<8U));
}
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

void Muninn_initMotorMock() {
    // G32 (!SLEEP + !RESET) is held LOW during the entire startup.
    // The A4988 microstep inputs are configured while ENABLE is HIGH.
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);
    digitalWrite(MOTOR_PWM_PIN, LOW);
    pinMode(MOTOR_MS0_PIN, OUTPUT);
    pinMode(MOTOR_MS1_PIN, OUTPUT);
    pinMode(MOTOR_MS2_PIN, OUTPUT);
    applyMicrostepMode(FY_MOTOR_MICROSTEP_MODE);
    // Timer is armed ONLY when a bounded motion test is accepted.
    (void)stepTimerBegin();
#if FY_STATUS_LED_PIN >= 0
    pinMode(FY_STATUS_LED_PIN, OUTPUT);
    setLed(false);
#endif
    // HW3: Sleep/Reset is solely a power-on/boot interlock.
    // Wake the A4988 once, after all control lines have known states.
    // No UART or Muninn task exists yet; delay here cannot block commands.
    digitalWrite(MOTOR_SLEEP_PIN, HIGH);
    delayMicroseconds(1500U); // A4988 tWAKE >= 1 ms, outside timer ISR.
}

namespace {
// Raw active-HIGH input levels, as in Nano HW_TEST. LS4/G35 not connected:
// reserve bit 3 and do not sample the floating pin.
uint8_t readSensorLevels() {
    return static_cast<uint8_t>(
        ((digitalRead(LSL_PIN) == HIGH) ? 0x01U : 0U) |
        ((digitalRead(LSR_PIN) == HIGH) ? 0x02U : 0U) |
        ((digitalRead(LSREF_PIN) == HIGH) ? 0x04U : 0U));
}
} // anonymous namespace

void Muninn_motorMockTask(void* arg) {
    auto* context = static_cast<BringupContext*>(arg);
    CommandMessage cmd{};
    Motor motor{};
    motor.initializeStorage(); // Kein Motorschritt, POR-STOP bleibt gesetzt.
    bool realTrackProfileActive = false; // Real profiled motion: REF->BG3 / SET_TRACK
    bool ledState = false;
    // POR/Reset is always locked. Only an explicit STOP AA 55 releases it.
    bool stopActive = true; // Mirrors Motor state for the existing STOP path.
    uint8_t targetTrack = 0U; // 0 = unknown, only Muninn changes these.
    uint8_t confirmedTrack = 0U;
#if FY_MOTOR_BENCH_TEST
#endif

    // Sensor inputs belong exclusively to Muninn. External 10k pull-ups.
    pinMode(LSL_PIN, INPUT);
    pinMode(LSR_PIN, INPUT);
    pinMode(LSREF_PIN, INPUT);
    uint8_t lastSensorLevels = readSensorLevels();
    uint8_t sensorChangeCount = 0U; // high nibble 0..E; F reserved
    uint8_t sensorMessage = lastSensorLevels;
    (void)xQueueOverwrite(context->sensorSnapshots, &sensorMessage); // initial snapshot

    // POR STOP stays latched; the driver is awake after init but disabled.
    // !SLEEP/!RESET (G32) stays HIGH throughout normal GO/STOP operation.
    digitalWrite(MOTOR_PWM_PIN, LOW);
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);
    driverEnabled.store(false, std::memory_order_release);
    motorMoving.store(false, std::memory_order_release);
    // No fake reference at boot. REFERENCE 00 must be explicitly requested.
    publishPosition(motor);
    publishReference(motor);
    publishTrack(0U, 0U, TRACK_STATUS_ERROR);
    motorStateSnapshot.store(static_cast<uint8_t>(motor.state()), std::memory_order_release);

    for (;;) {
        // Latest-value mailbox: never block STOP or motor event delivery.
        const uint8_t sensorLevels = readSensorLevels();
        if (sensorLevels != lastSensorLevels) {
            lastSensorLevels = sensorLevels;
            sensorChangeCount = (sensorChangeCount == 0x0EU)
                ? 0U : static_cast<uint8_t>(sensorChangeCount + 1U);
            sensorMessage = static_cast<uint8_t>(
                (sensorChangeCount << 4U) | sensorLevels);
            (void)xQueueOverwrite(context->sensorSnapshots, &sensorMessage);
        }
        // STOP has its own mailbox and is always checked before normal jobs.
        if (xQueueReceive(context->priorityCommands, &cmd, 0) == pdTRUE) {
            const bool activate = cmd.data[0] == 0x55U && cmd.data[1] == 0xAAU;
            const bool release = cmd.data[0] == 0xAAU && cmd.data[1] == 0x55U;
            if (cmd.id != CMD_STOPP || (!activate && !release)) {
                emit(context, cmd, false,
                     nackData(NackReason::INVALID_PARAMETER));
                continue;
            }
            // First disable the driver, then stop STEP.
            // !SLEEP remains HIGH; STOP never resets the A4988 translator.
            // STOP and release both remain non-resuming, Nano semantics.
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

            // Cancel any old queued movement requests; every dropped request
            // still receives an explicit negative result.
            CommandMessage pending{};
            while (xQueueReceive(context->commands, &pending, 0) == pdTRUE)
                emit(context, pending, false, nackData(NackReason::NOT_READY));

            emit(context, cmd, true, cmd.id);
            ledState = !ledState;
            setLed(ledState);
            continue;
        }

        // Physical Nano REF state machine: STOP above always wins.
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
                    // One-shot, non-ISR diagnostics of reference-to-BG3 transition.
                    Serial.print(F("REF: VALID pos="));
                    Serial.print(motor.referencePosition());
                    Serial.print(F(" end="));
                    Serial.println(motor.referenceOppositeEnd());
                    // Nano HW_TEST always selected BG3 after short REF.
                    // The same intended post-condition now applies after
                    // successful long calibration as well.
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
                        }
                        if (stepTimerStartReferenceProfile(
                                static_cast<uint16_t>(distance),
                                kTrack3 > start, stages)) {
                            realTrackProfileActive = true;
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
                    targetTrack >= 1U && targetTrack <= FY_BENCH_TRACK_COUNT &&
                    ([&]() { uint32_t expected = 0U;
                        return motor.trackPosition(targetTrack, expected) &&
                               atTrack.position == expected; })()) {
                    realReferencePosition.store(atTrack.position,
                                                std::memory_order_release);
                    motor.onMoveFinished();
                    confirmedTrack = targetTrack;
                    publishTrack(targetTrack, confirmedTrack, TRACK_STATUS_REACHED);
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

        // A completed pulse train is a motor event, not a new UART ACK.
        // Completion retains coil holding torque until explicit GO off.
        uint32_t pulsesDone = 0U;
        bool endstopHit = false;
        if (stepTimerTakeDone(pulsesDone, endstopHit)) {
            motorMoving.store(false, std::memory_order_release);
            if (endstopHit) {
                // Normal move was interrupted by its destination switch.
                // Never report a completed job or a valid absolute REF.
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
            // Position is a core-1 snapshot, never read straight from UART/ISR.
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

        // Nano GO/STOP enable semantics remain; only the explicit bench
        // MOVE handler below may start a bounded STEP train.
        if (cmd.id == CMD_GO &&
            cmd.data[0] == 0x55U && cmd.data[1] == 0xAAU) {
            // GO 55 AA: de-energize; SLEEP remains HIGH.
            // STOP latch is not affected.
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
            // GO AA 55: idempotent enable, without generating any STEP.
            // Never overwrite STEP while an already-accepted train is running.
            if (stopActive || motor.referenceActive()) {
                emit(context, cmd, false, nackData(
                    motor.referenceActive() ? NackReason::BUSY : NackReason::NOT_READY));
            } else {
                if (!driverEnabled.load(std::memory_order_acquire)) {
                    // HW3 driver was awakened in Muninn_initMotorMock()
                    // before task creation; STOP latch governs release.
                    digitalWrite(MOTOR_PWM_PIN, LOW);
                    digitalWrite(MOTOR_ENABLE_PIN, LOW); // active LOW
                    driverEnabled.store(true, std::memory_order_release);
                }
                motor.onGoEnabled();
                motorStateSnapshot.store(static_cast<uint8_t>(motor.state()), std::memory_order_release);
                emit(context, cmd, true, cmd.id);
            }
        } else if (cmd.id == CMD_REFERENCE) {
            // Real physical REF 00=short (requires calibration), 01=long.
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
        } else if (cmd.id == CMD_SET_TRACK
#if FY_MOTOR_BENCH_TEST
                   || cmd.id == CMD_MOVE || cmd.id == CMD_SET_POSITION
#endif
                   ) {
            const bool realTrack = cmd.id == CMD_SET_TRACK && motor.referenced();
            if (stopActive || !driverEnabled.load(std::memory_order_acquire) ||
                motor.hasNvmFault() ||
                (cmd.id == CMD_SET_TRACK && !motor.tracksValid()) ||
                (motor.referenced() && !realTrack)) {
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
#if FY_MOTOR_BENCH_TEST
                } else if (cmd.id == CMD_MOVE) {
                    prep = motor.prepareMove(cmd.data, request);
                } else if (cmd.id == CMD_SET_POSITION) {
                    prep = motor.prepareSetPosition(
                        static_cast<uint16_t>(cmd.data[0]) |
                        (static_cast<uint16_t>(cmd.data[1]) << 8U), request);
#endif
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
                    // Common Nano profile preparation for real and bench moves.
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
                    }
                    // Real reference needs the position-tracking STEP backend.
                    // BENCH MOVE/SET_POSITION retain their existing backend.
                    const bool started = realTrack
                        ? stepTimerStartReferenceProfile(
                              request.steps, request.directionRight, stages)
                        : stepTimerStartProfile(
                              request.steps, request.directionRight, stages);
                    if (!started) {
                        emit(context, cmd, false, nackData(NackReason::NOT_READY));
                    } else {
                        if (realTrack) realTrackProfileActive = true;
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
                cmd.data[1] != static_cast<uint8_t>(ParamSaveArea::TRACK_POSITIONS)) {
                emit(context, cmd, false, nackData(NackReason::INVALID_PARAMETER));
            } else if (stepTimerRunning() || motorMoving.load(std::memory_order_acquire) ||
                       realTrackProfileActive || motor.referenceActive() ||
                       (motor.state() != MotorState_t::IDLE &&
                 motor.state() != MotorState_t::STOPPED)) {
                emit(context, cmd, false, nackData(NackReason::BUSY));
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
                cmd.data[1] != static_cast<uint8_t>(ParamSaveArea::TRACK_POSITIONS)) {
                // Motor-Defaults erst nach fachlicher NVM-Anbindung; Kalibrierung nie.
                emit(context, cmd, false, nackData(NackReason::INVALID_PARAMETER));
            } else if (stepTimerRunning() || motorMoving.load(std::memory_order_acquire) ||
                       realTrackProfileActive || motor.referenceActive() ||
                       (motor.state() != MotorState_t::IDLE &&
                 motor.state() != MotorState_t::STOPPED)) {
                emit(context, cmd, false, nackData(NackReason::BUSY));
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
            // Other unimplemented real commands remain rejected.
            emit(context, cmd, false, nackData(NackReason::NOT_READY));
        }

        ledState = !ledState;
        setLed(ledState);
    }
}

BaseType_t Muninn_startMotorMock(BringupContext* context) {
    return xTaskCreatePinnedToCore(
        Muninn_motorMockTask, "Muninn_Motor", 4096,
        context, 3, nullptr, CORE_MUNINN);
}
} // namespace fy
